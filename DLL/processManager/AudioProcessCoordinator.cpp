#include "AudioProcessCoordinator.h"
#include "./AudioDefs.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include "otherUtils.hpp"
#include "spdlog/spdlog.h"
#include <zmq.hpp>
AudioProcessCoordinator::AudioProcessCoordinator() {}

bool AudioProcessCoordinator::start(const juce::File& exeFile, int oscPort, juce::File cacheDir) {
    auto logger{spdlog::get(LogDllID)};

    mSocket = zmq::socket_t(mContext, zmq::socket_type::dealer);

    mSocket.bind("tcp://127.0.0.1:*"); // "*" = 端口 0，系统自动挑一个空闲端口

    // 获得随机分配的端口号
    std::string endpoint = mSocket.get(zmq::sockopt::last_endpoint); // 例如 "tcp://127.0.0.1:55555"

    // ── 2. 构造子进程命令行，把 zmq 地址塞进 --zmq-endpoint ──
    juce::StringArray args;
    args.add(exeFile.getFullPathName());

    args.add(AudioDefs::zmqEndpoint);
    args.add(juce::String(endpoint));
    args.add(AudioDefs::cacheDir);
    args.add(cacheDir.getFullPathName());
    args.add(AudioDefs::oscPort);
    args.add(juce::String(oscPort));
    Utils::writeEmergencyLog("音频进程协调者的zmq端口号为");
    Utils::writeEmergencyLog(endpoint.c_str());

    if (!mChildProcess.start(args)) {
        if (logger)
            logger->critical("启动后端进程失败: {}", exeFile.getFullPathName().toStdString());
        mSocket.close();
        return false;
    }
    if (logger) logger->info("后端进程已启动: {}", exeFile.getFullPathName().toStdString());
    mSocket.set(zmq::sockopt::rcvtimeo, 5000);
    mSocket.set(zmq::sockopt::sndtimeo, 5000);

    // 等待 worker 主动发来的握手消息 "ready"，确认 DEALER 连接已真正建立。
    // DEALER 连接建立是异步的，这里 recv 一次能保证后续 stop() 发的 kill 能送达。
    zmq::message_t handshake;
    auto recvResult = mSocket.recv(handshake, zmq::recv_flags::none);
    if (recvResult.has_value()) {
        std::string handshakeStr(static_cast<char*>(handshake.data()), handshake.size());
        Utils::writeEmergencyLog(("收到worker握手:" + handshakeStr).c_str());
    } else {
        Utils::writeEmergencyLog("等待worker握手超时");
    }
    Utils::writeEmergencyLog(
        ("start 协调者 thread id: " +
         std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())))
            .c_str()
    );

    mRunning = true;
    return true;
}

void AudioProcessCoordinator::stop() {

    std::string endInfo = "kill";
    zmq::message_t msg(endInfo.data(), endInfo.size());
    auto r = mSocket.send(msg, zmq::send_flags::none);
    Utils::writeEmergencyLog(("send返回:" + std::to_string(r.value_or(0))).c_str());
    if (!mChildProcess.waitForProcessToFinish(2000)) { // 最多等两秒结束子进程
        Utils::writeEmergencyLog("音频进程未能退出，准备强制kill");
        mChildProcess.kill();
    }
    mSocket.close();
    mRunning = false;
}

AudioProcessCoordinator::~AudioProcessCoordinator() {
    stop();
    Utils::writeEmergencyLog(
        ("stop 协调者 thread id: " +
         std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())))
            .c_str()
    );
}
