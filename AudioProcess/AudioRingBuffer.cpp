#include "./AudioRingBuffer.hpp"
#include "juce_core/system/juce_PlatformDefs.h"
#include "otherUtils.hpp"

AudioRingBuffer::AudioRingBuffer(double bufferMs) : mBufferMs(bufferMs) {}

void AudioRingBuffer::prepareToPlay(int numChannels, double sampleRate) {
    mSampleRate = sampleRate;
    mNumChannels = numChannels;
    reset();
}

void AudioRingBuffer::pushAudioData(const juce::AudioBuffer<float>& data) {
    // Utils::writeEmergencyLog("ffmpeg和processBlock中转站推入一帧数据");
    jassert(data.getNumChannels() == buffer.getNumChannels());

    const int numSamples = data.getNumSamples();
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
    fifo.finishedWrite(size1 + size2);
    // Utils::writeEmergencyLog("ffmpeg和processBlock中转站数据推入完毕");
}
void AudioRingBuffer::popAudioData(juce::AudioBuffer<float>& destBuffer) {
    // Utils::writeEmergencyLog("ffmpeg和processBlock中转站推出一帧数据");
    jassert(destBuffer.getNumChannels() == buffer.getNumChannels());
    const int numChannels = destBuffer.getNumChannels();
    const int numSamples = destBuffer.getNumSamples();

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

    // 如果发生 Underrun（解码速度跟不上播放速度），剩余部分必须填充静音以免爆音
    if (totalRead < numSamples) {
        for (int ch = 0; ch < numChannels; ++ch)
            destBuffer.clear(ch, totalRead, numSamples - totalRead);
    }
    // Utils::writeEmergencyLog("ffmpeg和processBlock中转站推出数据完毕");
}

// void AudioRingBuffer::clip2Smooth(double smoothMs) {
//     int smoothSamples{static_cast<int>(smoothMs * mSampleRate / 1000.0f) + 1}; // 向上取整
//     // 获取当前 RingBuffer 中所有【已写入但还没被声卡读走】的数据量
//     int numReady = fifo.getNumReady();
//     if (numReady <= 0) return; // 没有数据，无需处理

//     int start1, size1, start2, size2;

//     // 2. 核心技巧：用 prepareToRead “偷看”有效数据的物理位置
//     fifo.prepareToRead(numReady, start1, size1, start2, size2);

//     const int numChannels = buffer.getNumChannels();

//     // 3. 遍历当前所有还没播放的有效数据
//     for (int i = 0; i < numReady; ++i) {
//         float gain = 0.0f;

//         // 如果在平滑窗口内，计算线性淡出的增益 (1.0 渐变到 0.0)
//         if (i < smoothSamples) {
//             gain = 1.0f - (static_cast<float>(i) / static_cast<float>(smoothSamples));
//         }
//         // 如果超过了 smoothSamples，gain 就是 0.0f，相当于清零

//         // 计算当前数据在底层数组中的真实物理索引（处理折返）
//         int physicalIndex = (i < size1) ? (start1 + i) : (start2 + (i - size1));

//         // 4. 将计算好的增益应用到缓冲区
//         for (int ch = 0; ch < numChannels; ++ch) {
//             float originalSample = buffer.getSample(ch, physicalIndex);
//             buffer.setSample(ch, physicalIndex, originalSample * gain);
//         }
//     }
// }

void AudioRingBuffer::reset() {
    auto totalNumSamples{mBufferMs * mSampleRate / 1000};
    buffer.setSize(mNumChannels, totalNumSamples);
    buffer.clear();
    fifo.reset();
    fifo.setTotalSize(totalNumSamples);
}