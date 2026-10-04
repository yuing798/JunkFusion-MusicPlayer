#pragma once
#include "juce_audio_basics/juce_audio_basics.h"

// 中转站类
class AudioRingBuffer {
public:
    // 传参：这个缓冲区设置为多少毫秒
    AudioRingBuffer(double bufferMs);

    // 传参：通道数，采样率
    void prepareToPlay(int, double);

    // 获取当前缓冲中可读取的采样数,noexcept是绝对不抛出异常的意思
    int getNumReady() const noexcept { return fifo.getNumReady(); }

    // 获取当前缓冲中还可以写入的空间
    int getFreeSpace() const noexcept { return fifo.getFreeSpace(); }

    /**
     * @brief 生产者:推入音频数据
     *
     * @param originData 原始数据
     * @param numSamples 想要推入的量
     * @return int 实际推入的数据量
     */
    int pushAudioData(const juce::AudioBuffer<float>& originData, int numSamples);

    /**
     * @brief 消费者:取出音频数据
     *
     * @param destBuffer 目标容器
     * @param numSamples 想要取出来的样本点个数
     * @return int 实际取出的样本点个数
     */
    int popAudioData(juce::AudioBuffer<float>& destBuffer, int numSamples);

    void reset();

    // void clip2Smooth(double smoothMs); // 将除了用于平滑处理外的部分都置为0,参数，平滑区的长度

private:
    juce::AbstractFifo fifo{1};
    juce::AudioBuffer<float> buffer;
    double mSampleRate{44100.0};
    int mNumChannels{2};
    double mBufferMs{0.0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRingBuffer)
};