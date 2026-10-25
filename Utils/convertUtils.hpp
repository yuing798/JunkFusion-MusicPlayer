#pragma once
#include "juce_core/juce_core.h"
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace ConvertUtils {

    // 把对象转换为const char*
    const char* object2Uint8t(nlohmann::json obj);

    nlohmann::json charPtr2object(const char* ptr);

    // 将cpp字符串转为字符串指针的形式并拷贝一份到堆上面
    const char* copyStringOnHeap(std::string& str);

    template <typename T> nlohmann::json optValueToVar(const std::optional<T>& value) {
        if (value.has_value()) {
            return value.value();
        } else {
            return nlohmann::json{};
        }
    }

} // namespace ConvertUtils