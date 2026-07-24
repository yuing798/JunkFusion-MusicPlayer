#include "./AudioRingBuffer.hpp"

void AudioRingBuffer::setSize(int numChannels,int totalNumSamples)
{
    buffer.setSize(numChannels, totalNumSamples);
    buffer.clear();
    fifo.setTotalSize(totalNumSamples);
}
void AudioRingBuffer::reset()
{
    fifo.reset();
    buffer.clear();
}
void AudioRingBuffer::pushAudioData(const juce::AudioBuffer<float>& decodedData)
{
    const int numChannels = juce::jmin(decodedData.getNumChannels(), buffer.getNumChannels());
    const int numSamples = decodedData.getNumSamples();

    int start1, size1, start2, size2;
    // 准备写入，fifo 会计算出环形数组折返时的两段内存区域
    fifo.prepareToWrite(numSamples, start1, size1, start2, size2);

    if (size1 > 0)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.copyFrom(ch, start1, decodedData, ch, 0, size1);
    }
    if (size2 > 0)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.copyFrom(ch, start2, decodedData, ch, size1, size2);
    }

    // 更新写指针
    fifo.finishedWrite(size1 + size2);
}
void AudioRingBuffer::popAudioData(juce::AudioBuffer<float>& destBuffer)
{
    const int numChannels = juce::jmin(destBuffer.getNumChannels(), buffer.getNumChannels());
    const int numSamples = destBuffer.getNumSamples();

    int start1, size1, start2, size2;
    // 准备读取
    fifo.prepareToRead(numSamples, start1, size1, start2, size2);

    const int totalRead = size1 + size2;

    if (size1 > 0)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            destBuffer.copyFrom(ch, 0, buffer, ch, start1, size1);
    }
    if (size2 > 0)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            destBuffer.copyFrom(ch, size1, buffer, ch, start2, size2);
    }

    // 更新读指针
    fifo.finishedRead(totalRead);

    // 如果发生 Underrun（解码速度跟不上播放速度），剩余部分必须填充静音以免爆音！
    if (totalRead < numSamples)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            destBuffer.clear(ch, totalRead, numSamples - totalRead);
    }
}