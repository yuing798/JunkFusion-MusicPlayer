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
    juce::String toString();
    bool isVoid() { return value.isVoid(); }
    bool hasProperty(const char* key) { return value.hasProperty(key); }
    int size() { return value.size(); }
    // void write(Yvar v);
    // void write(const char* key, Yvar v);
};