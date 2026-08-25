#pragma once

// #include "DartApi/dart_api_dl.h"
// #include "DartApi/dart_native_api.h"
#include "juce_core/juce_core.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

namespace DllUtils {
    // 把const char*转换为对象
    juce::var uint8t2Object(const char* str);

    // 把对象转换为const char*
    const char* object2Uint8t(juce::var obj);

    juce::var charPtr2object(const char* ptr);

    void sendMessage2AudioProcess(juce::var obj);

    // 将cpp字符串转为字符串指针的形式并拷贝一份到堆上面
    const char* copyStringOnHeap(juce::String& str);
    // 将cpp字符串转为字符串指针的形式并拷贝一份到堆上面
    const char* copyStringOnHeap(std::string& str);

    template <typename T> T* copyArrayOnHeap(std::vector<T> buffer) {
        size_t byteSize{buffer.size() * sizeof(T)};
        void* heapData{malloc(byteSize)};
        if (heapData) memcpy(heapData, buffer.data(), byteSize);
        return heapData;
    }
} // namespace DllUtils