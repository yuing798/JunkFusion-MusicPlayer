#include "AudioProcessCoordinator.h"
#include "./AudioDefs.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include "otherUtils.hpp"
#include "spdlog/spdlog.h"
#include <string>
#include <zmq.hpp>
AudioProcessCoordinator::AudioProcessCoordinator() {}

bool AudioProcessCoordinator::start(
    const juce::File& exeFile,
    int oscPort,
    juce::File cacheDir,
    std::string pushPollPort,
    std::string pubSubPort
) {
    auto logger{spdlog::get(LogDllID)};

    // ── 2. 构造子进程命令行，把 zmq 地址塞进 --zmq-endpoint ──
    juce::StringArray args;
    args.add(exeFile.getFullPathName());

    args.add(AudioDefs::pushPullPort);
    args.add(juce::String(pushPollPort));
    args.add(AudioDefs::pubSubPort);
    args.add(juce::String(pubSubPort));
    args.add(AudioDefs::cacheDir);
    args.add(cacheDir.getFullPathName());
    args.add(AudioDefs::oscPort);
    args.add(juce::String(oscPort));

    if (!mChildProcess.start(args)) {
        if (logger)
            logger->critical("启动后端进程失败: {}", exeFile.getFullPathName().toStdString());
        return false;
    }
    if (logger) logger->info("后端进程已启动: {}", exeFile.getFullPathName().toStdString());

    mRunning = true;
    return true;
}

void AudioProcessCoordinator::stop() {

    // std::string endInfo = "kill";
    // zmq::message_t msg(endInfo.data(), endInfo.size());
    // auto r = mSocket.send(msg, zmq::send_flags::none);
    // Utils::writeEmergencyLog(("send返回:" + std::to_string(r.value_or(0))).c_str());
    // if (!mChildProcess.waitForProcessToFinish(2000)) { // 最多等两秒结束子进程
    //     Utils::writeEmergencyLog("音频进程未能退出，准备强制kill");
    //     mChildProcess.kill();
    // }
    // mSocket.close();
    // mRunning = false;
}

AudioProcessCoordinator::~AudioProcessCoordinator() {}

AudioProcessPusher::AudioProcessPusher(zmq::context_t& context)
    : juce::Thread("AudioProcessPusher"), mContext(context) {}

void AudioProcessPusher::run() {
    zmq::socket_t socket(mContext, zmq::socket_type::push);
    socket.bind("tcp://127.0.0.1:*");
    mPushPullPort = socket.get(zmq::sockopt::last_endpoint);
    while (!threadShouldExit()) {
    }
}

AudioProcessSuber::AudioProcessSuber(zmq::context_t& context)
    : juce::Thread("AudioProcessSuber"), mContext(context) {}

void AudioProcessSuber::run() {
    zmq::socket_t socket(mContext, zmq::socket_type::sub);
    socket.bind("tcp://127.0.0.1:*");
    mPubSubPort = socket.get(zmq::sockopt::last_endpoint);
    while (!threadShouldExit()) {
    }
}
