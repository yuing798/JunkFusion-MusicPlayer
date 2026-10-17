#pragma once

#include <vector>

namespace MathUtils {

    static int lookupTableSize{1024};

    // 生成正弦表;
    //  num4pi:需要从0 ~ num4pi * pi的区域的数组
    std::vector<float> generateSinTable(double num4pi);

    // 生成余弦表;
    //  num4pi:需要从0 ~ num4pi * pi的区域的数组
    std::vector<float> generateCosTable(double num4pi);

    // 线性插值
    // process:程度:0 ~ 1
    float getLinearInterpolator(const float* data, int size, float process);

    // 拉格朗日插值
    float getLagrangeInterpolator(const float* data, int size, float process);

    /**
     * @brief 将毫秒数转换为样本数
     *
     * @param ms 毫秒数
     * @param sr 采样率
     * @return int 样本数
     */
    int msToSample(float ms, float sr);

    /**
     * @brief 将样本数转换为毫秒数
     *
     * @param sample 样本数
     * @param sr 采样率
     * @return float 毫秒数
     */
    float sampleToMs(int sample, float sr);

    // 环形缓冲区避免索引越界函数
    template <typename T1> T1 getCircularBufferIndex(T1 currentIndex, int size) {
        if (currentIndex >= size) {

            while (currentIndex >= size) {
                currentIndex -= size;
            }
            return currentIndex;
        } else if (currentIndex < 0) {
            while (currentIndex < 0) {
                currentIndex += size;
            }
            return currentIndex;
        }
        return currentIndex;
    }

    inline std::vector<float> fadeInSinTable = generateSinTable(0.5);

    inline std::vector<float> fadeOutCosTable = generateCosTable(0.5);
} // namespace MathUtils