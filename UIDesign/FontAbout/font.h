#pragma once
#include "juce_core/juce_core.h"
#include <JuceHeader.h>
#include <cstddef>
#include <string>
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
        } else if constexpr (std::is_same_v<Args, std::string>){
            return U(args.c_str());
        } else if constexpr (
            std::is_same_v<Args, int> || 
            std::is_same_v<Args, float> || 
            std::is_same_v<Args, size_t>
        ){
            return juce::String(args);
        } else{
            return "";
        }
    }()), ...);
    //折叠表达式 ( ... , ... )
    //这是右折叠语法，专门针对逗号运算符
    //对于参数包Args...,他会展开为{expr1,expr2,expr3,...}
    //逗号折叠表达式会从左到右一次求值没有expri
    return result;
}
