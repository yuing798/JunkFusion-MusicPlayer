#pragma once
#include "juce_core/juce_core.h"
#include <JuceHeader.h>
#include <type_traits>

//这个函数支持UTF8字符显示
inline juce::String U(const char* str) {
    return juce::String::fromUTF8(str);
}

//把多个参数拼接成一整个字符串
template<typename... Args>
inline juce::String longUTF8(Args... args) {
    juce::String result;
    // 使用折叠表达式 + 逗号运算符，逐个处理
    ((result += [&]() -> juce::String {
        if constexpr (std::is_same_v<Args, const char*>) {
            return U(args);
        } else if constexpr (std::is_same_v<Args, juce::String>) {
            return args;
        } else {
            return juce::String(args); // 需确保支持
        }
    }()), ...);
    //折叠表达式 ( ... , ... )
    //这是右折叠语法，专门针对逗号运算符
    //对于参数包Args...,他会展开为{expr1,expr2,expr3,...}
    //逗号折叠表达式会从左到右一次求值没有expri
    return result;
}
