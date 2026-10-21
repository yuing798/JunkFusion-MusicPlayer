#pragma once

#include "juce_core/juce_core.h"
#include <cstdint>
#include <optional>
#include <type_traits>
#include <vector>

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
    juce::StringArray toStringArray() const;
    bool toBool() const;

    juce::String toString();
    std::optional<juce::String> toOptStr();
    bool isVoid() { return value.isVoid(); }
    bool isObject() { return value.isObject(); }
    bool hasProperty(const char* key) { return value.hasProperty(key); }
    int size() { return value.size(); }

    template <typename T> std::vector<T> toVector() {
        if (!value.isArray()) return {};
        std::vector<T> vec;
        for (int i = 0; i < size(); i++) {
            T value;
            if constexpr (std::is_same_v<T, int> || std::is_same_v<T, int64_t>) {
                value = read(i).toInt();
            } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
                value = read(i).toDouble();
            }
            vec.push_back(value);
        }
        return vec;
    }
};