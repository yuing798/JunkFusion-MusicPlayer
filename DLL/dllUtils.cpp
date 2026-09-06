#include "./dllUtils.hpp"
#include "dllManager.hpp"
#include "dllUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

const char* DllUtils::object2Uint8t(juce::var obj) {

    // 2. 转换为 JSON 字符串（无多余空格，紧凑格式）
    //    必须用局部变量持有 juce::String，否则 .toRawUTF8() 指向临时对象的内部缓冲区，
    //    分号执行完后临时 String 销毁 → 野指针 → 下游读到垃圾数据
    juce::String json = juce::JSON::toString(obj);

    auto cString = copyStringOnHeap(json);
    return cString;
}

juce::var DllUtils::charPtr2object(const char* ptr) {
    // 1. 解析 JSON
    juce::var parsed = juce::JSON::parse(juce::String::fromUTF8(ptr));

    // 2. 获取 DynamicObject
    juce::var obj = parsed.getDynamicObject();

    return obj;
}

const char* DllUtils::copyStringOnHeap(juce::String& str) {
    const char* ptr{str.toRawUTF8()};
    auto length{strlen(ptr)};
    char* copyPtr{static_cast<char*>(malloc(length + 1))};
    if (copyPtr) memcpy(copyPtr, ptr, length + 1);
    return copyPtr;
}

const char* DllUtils::copyStringOnHeap(std::string& str) {
    const char* ptr{str.c_str()};
    auto length{strlen(ptr)};
    char* copyPtr{static_cast<char*>(malloc(length + 1))};
    if (copyPtr) memcpy(copyPtr, ptr, length + 1);
    return copyPtr;
}

std::string DllUtils::escapeLucene(const std::string& input) {
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