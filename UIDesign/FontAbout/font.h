#pragma once
#include <JuceHeader.h>

//这个函数支持中文字符显示
inline juce::String U(const char* str) {
    return juce::String::fromUTF8(str);
}
