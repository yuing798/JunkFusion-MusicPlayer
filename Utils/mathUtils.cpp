#include "./mathUtils.hpp"
#include "juce_core/juce_core.h"

std::vector<float> MathUtils::generateSinTable(double num4pi) {
    std::vector<float> tableBuffer;

    tableBuffer.resize(MathUtils::lookupTableSize);
    // tableBuffer.clear();

    for (int index = 0; index < MathUtils::lookupTableSize; index++) {

        float phase = (static_cast<float>(index) / MathUtils::lookupTableSize) * num4pi *
                      juce::MathConstants<float>::pi;
        tableBuffer[index] = std::sin(phase);
    }
    return tableBuffer;
}

std::vector<float> MathUtils::generateCosTable(double num4pi) {
    std::vector<float> tableBuffer;

    tableBuffer.resize(MathUtils::lookupTableSize);
    // tableBuffer.clear();

    for (int index = 0; index < MathUtils::lookupTableSize; index++) {

        float phase = (static_cast<float>(index) / MathUtils::lookupTableSize) * num4pi *
                      juce::MathConstants<float>::pi;
        tableBuffer[index] = std::cos(phase);
    }
    return tableBuffer;
}

float MathUtils::getLinearInterpolator(const float* data, int size, float process) {
    int index1 = static_cast<int>(process * size);
    int index2 = getCircularBufferIndex(index1 + 1, size);
    float fraction = process * size - index1;

    return (1.0f - fraction) * data[index1] + fraction * data[index2];
}

float MathUtils::getLagrangeInterpolator(const float* data, int size, float process) {
    // 原理：用一个 N 阶多项式穿过 N+1 个最近的样本点，然后取多项式在所需延迟位置的值。
    // 奇数阶的拉格朗日插值（如 3 阶，用 4 个样本）相当于一个对称的 FIR 滤波器，其系数是分数延迟 d
    // 的简单多项式。
    int intIndex = static_cast<int>(process * size);
    float distanceIndex2IntIndex = process * size - intIndex;
    float nagetive1 = data[getCircularBufferIndex(intIndex - 1, size)];
    float intSample = data[intIndex];
    float positive1 = data[getCircularBufferIndex(intIndex + 1, size)];
    float positive2 = data[getCircularBufferIndex(intIndex + 2, size)];

    float outputSample = 0.0f;
    float distanceIndex2IntIndexMinus1 = distanceIndex2IntIndex - 1;
    float distanceIndex2IntIndexMinus2 = distanceIndex2IntIndex - 2;
    float distanceIndex2IntIndexPlus1 = distanceIndex2IntIndex + 1;

    outputSample -= distanceIndex2IntIndex * distanceIndex2IntIndexMinus1 *
                    distanceIndex2IntIndexMinus2 * 0.16667f * nagetive1;
    outputSample += distanceIndex2IntIndexPlus1 * distanceIndex2IntIndexMinus1 *
                    distanceIndex2IntIndexMinus2 * 0.5f * intSample;
    outputSample -= distanceIndex2IntIndexPlus1 * distanceIndex2IntIndex *
                    distanceIndex2IntIndexMinus2 * 0.5f * positive1;
    outputSample += distanceIndex2IntIndexPlus1 * distanceIndex2IntIndex *
                    distanceIndex2IntIndexMinus1 * 0.16667f * positive2;
    return outputSample;
}