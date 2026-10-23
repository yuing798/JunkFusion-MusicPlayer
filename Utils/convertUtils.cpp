#include "./convertUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

const char* ConvertUtils::object2Uint8t(juce::var obj) {

    juce::String json = juce::JSON::toString(obj);

    auto cPtr = copyStringOnHeap(json);
    return cPtr;
}

juce::var ConvertUtils::charPtr2object(const char* ptr) {
    // 1. 解析 JSON
    juce::var parsed = juce::JSON::fromString(juce::String::fromUTF8(ptr));

    // 2. 获取 DynamicObject
    juce::var obj = parsed.getDynamicObject();

    return obj;
}

const char* ConvertUtils::copyStringOnHeap(juce::String& str) {
    const char* ptr{str.toRawUTF8()};
    auto length{strlen(ptr)};
    char* copyPtr{static_cast<char*>(malloc(length + 1))};
    if (copyPtr) memcpy(copyPtr, ptr, length + 1);
    return copyPtr;
}

const char* ConvertUtils::copyStringOnHeap(std::string& str) {
    const char* ptr{str.c_str()};
    auto length{strlen(ptr)};
    char* copyPtr{static_cast<char*>(malloc(length + 1))};
    if (copyPtr) memcpy(copyPtr, ptr, length + 1);
    return copyPtr;
}
juce::Array<juce::var>
ConvertUtils::stringArray2ArrayVar(const std::optional<juce::StringArray>& arr) {
    if (arr.has_value()) {
        return stringArray2ArrayVar(arr.value());
    } else {
        return juce::Array<juce::var>{};
    }
}

juce::Array<juce::var> ConvertUtils::stringArray2ArrayVar(const juce::StringArray& arr) {
    juce::Array<juce::var> varArr;
    for (auto& singleStr : arr) {
        varArr.add(singleStr);
    }
    return varArr;
}

std::vector<std::string> ConvertUtils::stringArrayToVector(const juce::StringArray& arr) {
    std::vector<std::string> vec;
    for (auto& element : arr) {
        vec.push_back(element.toStdString());
    }
    return vec;
}
