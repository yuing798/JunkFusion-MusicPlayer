#pragma once

#include "juce_core/juce_core.h"
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

// 用于安全读取json的类
class Yvar {
private:
    nlohmann::json value;

public:
    Yvar() = default;
    Yvar(nlohmann::json value) : value(std::move(value)) {}

    // 安全读取对象的键值对
    Yvar read(const char* key) const;

    // 安全读取数组索引
    Yvar read(int index) const;

    double toDouble() const;
    int toInt() const;
    std::vector<std::string> toStringArray() const;
    bool toBool() const;

    std::string toString();
    std::optional<std::string> toOptStr();
    bool isVoid() { return value.is_null(); }
    bool isObject() { return value.is_object(); }
    bool hasProperty(const char* key) { return value.contains(key); }
    int size() { return value.size(); }

    template <typename T> std::vector<T> toVector() {
        if (!value.is_array()) return {};
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