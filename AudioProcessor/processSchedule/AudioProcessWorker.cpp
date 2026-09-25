#include "./AudioProcessWorker.hpp"
#include "AudioProcessWorker.hpp"
#include "Macro/audioMacro.hpp"
#include "Macro/sliderParam.hpp"
#include "Utils/constants.h"
#include "Utils/otherUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "juce_events/juce_events.h"
#include <spdlog/spdlog.h>
#include <zmq.hpp>

AudioProcessorPuller::AudioProcessorPuller(zmq::context_t& ctx, std::string pushPullPort)
    : juce::Thread("AudioProcessorPuller"), context(ctx), mPushPullPort(pushPullPort) {
    OtherUtils::writeEmergencyLog("AudioProcessorPuller构造函数执行完成");
}

void AudioProcessorPuller::run() {
    zmq::socket_t pullSocket(context, zmq::socket_type::pull);
    pullSocket.connect(mPushPullPort); // 连接到 UI 的 PUSH 端
    pullSocket.set(zmq::sockopt::rcvtimeo, 500);
    OtherUtils::writeEmergencyLog("puller开转");

    while (!threadShouldExit()) {
        zmq::message_t msg;
        // 纯阻塞等待，完全不消耗 CPU
        auto res = pullSocket.recv(msg, zmq::recv_flags::none); // 无限期阻塞
        if (res) {

            std::string command(static_cast<const char*>(msg.data()), msg.size());
            OtherUtils::writeEmergencyLog("puller接收到了消息" + command);
            spdlog::get(LogAudioID)->debug("音频进程pusller收到pusher的命令:{}", command);
            // 在这里解析指令，比如通知 AudioProcessor 加载预设
            auto jsonStr{juce::JSON::fromString(juce::String(command))};
            if (jsonStr.isVoid() || !jsonStr.isObject()) {
                OtherUtils::writeEmergencyLog("puller接收到未知指令");
                spdlog::get(LogAudioID)->debug("puller接收到未知指令：{}", command);
            } else {
                auto obj = jsonStr.getDynamicObject();
                jassert(obj);
                if (obj->hasProperty(AudioMacro::killAudioProcess)) {
                    juce::MessageManager::callAsync([]() {
                        juce::MessageManager::getInstance()->stopDispatchLoop();
                    });
                    continue;
                }
                if (obj->hasProperty(AudioMacro::play)) {
                    spdlog::get(LogAudioID)->debug("收到播放指令");
                    juce::var playInfo{obj->getProperty(AudioMacro::play)};
                    if (auto playInfoObj = playInfo.getDynamicObject()) {
                        auto songPath{
                            playInfoObj->getProperty(AudioMacro::songPath).toString().toStdString()
                        };
                        double targetPTS{playInfoObj->getProperty(AudioMacro::targetPTS)};

                        if (onPlay) onPlay(songPath, targetPTS);
                        continue;
                    }
                }
                if (obj->hasProperty(AudioMacro::pause)) {
                    spdlog::get(LogAudioID)->debug("收到暂停指令");
                    if (onPausePlay) onPausePlay();
                    continue;
                }
                if (obj->hasProperty(AudioMacro::setSliderValue)) {
                    auto ptr{obj->getProperty(AudioMacro::setSliderValue).getDynamicObject()};
                    jassert(ptr);
                    auto identifyParam{ptr->getProperty(AudioMacro::sliderParam).toString()};
                    if (identifyParam == SliderParam::masterVolume) {
                        float value{ptr->getProperty(AudioMacro::sliderValue)};
                        if (onMasterVolumeChange) onMasterVolumeChange(value);
                    }
                }
            }
        }
    }
}

AudioProcessorPuber::AudioProcessorPuber(zmq::context_t& ctx, std::string pubSubPort)
    : juce::Thread("AudioProcessorPuber"), context(ctx), mPubSubPort(pubSubPort) {}

void AudioProcessorPuber::sendMessage(const std::string& msg) {
    std::lock_guard<std::mutex> lock(queueMutex);
    messageQueue.push(msg);
    wakeUpEvent.signal(); // 唤醒沉睡的发送线程
}

void AudioProcessorPuber::run() {
    // Socket 在 run() 内创建
    zmq::socket_t pubSocket(context, zmq::socket_type::pub);
    pubSocket.connect(mPubSubPort); // 连接到 UI 的 SUB 端

    // 稍微等待 100ms，防止“慢连接综合征”导致启动第一条消息丢失
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
            // OtherUtils::writeEmergencyLog("[AudioProcess]准备发射命令:" + msg);

            zmq::message_t zmsg(msg.data(), msg.size());
            pubSocket.send(zmsg, zmq::send_flags::none);
        }
    }
}

void AudioProcessorPuber::timerCallback() {}

AudioProcessWorker::AudioProcessWorker(std::string pushPullPort, std::string pubSubPort) {
    OtherUtils::writeEmergencyLog("开始执行worker的构造函数");
    // 将 context 引用传递给线程
    receiver = std::make_unique<AudioProcessorPuller>(zmqContext, pushPullPort);
    sender = std::make_unique<AudioProcessorPuber>(zmqContext, pubSubPort);

    receiver->startThread();
    sender->startThread();
}

AudioProcessWorker::~AudioProcessWorker() {
    receiver->stopThread(2000);
    sender->stopThread(2000);
}