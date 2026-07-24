#include "juce_audio_basics/juce_audio_basics.h"
class AudioRingBuffer
{
public:
    AudioRingBuffer() = default;

    // 初始化：分配足够的空间，通常建议为 1~2 秒的音频长度
    void setSize(int numChannels, int totalNumSamples);

    // 重置状态（例如切歌或 Seek 时调用）
    void reset();

    // 获取当前缓冲中可读取的采样数,noexcept是绝对不抛出异常的意思
    int getNumReady() const noexcept { return fifo.getNumReady(); }

    // 获取当前缓冲中还可以写入的空间
    int getFreeSpace() const noexcept { return fifo.getFreeSpace(); }

    // 生产者：FFmpeg 线程调用此函数写入解码并重采样后的 PCM 数据
    // 注意：假设 decodedData 已经是与 JUCE 匹配的 planar float 格式
    void pushAudioData(const juce::AudioBuffer<float>& decodedData);

    // 消费者：JUCE 的 processBlock 调用此函数捞出数据播放
    void popAudioData(juce::AudioBuffer<float>& destBuffer);

private:
    juce::AbstractFifo fifo{0};
    juce::AudioBuffer<float> buffer;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRingBuffer)
};