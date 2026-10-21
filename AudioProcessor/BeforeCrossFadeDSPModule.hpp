#include "AudioPreProcess.hpp"
#include "AudioRingBuffer.hpp"
#include "SmoothSliderControl.hpp"
#include "Stretch.hpp"
#include "Utils/constants.h"
#include "ffmpegDecoder.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "processSchedule/AudioProcessWorker.hpp"
#include <atomic>
#include <functional>
#include <string>

// 交叉淡化区前面的模块都需要两条链路
class BeforeCrossFadeDSPModule {
private:
    AudioProcessWorker& mWorker;
    SmoothSliderControl& mSmoothSlider;
    FFmpegDecoder mDecoder;
    AudioRingBuffer mRingBuffer1{1000};
    // AudioPreProcess mPreProcess;
    Stretch mStretch;
    // 两个fifo在内部自行维护

    // 变速时，需要两个fifo(ffmpeg-->preProcess)和(-->signalSmith)共同作用，而在两个fifo之间的缓冲区大小必须是
    // 声卡需要的缓冲区大小乘以倍速率
    juce::AudioBuffer<float> mSpeedShifterBuffer;
    std::atomic_bool isFullMute{false};

    const float kMaxSpeedShiftValue{2.0f};

    /**
     * @brief 从第一个fifo中取出数据并在不足的地方补上0
     *
     * @param buffer
     * @param index
     * @param numSamples
     */
    void popAudioDataAndAddZero(juce::AudioBuffer<float>& buffer, int numSamples);

public:
    BeforeCrossFadeDSPModule(AudioProcessWorker& worker, SmoothSliderControl& smoothSlider);
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples);
    void prepareToPlay(float sampleRate, juce::AudioChannelSet channelSet, int maxBufferCapacity);

    std::function<void(void)> onRequestPlayNextSong;

    bool isDecoderRunning() { return mDecoder.isThreadRunning(); }

    /**
     * @brief 开启新的解码线程
     *
     * @param songPath
     * @param targetSeconds
     */
    void beginNewDecoderThread(std::string songPath, double targetSeconds);
    void continueThread(); // 继续出水
    void pauseThread();    // 暂停出水
    void reset();          // 重置线程

    DONT_COPY_AND_MOVE(BeforeCrossFadeDSPModule)
};