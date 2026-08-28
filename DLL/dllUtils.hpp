#pragma once
#include "juce_core/juce_core.h"
#include <string>

namespace DllUtils {

    // 把对象转换为const char*
    const char* object2Uint8t(juce::var obj);

    juce::var charPtr2object(const char* ptr);

    // 将cpp字符串转为字符串指针的形式并拷贝一份到堆上面
    const char* copyStringOnHeap(juce::String& str);
    // 将cpp字符串转为字符串指针的形式并拷贝一份到堆上面
    const char* copyStringOnHeap(std::string& str);

} // namespace DllUtils