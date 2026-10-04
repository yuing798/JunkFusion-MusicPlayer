#include "./AudioRingBuffer.hpp"
#include "Utils/otherUtils.hpp"
#include "juce_core/system/juce_PlatformDefs.h"

AudioRingBuffer::AudioRingBuffer(double bufferMs) : mBufferMs(bufferMs) {}

void AudioRingBuffer::prepareToPlay(int numChannels, double sampleRate) {
    mSampleRate = sampleRate;
    mNumChannels = numChannels;
    reset();
}

int AudioRingBuffer::pushAudioData(const juce::AudioBuffer<float>& data, int numSamples) {
    const int numChannels{data.getNumChannels()};

    int start1, size1, start2, size2;
    // 准备写入，fifo 会计算出环形数组折返时的两段内存区域
    fifo.prepareToWrite(numSamples, start1, size1, start2, size2);

    if (size1 > 0) {
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.copyFrom(ch, start1, data, ch, 0, size1);
    }
    if (size2 > 0) {
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.copyFrom(ch, start2, data, ch, size1, size2);
    }

    // 更新写指针
    auto totalWritten{size1 + size2};
    fifo.finishedWrite(size1 + size2);
    return totalWritten;
}
int AudioRingBuffer::popAudioData(juce::AudioBuffer<float>& destBuffer, int numSamples) {

    const int numChannels = destBuffer.getNumChannels();

    int start1, size1, start2, size2;
    // 准备读取
    fifo.prepareToRead(numSamples, start1, size1, start2, size2);

    const int totalRead = size1 + size2;

    if (size1 > 0) {
        for (int ch = 0; ch < numChannels; ++ch)
            destBuffer.copyFrom(ch, 0, buffer, ch, start1, size1);
    }
    if (size2 > 0) {
        for (int ch = 0; ch < numChannels; ++ch)
            destBuffer.copyFrom(ch, size1, buffer, ch, start2, size2);
    }

    // 更新读指针
    fifo.finishedRead(totalRead);
    return totalRead;

    // 如果发生 Underrun（解码速度跟不上播放速度），剩余部分必须填充静音以免爆音
    // if (totalRead < numSamples) {
    //     for (int ch = 0; ch < numChannels; ++ch)
    //         destBuffer.clear(ch, totalRead, numSamples - totalRead);
    // }
}

void AudioRingBuffer::reset() {
    auto totalNumSamples{mBufferMs * mSampleRate / 1000};
    buffer.setSize(mNumChannels, totalNumSamples);
    buffer.clear();
    fifo.reset();
    fifo.setTotalSize(totalNumSamples);
}