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

std::string ConvertUtils::escapeLucene(const std::string& input) {
    // 构建一个用于快速查找的哈希表（仅初始化一次）
    static const std::unordered_set<char> specials = {
        '+',
        '-',
        '&',
        '|',
        '!',
        '(',
        ')',
        '{',
        '}',
        '[',
        ']',
        '^',
        '"',
        '~',
        '*',
        '?',
        ':',
        '\\',
        '/'
    };

    std::string output;
    output.reserve(input.size() * 2); // 预分配内存，防止频繁扩容

    for (char c : input) {
        if (specials.find(c) != specials.end()) {
            output.push_back('\\'); // 在前面加反斜杠
        }
        output.push_back(c);
    }
    return output;
}

juce::Array<juce::var> ConvertUtils::stringArray2ArrayVar(juce::StringArray arr) {
    juce::Array<juce::var> varArr;
    for (auto& singleStr : arr) {
        varArr.add(juce::var(singleStr));
    }
    return varArr;
}

juce::var ConvertUtils::juceStringToVar(const juce::String& v) {
    return v.isNotEmpty() ? juce::var(v) : juce::var();
}
juce::var ConvertUtils::optionalIntToVar(const std::optional<int>& v) {
    return v.has_value() ? juce::var(v.value()) : juce::var();
}