#pragma once

#include "juce_core/juce_core.h"

// 用于安全读取json的类
class Yvar {
private:
    juce::var value{};

public:
    Yvar() = default;
    Yvar(juce::var value) : value(std::move(value)) {}

    // 安全读取对象的键值对
    Yvar read(const char* key) const;

    // 安全读取数组索引
    Yvar read(int index) const;

    double toDouble() const;
    int toInt() const;
    float toFloat32() const;

    const char* toRawUTF8() { return toString().toRawUTF8(); }
    juce::String toString();
    bool isVoid() { return value.isVoid(); }
    bool isObject() { return value.isObject(); }
    bool hasProperty(const char* key) { return value.hasProperty(key); }
    int size() { return value.size(); }
};