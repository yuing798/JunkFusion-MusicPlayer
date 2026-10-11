#pragma once
#include "juce_core/juce_core.h"
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace ConvertUtils {

    // 把对象转换为const char*
    const char* object2Uint8t(juce::var obj);

    juce::var charPtr2object(const char* ptr);

    // 将cpp字符串转为字符串指针的形式并拷贝一份到堆上面
    const char* copyStringOnHeap(juce::String& str);
    // 将cpp字符串转为字符串指针的形式并拷贝一份到堆上面
    const char* copyStringOnHeap(std::string& str);

    // juce::StringArray转换为juce::Array<juce::var>
    juce::Array<juce::var> stringArray2ArrayVar(const std::optional<juce::StringArray>& arr);
    juce::Array<juce::var> stringArray2ArrayVar(const juce::StringArray& arr);

    std::vector<std::string> stringArrayToVector(const juce::StringArray& arr);
    template <typename T> juce::Array<juce::var> vectorToArrayVar(const std::vector<T>& vecs) {
        static_assert(
            std::is_same_v<T, int> || std::is_same_v<T, double> || std::is_same_v<T, float>,
            "convertUtils::vectorToArrayVar传入类型错误"
        );
        juce::Array<juce::var> arr;

        for (auto& each : vecs) {
            arr.add(each);
        }
        return arr;
    }

    template <typename T> juce::var optValueToVar(const std::optional<T>& value) {
        if (value.has_value()) {
            return value.value();
        } else {
            return juce::var{};
        }
    }

} // namespace ConvertUtils