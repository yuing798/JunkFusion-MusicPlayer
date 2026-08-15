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

    if (!mChildProcess.start(args)) {
        if (logger)
            logger->critical("启动后端进程失败: {}", exeFile.getFullPathName().toStdString());
        mSocket.close();
        return false;
    }
    if (logger) logger->info("后端进程已启动: {}", exeFile.getFullPathName().toStdString());
    mSocket.set(zmq::sockopt::rcvtimeo, 5000);
    mSocket.set(zmq::sockopt::sndtimeo, 5000);

    mRunning = true;
    return true;
}

void AudioProcessCoordinator::stop() {
    mChildProcess.kill();
    mSocket.close();
    mRunning = false;
}

AudioProcessCoordinator::~AudioProcessCoordinator() {}
