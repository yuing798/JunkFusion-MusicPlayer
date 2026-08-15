#include "AudioProcessCoordinator.h"
#include "./AudioDefs.hpp"
#include "constants.h"
#include "otherUtils.hpp"
#include "spdlog/spdlog.h"

bool AudioProcessCoordinator::start(const juce::File& exeFile, int timeoutMs) {
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
    args.add(exeFile.getFullPathName());

    if (!mChildProcess.start(args)) {
        if (logger)
            logger->critical("启动后端进程失败: {}", exeFile.getFullPathName().toStdString());
        mSocket.close();
        return false;
    }
    if (logger) logger->info("后端进程已启动: {}", exeFile.getFullPathName().toStdString());

    // ── 3. 等待子进程发来 "ready"，收到后回发 "helloworld" ──
    mSocket.set(zmq::sockopt::rcvtimeo, timeoutMs); // 接收超时，防止子进程没连上时永久阻塞

    zmq::message_t request;

    // zmq::recv_flags::none 的意思是：“阻塞模式（Blocking Mode）”。
    //  它告诉 recv 函数：“在没有收到消息或者未超时之前，这个线程就死等在这里，绝对不返回。
    auto recvResult = mSocket.recv(request, zmq::recv_flags::none);

    if (!recvResult) { // 超时没收到 ready
        if (logger) logger->critical("等待子进程 ready 超时");
        stop();
        return false;
    }

    std::string ready(request.to_string());
    if (logger) logger->info("收到子进程握手信号: {}", ready);

    // 回发一句 helloworld
    zmq::message_t reply(std::string("helloworld"));
    mSocket.send(reply, zmq::send_flags::none);
    if (logger) logger->info("已回发 helloworld 给子进程");

    mRunning = true;
    return true;
}

void AudioProcessCoordinator::stop() {
    mChildProcess.kill();
    mSocket.close();
    mRunning = false;
}
