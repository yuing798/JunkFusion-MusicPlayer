#include "AudioProcessCoordinator.h"
#include "./AudioDefs.hpp"
#include "constants.h"
#include "dllManager.hpp"
#include "juce_core/juce_core.h"
#include "otherUtils.hpp"
#include "spdlog/spdlog.h"
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <zmq.hpp>
AudioProcessCoordinator::AudioProcessCoordinator() {
    mAudioProcessPusher = std::make_unique<AudioProcessPusher>(mContext, pushInitOK);
    mAudioProcessSuber = std::make_unique<AudioProcessSuber>(mContext, subInitOK);
}

bool AudioProcessCoordinator::start(const juce::File& exeFile, int oscPort, juce::File cacheDir) {
    auto logger{spdlog::get(LogDllID)};

    // ── 2. 构造子进程命令行──
    juce::StringArray args;
    args.add(exeFile.getFullPathName());

    // 等待端口分配完成
    pushInitOK.wait();
    logger->debug("成功等到push端口号{}", mAudioProcessPusher->getPort());
    args.add(AudioDefs::pushPullPort);
    args.add(juce::String(mAudioProcessPusher->getPort()));

    subInitOK.wait();
    logger->debug("成功等到sub端口号{}", mAudioProcessSuber->getPort());
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

    juce::var obj{new juce::DynamicObject()};
    obj.getDynamicObject()->setProperty(AudioDefs::killAudioProcess, "");
    auto jsonStr = juce::JSON::toString(obj).toStdString();
    Utils::writeEmergencyLog("在AudioProcessCoordinator::stop()中通知音频进程自杀");
    mAudioProcessPusher->sendMessage(jsonStr);
    mRunning = false;
}

AudioProcessCoordinator::~AudioProcessCoordinator() {
    Utils::writeEmergencyLog("准备执行AudioProcessCoordinator的析构函数");
    stop();
    juce::Thread::sleep(100);
    if (mAudioProcessPusher) {
        mAudioProcessPusher->stopThread(2000);
    }
    if (mAudioProcessSuber) {
        mAudioProcessSuber->stopThread(2000);
    }
}

AudioProcessPusher::AudioProcessPusher(zmq::context_t& context, juce::WaitableEvent& e)
    : juce::Thread("AudioProcessPusher"), mContext(context), portInitOK(e) {
    startThread();
}

void AudioProcessPusher::sendMessage(const std::string& msg) {
    std::lock_guard<std::mutex> lock(queueMutex);
    messageQueue.push(msg);
    wakeUpEvent.signal(); // 唤醒沉睡的发送线程
}

void AudioProcessPusher::run() {
    zmq::socket_t socket(mContext, zmq::socket_type::push);
    socket.bind("tcp://127.0.0.1:*"); // 主进程bind,子进程connect
    mPushPullPort = socket.get(zmq::sockopt::last_endpoint);
    portInitOK.signal(); // 跨线程供给必须需要waitableEvent,防止收不到
    spdlog::get(LogDllID)->debug("已获取协调者：push端口为{}", mPushPullPort);

    juce::Thread::sleep(100);

    while (!threadShouldExit()) {
        // 线程睡眠在此，等待被 sendMessage 唤醒。超时设为 500ms 方便退出检查
        wakeUpEvent.wait(500);

        std::queue<std::string> localQueue;
        {
            // td::lock_guard<std::mutex> 是 C++ 标准库提供的一个 RAII（资源获取即初始化）
            // 锁管理器。简单来说，它是 “自动锁”
            // std::lock_guard<Mutex>（锁守卫）：这是一个类模板。它的构造函数会调用
            // mutex.lock()，它的析构函数会调用 mutex.unlock()。
            std::lock_guard<std::mutex> lock(queueMutex);
            std::swap(localQueue, messageQueue); // 快速把队列交换出来，减少锁占用时间
        }

        while (!localQueue.empty()) {
            std::string msg = localQueue.front();
            localQueue.pop();

            zmq::message_t zmsg(msg.data(), msg.size());
            socket.send(zmsg, zmq::send_flags::none);
        }
    }
    socket.close();
}

AudioProcessPusher::~AudioProcessPusher() {
    // stopThread(2000);
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
    spdlog::get(LogDllID)->debug("已获取协调者：sub端口为{}", mPubSubPort);

    // 设置订阅过滤器
    socket.set(zmq::sockopt::subscribe, ""); // 默认接收所有消息
    socket.set(zmq::sockopt::rcvtimeo, 500); // 超时时间为500ms
    while (!threadShouldExit()) {

        zmq::message_t msg;
        auto rst = socket.recv(msg, zmq::recv_flags::none); // 同步阻塞等待

        if (rst.has_value()) {
            Utils::writeEmergencyLog("suber收到消息");
            std::string data(static_cast<char*>(msg.data()), msg.size());
            juce::var obj = juce::JSON::fromString(juce::String(data));
            if (obj.isVoid() || !obj.isObject()) {
                Utils::writeEmergencyLog(
                    "AudioProcessSuber接收到未知格式:" + obj.toString().toStdString()
                );
                spdlog::get(LogDllID)->error(
                    "AudioProcessSuber接收到未知格式:{}",
                    obj.toString().toStdString()
                );
                continue;
            };
            if (obj.getDynamicObject()->hasProperty(AudioDefs::errorPopupWindowMsg)) {
                auto receiverMsg =
                    obj.getDynamicObject()->getProperty(AudioDefs::errorPopupWindowMsg).toString();
                spdlog::get(LogDllID)->debug(
                    "sub接收到发送错误弹窗消息:{}",
                    receiverMsg.toStdString()
                );
                Utils::writeEmergencyLog("sub接收到发送错误弹窗消息");
                const char* sendMsg = receiverMsg.toRawUTF8();
                auto length{strlen(sendMsg)};
                char* cString{static_cast<char*>(malloc(length + 1))};
                if (cString) memcpy(cString, sendMsg, length + 1);

                if (dllManager::getInstance().errorSendCallback)
                    dllManager::getInstance().errorSendCallback(cString);
                continue;
            } else if (obj.getDynamicObject()->hasProperty(AudioDefs::currentPTS)) {
                double pts = obj.getDynamicObject()->getProperty(AudioDefs::currentPTS);
                if (dllManager::getInstance().currentPTSCallback)
                    dllManager::getInstance().currentPTSCallback(pts);
            }
        }
    }

    socket.close();
}

AudioProcessSuber::~AudioProcessSuber() {
    // stopThread(2000);
}