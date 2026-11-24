#include "./GodProcessor.hpp"
#include "DSPModule/CrossFadeArea.hpp"
#include "DSPModule/PlayCount.hpp"
#include "DSPModule/PlayPauseControl.hpp"
#include "DSPModule/ProgressBarWatch.hpp"
#include "DSPModule/SmoothSliderControl.hpp"
#include "DeviceManager.hpp"
#include "Macro/audioMacro.hpp"
#include "SystemAudioControl.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "Utils/convertUtils.hpp"
#include "Utils/otherUtils.hpp"
#include "WindowsSystemAudioControl.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "processSchedule/AudioProcessWorker.hpp"
#include "processSchedule/OscReceiver.hpp"
#include <cstddef>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>

#ifdef __linux__
    #include "SystemAudioControl/LinuxSystemAudioControl.hpp"
#elif defined(__APPLE__)
    #include "SystemAudioControl/MacosSystemAudioControl.hpp"
#endif
GodProcessor::GodProcessor(
    std::string pushPullPort,
    std::string pubSubPort,
    juce::File cacheDir,
    int oscPort
)
    : mWorker(pushPullPort, pubSubPort), mOscReceiver(oscPort), mProgressBar(mWorker),
      mPlayCount(mWorker), mSmooth(mOscReceiver, mWorker),
      mDoubleDecks{
          {BeforeCrossFadeDSPModuleAndBuffer{mWorker, mSmooth},
           BeforeCrossFadeDSPModuleAndBuffer{mWorker, mSmooth}}
      } {

    // 设置系统音频管理类
    {
#ifdef _WIN32
        mSystemAudioControl = std::make_unique<WindowsSystemAudioControl>();
#elif defined(__linux__)
        mSystemAudioControl = std::make_unique<LinuxSystemAudioControl>();
#elif defined(__APPLE__)
        mSystemAudioControl = std::make_unique<MacosSystemAudioControl>();
#endif

        mSystemAudioControl->onLog = [](SystemAudioControl::LogRank rank, std::string str) {
            auto ptr{spdlog::get(LogAudioID).get()};
            if (rank == SystemAudioControl::LogRank::Debug) {
                ptr->debug(str);
            } else if (rank == SystemAudioControl::LogRank::Info) {
                ptr->info(str);
            } else if (rank == SystemAudioControl::LogRank::Error) {
                ptr->error(str);
            }
        };

        if (mSystemAudioControl->initialize()) {
            spdlog::get(LogAudioID)->debug("系统音频同步初始化成功");
        }

        mSystemAudioControl->onPlay = [this]() {
            play(mPlayInfo, mProgressBar.getCurSeconds(), 0, 0);
            mSystemAudioControl->updatePlaybackState(SystemAudioControl::PlaybackState::Play);
            Yvar obj = Yvar::object();
            obj.set(AudioMacro::playStateSync, 1);
            mWorker.sender->sendMessage(obj.dump());
        };
        mSystemAudioControl->onPause = [this]() {
            pausePlay();
            mSystemAudioControl->updatePlaybackState(SystemAudioControl::PlaybackState::Pause);
            Yvar obj = Yvar::object();
            obj.set(AudioMacro::playStateSync, 0);
            mWorker.sender->sendMessage(obj.dump());
        };

        mSystemAudioControl->onNext = [this] { playNextOrPreviousSong(true); };
        mSystemAudioControl->onPrevious = [this] { playNextOrPreviousSong(false); };

        mSystemAudioControl->onFastForward = [this](int value) {
            auto targetSeconds{mProgressBar.getCurSeconds() + (float)value};
            if (targetSeconds >= mPlayInfo.duration) {
                playNextOrPreviousSong(true);
            } else {
                play(mPlayInfo, targetSeconds, 0, 0);
            }
        };
        mSystemAudioControl->onRewind = [this](int value) {
            auto targetSeconds{mProgressBar.getCurSeconds() - (float)(value)};
            if (targetSeconds < 0.0) {
                playNextOrPreviousSong(false);
            } else {
                play(mPlayInfo, targetSeconds, 0, 0);
            }
        };
    }

    // 停止播放，不代表现在开始完全静音
    mWorker.receiver->onPausePlay = [this]() { pausePlay(); };


    // ── 频谱：共享内存由 Coordinator 创建，这里只负责接上并开关 ──
    mWorker.receiver->onSpectrumAttach = [this](const std::string& name, size_t bytes) {
        mSpectrum.attachSharedMemory(name, bytes);
    };
    mWorker.receiver->onSpectrumEnabled = [this](bool enabled) { mSpectrum.setEnabled(enabled); };

    // ── 全局效果器链 ──
    // 整链重建会实例化插件（LoadLibrary，几十~几百毫秒），只可能在 puller 线程做；
    // 音频线程只通过 mActiveChain 读一个指针，见 processBlock
    mWorker.receiver->onPluginChainConfig = [this](const Yvar& config) {
        applyPluginChain(config);
    };
    mWorker.receiver->onPluginChainBypassAll = [this](bool bypassAll) {
        if (auto* chain = mActiveChain.load(std::memory_order_acquire)) {
            chain->setBypassAll(bypassAll);
        }
    };
    mWorker.receiver->onPluginChainSlotBypass = [this](int slot, bool bypass) {
        if (auto* chain = mActiveChain.load(std::memory_order_acquire)) {
            chain->setSlotBypass(slot, bypass);
        }
    };
    mWorker.receiver->onPluginChainOpenEditor = [this](int slot) { openPluginEditor(slot); };

    // 完全静音的切换态
    mPlayPauseControl.onPauseOver = [this]() {
        mPlayCount.pauseCount();
        mProgressBar.pauseProgressBar();
        mDoubleDecks[mCrossFade.getMainDeckIndex()].module.pauseThread();
    };

    mCrossFade.onCrossFadeEnd = [this] {
        // ⚠️ 这个回调在**音频线程**上（CrossFadeArea::processBlock 里），
        // 所以只能做原子操作：暂停出水就够了，真正的停解码线程 + 清缓冲
        // 留给这个 deck 下次被复用时的 beginNewDecoderThread()。
        // 以前这里直接调 module.reset()，会在音频线程里等解码线程退出（最多 200ms）
        // 并且并发改 fifo —— 每次切歌结束都埋一次卡顿/崩溃的雷。
        mDoubleDecks[!mCrossFade.getMainDeckIndex()].module.pauseThread();
    };

    // 播放
    mWorker.receiver->onPlay =
        [this](PlayInfo info, double targetSeconds, bool isSeek, bool isNewSong) {
            // OtherUtils::writeEmergencyLog("收到播放指令");
            play(std::move(info), targetSeconds, isSeek, isNewSong);
            // OtherUtils::writeEmergencyLog("播放指令结束");
        };

    for (int i = 0; i < 2; i++) {
        mDoubleDecks[i].module.onRequestPlayNextSong = [this]() {
            playNextOrPreviousSong(true);
            mPlayCount.pauseCount();
            mProgressBar.pauseProgressBar();
        };
    }

    {
        auto configFile{cacheDir.getChildFile("JFConfig.xml")};
        mDeviceManager = std::make_unique<DeviceManager>(configFile);
        mDeviceManager->connectProcessor(this);
    }

    // ── 最后一步：启动 IPC 线程 ──
    //
    // 必须放在构造函数**最后**：此时上面所有回调都赋值完了、mSpectrum 等成员
    // 也都构造完了。提前启动的话，Coordinator 在进程一起来就发的 `spectrumShm`
    // 可能落进"线程在收消息、回调还是 null"的窗口里被静默丢掉（实测 19 次丢 5 次），
    // 那整场都没有频谱数据。详见 AudioProcessWorker::start() 的注释。
    mWorker.start();
}

void GodProcessor::playNextOrPreviousSong(bool nextOrPrevious) {
    Yvar obj = Yvar::object();
    obj.set(AudioMacro::onPlayNextOrPreviousSong, nextOrPrevious);
    mWorker.sender->sendMessage(obj.dump());
}

void GodProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    mSampleRate = (float)sampleRate;

    juce::AudioChannelSet outputLayout = getChannelLayoutOfBus(false, 0);
    mNumChannels = outputLayout.size();
    mSpectrum.prepare(sampleRate); // 频谱分析

    mProgressBar.prepareToPlay(mSampleRate);
    mPlayCount.prepareToPlay(mSampleRate);
    mSmooth.prepareToPlay(mSampleRate);
    mPlayPauseControl.prepareToPlay(mSampleRate);
    mCrossFade.prepareToPlay(mSampleRate);
    for (int i = 0; i < 2; i++) {
        // 这里只需要分配最大缓冲区数目是因为这个缓冲区只需要存储交叉淡化前最后一环的数据
        mDoubleDecks[i].buffer.setSize(mNumChannels, maximumExpectedSamplesPerBlock);
        mDoubleDecks[i]
            .module.prepareToPlay(mSampleRate, outputLayout, maximumExpectedSamplesPerBlock);
    }
}

void GodProcessor::releaseResources() {}
void GodProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(midiMessages);
    for (int i = 0; i < 2; i++) {
        mDoubleDecks[i].buffer.clear();
        mDoubleDecks[i].module.processBlock(mDoubleDecks[i].buffer, buffer.getNumSamples());
    }

    mCrossFade.processBlock(
        {&mDoubleDecks[0].buffer, &mDoubleDecks[1].buffer},
        buffer,
        buffer.getNumSamples()
    ); // 交叉淡化

    // 全局效果器链：交叉淡化之后、EQ 之前
    // 单条路径 → 不需要 PDC；块大小 = 设备块，插件不会看到变长的块。
    // 理由见 docs/audio-routing.md §4.1
    if (auto* chain = mActiveChain.load(std::memory_order_acquire)) {
        chain->process(buffer);
    }

    for (int i = 0; i < buffer.getNumSamples(); i++) {
        auto currentMasterVolume = mSmooth.masterVolume.getNextValue();
        for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
            auto ptr{buffer.getWritePointer(ch)};
            ptr[i] *= currentMasterVolume;
        }
    } // 主音量控制（不需要上下文的DSP模块建议都放到这个循环里面）
    mPlayPauseControl.processBlock(buffer, buffer.getNumSamples()); // 播放暂停
    mPlayCount.processBlock(buffer.getNumSamples());                // 播放计数
    mProgressBar.processBlock(buffer.getNumSamples());              // 进度条

    // 频谱放在最后：取的是"用户实际听到的"信号（EQ + 主音量之后）
    mSpectrum.pushSamples(buffer);
}

void GodProcessor::continuePlay(double targetSeconds) {
    if (targetSeconds == 0.0) {
        mPlayPauseControl.setPlaybackState(
            PlayPauseControl::PlayPauseState::Play,
            PlayPauseControl::BeginEndState::Begin
        );
    } else {
        mPlayPauseControl.setPlaybackState(
            PlayPauseControl::PlayPauseState::Play,
            PlayPauseControl::BeginEndState::Nor
        );
    }
}

void GodProcessor::pausePlay() {
    mSystemAudioControl->updatePlaybackState(SystemAudioControl::PlaybackState::Pause);
    mPlayPauseControl.setPlaybackState(
        PlayPauseControl::PlayPauseState::Pause,
        PlayPauseControl::BeginEndState::Nor
    );
}

void GodProcessor::play(PlayInfo info, double targetSeconds, bool isSeek, bool isNewSong) {
    // if (isCrossFade) {
    //     waitCrossFadeOver.wait(1000);
    // }

    if (isNewSong) {
        mPlayInfo = info;
        // spdlog::get(LogAudioID)
        //     ->debug("准备播放新歌:{}", info.toJson().toString());

        SystemAudioControl::MediaMetadata data{};
        data.title = info.title;
        if (info.artists.has_value()) data.artists = info.artists.value();
        mSystemAudioControl->updateMetadata(data);

        mPlayCount.setNewSong(info.path, info.duration);

        if (mDoubleDecks[mCrossFade.getMainDeckIndex()].module.isDecoderRunning() &&
            !mPlayPauseControl.getIsFullMute()) {
            // 如果是新歌且原歌正在播放，切换甲板索引,采取600ms的交叉淡化长度

            mDoubleDecks[!mCrossFade.getMainDeckIndex()].module.beginNewDecoderThread(
                info.path,
                targetSeconds
            );
            mCrossFade.setCrossFadeBegin(true); // 先开新歌再切甲板

        } else if (mDoubleDecks[mCrossFade.getMainDeckIndex()].module.isDecoderRunning() &&
                   mPlayPauseControl.getIsFullMute()) {

            mCrossFade.flipDeck();
            mDoubleDecks[mCrossFade.getMainDeckIndex()].module.beginNewDecoderThread(
                info.path,
                targetSeconds
            );
            mDoubleDecks[!mCrossFade.getMainDeckIndex()].module.reset();
            continuePlay(targetSeconds);

        } else {
            // 正常的播放
            continuePlay(targetSeconds);
            mDoubleDecks[mCrossFade.getMainDeckIndex()].module.beginNewDecoderThread(
                info.path,
                targetSeconds
            );
        }
    } else {
        mPlayCount.continueCount();
        if (isSeek) {
            if (mDoubleDecks[mCrossFade.getMainDeckIndex()].module.isDecoderRunning() &&
                !mPlayPauseControl.getIsFullMute()) {
                // 不是新歌但是跳转位置，且原歌曲正在播放，采取50ms交叉长度
                mDoubleDecks[!mCrossFade.getMainDeckIndex()].module.beginNewDecoderThread(
                    mPlayInfo.path,
                    targetSeconds
                );
                mCrossFade.setCrossFadeBegin(false);

            } else if (mDoubleDecks[mCrossFade.getMainDeckIndex()].module.isDecoderRunning() &&
                       mPlayPauseControl.getIsFullMute()) {

                mCrossFade.flipDeck();
                mDoubleDecks[mCrossFade.getMainDeckIndex()].module.beginNewDecoderThread(
                    info.path,
                    targetSeconds
                );
                mDoubleDecks[!mCrossFade.getMainDeckIndex()].module.reset();
                continuePlay(targetSeconds);

            } else {
                mDoubleDecks[mCrossFade.getMainDeckIndex()].module.beginNewDecoderThread(
                    info.path,
                    targetSeconds
                );
                continuePlay(targetSeconds);
            }
        } else {
            continuePlay(targetSeconds);
            mDoubleDecks[mCrossFade.getMainDeckIndex()].module.continueThread();
        }
    }
    mProgressBar.beginProgressBar(targetSeconds);
}

GodProcessor::~GodProcessor() {}

void GodProcessor::openPluginEditor(int slot) {
    // 编辑器是 GUI 对象：创建、显示都必须在消息线程
    juce::MessageManager::callAsync([this, slot]() {
        if (auto it = mEditorWindows.find(slot); it != mEditorWindows.end()) {
            // 已经开过了：显示出来，并且重新"抬一次"最上层
            // （只 setVisible 的话窗口会留在上次被压到的那一层）
            it->second->setVisible(true);
            it->second->bringToFrontOnce();
            return;
        }

        auto* chain = mActiveChain.load(std::memory_order_acquire);
        if (chain == nullptr) return;

        auto* processor = chain->processorAt(slot);
        if (processor == nullptr) {
            if (auto logger = spdlog::get(LogAudioID))
                logger->warn("[PluginEditor] 槽 {} 没有加载插件", slot);
            return;
        }

        auto window =
            std::make_unique<PluginHost::PluginEditorWindow>(*processor, processor->getName());
        if (!window->isValid()) return;
        mEditorWindows[slot] = std::move(window);
    });
}

void GodProcessor::closeAllPluginEditors() {
    // 用 shared_ptr 兜住超时后 lambda 迟到的风险（栈上的 WaitableEvent 早就没了）
    auto finished = std::make_shared<juce::WaitableEvent>(false);

    juce::MessageManager::callAsync([this, finished]() {
        mEditorWindows.clear();
        finished->signal();
    });

    // 换链不能等太久；窗口一般 instantly 就删完了
    finished->wait(2000);
}

void GodProcessor::applyPluginChain(const Yvar& config) {
    auto logger{spdlog::get(LogAudioID)};

    // 0. 先关编辑器：它们还拿着即将析构的 AudioProcessor 指针
    closeAllPluginEditors();

    const int channels = mNumChannels > 0 ? mNumChannels : 2;
    const double sampleRate = mSampleRate > 0.0f ? static_cast<double>(mSampleRate) : 44100.0;
    const int blockSize = getBlockSize() > 0 ? getBlockSize() : 512;

    if (logger) logger->info("[PluginChain] 开始重建（{} 槽，{}Hz/{}）", 8, sampleRate, blockSize);

    // 1. 在**离线**对象上把整条链建好：实例化插件、prepare、恢复状态
    auto newChain = std::make_unique<PluginHost::PluginChain>();
    newChain->applyConfig(config, mPluginFactory, sampleRate, blockSize, channels);

    // 2. 原子换指针：音频线程从下一块开始用新链
    mActiveChain.store(newChain.get(), std::memory_order_release);

    // 3. 等两个块（441@44.1k ≈ 10ms/块），确保没有音频线程还在旧链里
    juce::Thread::sleep(50);

    // 4. 销毁旧链（赋值时旧对象析构）。
    //    插件析构可能挂死（实验台实测 Dexed），但这里在 puller 线程：
    //    只会拖慢后续指令处理，不会卡音频线程。
    //    TODO(异步销毁): 真遇到挂死的插件，要改成丢后台线程 + 超时后宁可泄漏。
    mPluginChain = std::move(newChain);

    sendPluginChainState();

    if (logger) {
        logger->info(
            "[PluginChain] 重建完成：loaded={} 总延迟={} 采样",
            mActiveChain.load()->isAnyLoaded(),
            mActiveChain.load()->totalLatencySamples()
        );
    }
}

void GodProcessor::sendPluginChainState() {
    Yvar envelope = Yvar::object();

    if (auto* chain = mActiveChain.load(std::memory_order_acquire)) {
        envelope.set(AudioMacro::pluginChainState, chain->statusJson());
    } else {
        envelope.set(AudioMacro::pluginChainState, Yvar{});
    }

    mWorker.sender->sendMessage(envelope.dump());
}


void GodProcessor::setStateInformation(const void* data, int sizeInBytes) {
    juce::ignoreUnused(data);
    juce::ignoreUnused(sizeInBytes);
}
void GodProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::ignoreUnused(destData);
}

bool GodProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    auto outSet = layouts.getMainOutputChannelSet();
    // 只要输出不是禁用状态，且通道数在 1~256 之间，就允许声卡进行绑定
    return !outSet.isDisabled() && outSet.size() > 0 && outSet.size() <= 256;
}