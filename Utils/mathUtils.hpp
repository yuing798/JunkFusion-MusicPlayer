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
} // namespace MathUtils