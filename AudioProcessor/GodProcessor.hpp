#pragma once

#include "./DSPModule/BeforeCrossFadeDSPModule.hpp"
#include "./DSPModule/CrossFadeArea.hpp"
#include "./DSPModule/PlayCount.hpp"
#include "./DSPModule/PlayPauseControl.hpp"
#include "./DSPModule/ProgressBarWatch.hpp"
#include "./DSPModule/SmoothSliderControl.hpp"
#include "DSPModule/Eq.hpp"
#include "DSPModule/SpectrumAnalyzer.hpp"
#include "DeviceManager.hpp"
#include "Model/PlayInfo.hpp"
#include "PluginHost/PluginChain.hpp"
#include "PluginHost/PluginEditorWindow.hpp"
#include "SystemAudioControl.hpp"
#include "Utils/constants.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "processSchedule/AudioProcessWorker.hpp"
#include "processSchedule/OscReceiver.hpp"
#include <array>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <spdlog/logger.h>
#include <string>

class GodProcessor : public juce::AudioProcessor {
private:
    AudioProcessWorker mWorker;
    OscReceiver mOscReceiver;

    float mSampleRate{44100.0f};
    int mNumChannels{2};

    PlayInfo mPlayInfo;

    std::unique_ptr<SystemAudioControl> mSystemAudioControl; // 系统级音频控制

    ProgressBarWatch mProgressBar;
    PlayCount mPlayCount;
    PlayPauseControl mPlayPauseControl;
    CrossFadeArea mCrossFade;
    SmoothSliderControl mSmooth;
    Eq mEq; // 均衡器
    SpectrumAnalyzer mSpectrum; // 实时频谱（写共享内存，前端读）

    // ── 全局效果器链（交叉淡化之后、EQ 之前）──
    // 实例化插件会 LoadLibrary，只能在 puller 线程做；音频线程只读 mActiveChain
    PluginHost::PluginInstanceFactory mPluginFactory;
    std::unique_ptr<PluginHost::PluginChain> mPluginChain; // 拥有者
    std::atomic<PluginHost::PluginChain*> mActiveChain{nullptr}; // 音频线程读这个

    // 插件编辑器窗口：**只被消息线程碰**（open/close 都走 MessageManager）
    std::map<int, std::unique_ptr<PluginHost::PluginEditorWindow>> mEditorWindows;

    /** @brief 重建整链（puller 线程）：建新链 → 换指针 → 等两块 → 销毁旧链 */
    void applyPluginChain(const Yvar& config);

    /** @brief 把当前链状态回传给 Coordinator（前端据此显示每个槽加载成功没有） */
    void sendPluginChainState();

    /**
     * @brief 打开某个槽的插件编辑器窗口（任意线程）
     *
     * 内部转成 `MessageManager::callAsync`：编辑器是 GUI 对象，只能在消息线程造。
     * 同一个槽只开一个窗口，再点一次 = 把藏起来的窗口 show 回来。
     */
    void openPluginEditor(int slot);

    /**
     * @brief 关掉所有编辑器窗口（**只能在非消息线程调**，比如 puller）
     *
     * 换链之前必须调：编辑器还拿着即将析构的 AudioProcessor 指针，
     * 不关掉的话下次 repaint 就炸。这里阻塞到消息线程真的删完（最多 2 秒）。
     */
    void closeAllPluginEditors();

    struct BeforeCrossFadeDSPModuleAndBuffer {
        juce::AudioBuffer<float> buffer;
        BeforeCrossFadeDSPModule module;

        BeforeCrossFadeDSPModuleAndBuffer(AudioProcessWorker& worker, SmoothSliderControl& smooth)
            : module(worker, smooth) {}
    };
    std::array<BeforeCrossFadeDSPModuleAndBuffer, 2> mDoubleDecks;

    // DeviceManager mDeviceManager; // 设备管理要在所有音频处理子类之前就析构

    std::unique_ptr<DeviceManager> mDeviceManager;

    /**
     * @brief 播放上一首歌曲或者下一首歌曲的回调请求
     *
     * @param nextOrPrevious true为下一首，false为上一首
     */
    void playNextOrPreviousSong(bool nextOrPrevious);

    /**
     * @brief Set the Ordinary Play
     * object,设置普通的播放状态，也就是不走交叉淡化那一条路径，只是普通的淡入处理
     *
     */
    void continuePlay(double targetSeconds);
    void pausePlay();
    void play(PlayInfo info, double targetSeconds, bool isSeek, bool isNewSong);

    /** @brief 把一条 EQ 报文分派给 DSP（band / master / all） */
    void applyEqMessage(const EqWire::Message& message);

public:
    GodProcessor(
        std::string pushPullPort,
        std::string pubSubPort,
        juce::File cacheDir,
        int oscPort
    );
    ~GodProcessor();
    const juce::String getName() const override { return "JunkFusion"; }
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {
        juce::ignoreUnused(index);
        return;
    }
    const juce::String getProgramName(int index) override {
        juce::ignoreUnused(index);
        return "JunkFusion";
    }
    void changeProgramName(int index, const juce::String& newName) override {
        juce::ignoreUnused(index);
        juce::ignoreUnused(newName);
        return;
    }
    void setStateInformation(const void* data, int sizeInBytes) override;
    void getStateInformation(juce::MemoryBlock& destData) override;
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    DONT_COPY_AND_MOVE(GodProcessor)
};