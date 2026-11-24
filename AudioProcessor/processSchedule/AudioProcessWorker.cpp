#include "./AudioProcessWorker.hpp"
#include "AudioProcessWorker.hpp"
#include "Macro/SongInfoMacro.hpp"
#include "Macro/audioMacro.hpp"
#include "Macro/sliderParam.hpp"
#include "Model/PlayInfo.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "Utils/otherUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "juce_events/juce_events.h"
#include <spdlog/spdlog.h>
#include <zmq.hpp>

AudioProcessorPuller::AudioProcessorPuller(zmq::context_t& ctx, std::string pushPullPort)
    : juce::Thread("AudioProcessorPuller"), context(ctx), mPushPullPort(pushPullPort) {
    // OtherUtils::writeEmergencyLog("AudioProcessorPuller构造函数执行完成");
}
void AudioProcessorPuller::run() {
    zmq::socket_t pullSocket(context, zmq::socket_type::pull);
    pullSocket.connect(mPushPullPort); // 连接到 UI 的 PUSH 端
    pullSocket.set(zmq::sockopt::rcvtimeo, 500);
    // OtherUtils::writeEmergencyLog("puller开转");

    while (!threadShouldExit()) {
        zmq::message_t msg;
        // 纯阻塞等待，完全不消耗 CPU
        auto res = pullSocket.recv(msg, zmq::recv_flags::none); // 无限期阻塞
        if (res) {

            std::string command(static_cast<const char*>(msg.data()), msg.size());
            // OtherUtils::writeEmergencyLog("puller接收到了消息" + command);
            spdlog::get(LogAudioID)->debug("音频进程puller收到pusher的命令:{}", command);
            // 在这里解析指令，比如通知 AudioProcessor 加载预设
            Yvar jsonObj = Yvar::parse(command);
            if (!jsonObj.isObject()) {
                spdlog::get(LogAudioID)->debug("puller接收到未知指令：{}", command);
            } else {
                // 频谱共享内存：Coordinator 把名字和大小发过来
                if (jsonObj.has(AudioMacro::spectrumShm)) {
                    const Yvar shmObj = jsonObj.read(AudioMacro::spectrumShm);
                    const auto name = shmObj.read(AudioMacro::spectrumName).toString();
                    const auto bytes =
                        static_cast<size_t>(shmObj.read(AudioMacro::spectrumBytes).toInt64());
                    if (onSpectrumAttach) onSpectrumAttach(name, bytes);
                    continue;
                }
                // 频谱开/关：播放页打开时才让分析线程工作
                if (jsonObj.has(AudioMacro::spectrumEnable)) {
                    const bool enabled = jsonObj.read(AudioMacro::spectrumEnable).toBool();
                    if (onSpectrumEnabled) onSpectrumEnabled(enabled);
                    continue;
                }
                // 全局效果器链：整链重建（实例化插件，会比较慢，但这是在 puller 线程）
                if (jsonObj.has(AudioMacro::pluginChain)) {
                    if (onPluginChainConfig) onPluginChainConfig(jsonObj.read(AudioMacro::pluginChain));
                    continue;
                }
                if (jsonObj.has(AudioMacro::pluginChainBypassAll)) {
                    const bool bypassAll =
                        jsonObj.read(AudioMacro::pluginChainBypassAll).toBool();
                    if (onPluginChainBypassAll) onPluginChainBypassAll(bypassAll);
                    continue;
                }
                if (jsonObj.has(AudioMacro::pluginChainSlotBypass)) {
                    const auto inner = jsonObj.read(AudioMacro::pluginChainSlotBypass);
                    const int slot = inner.read(AudioMacro::pluginChainSlot).toInt();
                    const bool bypass = inner.read(AudioMacro::pluginChainBypass).toBool();
                    if (onPluginChainSlotBypass) onPluginChainSlotBypass(slot, bypass);
                    continue;
                }
                // 打开插件编辑器窗口（在音频进程的消息线程上建）
                if (jsonObj.has(AudioMacro::pluginChainOpenEditor)) {
                    const auto inner = jsonObj.read(AudioMacro::pluginChainOpenEditor);
                    const int slot = inner.read(AudioMacro::pluginChainSlot).toInt();
                    if (onPluginChainOpenEditor) onPluginChainOpenEditor(slot);
                    continue;
                }
                if (jsonObj.has(AudioMacro::killAudioProcess)) {
                    juce::MessageManager::callAsync([]() {
                        juce::MessageManager::getInstance()->stopDispatchLoop();
                    });
                    continue;
                }
                if (jsonObj.has(AudioMacro::play)) {
                    spdlog::get(LogAudioID)->debug("收到播放指令");
                    const Yvar playObj = jsonObj.read(AudioMacro::play);
                    double targetSeconds{playObj.read(AudioMacro::targetPTS).toDouble()};
                    auto isSeek = playObj.read(AudioMacro::isSeek).toBool();
                    auto isNewSong = playObj.read(AudioMacro::isNewSong).toBool();

                    auto info{PlayInfo::fromJson(playObj.read(AudioMacro::playInfo))};
                    // spdlog::get(LogAudioID)->debug("路径:{}", info.path);

                    if (onPlay) onPlay(info, targetSeconds, isSeek, isNewSong);
                    continue;
                }
                if (jsonObj.has(AudioMacro::pause)) {
                    spdlog::get(LogAudioID)->debug("收到暂停指令");
                    if (onPausePlay) onPausePlay();
                    continue;
                }
                if (jsonObj.has(AudioMacro::setSliderValue)) {
                    const Yvar sliderObj = jsonObj.read(AudioMacro::setSliderValue);
                    auto identifyParam{sliderObj.read(AudioMacro::sliderParam).toString()};

                    const Yvar sliderValueVar = sliderObj.read(AudioMacro::sliderValue);

                    if (identifyParam == SliderParam::masterVolume) {
                        double value{sliderValueVar.toDouble()};
                        if (onMasterVolumeChange) onMasterVolumeChange(value);
                    }
                    if (identifyParam == SliderParam::pitchShifter) {
                        double value{sliderValueVar.toDouble()};
                        if (onPitchShifterChange) onPitchShifterChange(value);
                    }
                    if (identifyParam == SliderParam::speedShifter) {
                        auto value{sliderValueVar.toDouble()};
                        if (onSpeedShifterChange) onSpeedShifterChange(value);
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

    // ── 心跳 ──
    //
    // 只在这条 IPC 线程上发，**绝不能放到音频线程**：dump() 会分配内存、
    // 这里还要拿锁，音频回调里做这些就是自找爆音。
    // 代价是它只能证明"进程还活着"，证明不了"音频线程没卡死"——
    // 想测后者要另加一个"每块自增的计数器"，等真遇到再补。
    int64_t nextHeartbeatAt = juce::Time::currentTimeMillis();
    int heartbeatSeq = 0;

    auto sendHeartbeat = [&]() {
        Yvar inner = Yvar::object();
        inner.set(AudioMacro::heartbeatSeq, heartbeatSeq++);
        inner.set(AudioMacro::heartbeatTs, juce::Time::currentTimeMillis());

        Yvar envelope = Yvar::object();
        envelope.set(AudioMacro::heartbeat, inner);

        const std::string text = envelope.dump();
        zmq::message_t zmsg(text.data(), text.size());
        pubSocket.send(zmsg, zmq::send_flags::none);
    };

    while (!threadShouldExit()) {
        // 线程睡眠在此，等待被 sendMessage 唤醒；100ms 醒来一次看看该不该发心跳
        wakeUpEvent.wait(100);

        std::queue<std::string> localQueue;
        {
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

        const auto now = juce::Time::currentTimeMillis();
        if (now >= nextHeartbeatAt) {
            sendHeartbeat();
            nextHeartbeatAt = now + kHeartbeatIntervalMs;
        }
    }
}

AudioProcessWorker::AudioProcessWorker(std::string pushPullPort, std::string pubSubPort) {
    // 将 context 引用传递给线程
    receiver = std::make_unique<AudioProcessorPuller>(zmqContext, pushPullPort);
    sender = std::make_unique<AudioProcessorPuber>(zmqContext, pubSubPort);

    // 这里**不** startThread()：回调还没接好，见 start() 的注释
}

void AudioProcessWorker::start() {
    receiver->startThread();
    sender->startThread();
}

AudioProcessWorker::~AudioProcessWorker() {
    receiver->stopThread(500);
    sender->stopThread(500);
}
