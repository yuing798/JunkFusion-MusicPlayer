#include "AudioProcessCoordinator.h"
#include "./AudioDefs.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include "otherUtils.hpp"
#include "spdlog/spdlog.h"
#include <memory>
#include <string>
#include <zmq.hpp>
AudioProcessCoordinator::AudioProcessCoordinator() {
    mAudioProcessPusher = std::make_unique<AudioProcessPusher>(mContext, pushInitOK);
    mAudioProcessSuber = std::make_unique<AudioProcessSuber>(mContext, subInitOK);
}

bool AudioProcessCoordinator::start(const juce::File& exeFile, int oscPort, juce::File cacheDir) {
    auto logger{spdlog::get(LogDllID)};

    // ── 2. 构造子进程命令行，把 zmq 地址塞进 --zmq-endpoint ──
    juce::StringArray args;
    args.add(exeFile.getFullPathName());

    // 等待端口分配完成
    pushInitOK.wait();
    args.add(AudioDefs::pushPullPort);
    args.add(juce::String(mAudioProcessPusher->getPort()));

    subInitOK.wait();
    args.add(AudioDefs::pubSubPort);
    args.add(juce::String(mAudioProcessSuber->getPort()));
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

AudioProcessCoordinator::~AudioProcessCoordinator() { stop(); }

AudioProcessPusher::AudioProcessPusher(zmq::context_t& context, juce::WaitableEvent& e)
    : juce::Thread("AudioProcessPusher"), mContext(context), portInitOK(e) {
    startThread();
}

void AudioProcessPusher::run() {
    zmq::socket_t socket(mContext, zmq::socket_type::push);
    socket.bind("tcp://127.0.0.1:*"); // 主进程bind,子进程connect
    mPushPullPort = socket.get(zmq::sockopt::last_endpoint);
    portInitOK.signal();
    spdlog::get(LogDllID)->debug("协调者：push端口为{}", mPushPullPort);
    while (!threadShouldExit()) {
    }
}

AudioProcessSuber::AudioProcessSuber(zmq::context_t& context, juce::WaitableEvent& e)
    : juce::Thread("AudioProcessSuber"), mContext(context), portInitOK(e) {
    startThread();
}

void AudioProcessSuber::run() {
    zmq::socket_t socket(mContext, zmq::socket_type::sub);
    socket.bind("tcp://127.0.0.1:*");
    mPubSubPort = socket.get(zmq::sockopt::last_endpoint);
    portInitOK.signal();
    spdlog::get(LogDllID)->debug("协调者：sub端口为{}", mPubSubPort);

    // 设置订阅过滤器
    socket.set(zmq::sockopt::subscribe, ""); // 默认接收所有消息
    socket.set(zmq::sockopt::rcvtimeo, 100); // 超时时间为100ms
    while (!threadShouldExit()) {

        zmq::message_t msg;
        auto rst = socket.recv(msg, zmq::recv_flags::none); // 同步阻塞等待
        if (rst.has_value()) {
            std::string data(static_cast<char*>(msg.data()), msg.size());
            juce::var obj = juce::JSON::fromString(juce::String(data));
            if (obj.isVoid() || !obj.isObject()) {
                spdlog::get(LogDllID)->error(
                    "AudioProcessSuber接收到未知格式:{}",
                    obj.toString().toStdString()
                );
            };
            if (obj.getDynamicObject()->hasProperty(AudioDefs::errorPopupWindowMsg)) {
                auto msg =
                    obj.getDynamicObject()->getProperty(AudioDefs::errorPopupWindowMsg).toString();
            }
        }
    }
}