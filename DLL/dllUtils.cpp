#include "./dllUtils.hpp"
#include "dllManager.hpp"
#include "dllUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>

juce::var DllUtils::uint8t2Object(const char* str) {

    // 解析为 juce::var
    juce::var parsed = juce::JSON::parse(juce::String(str));

    // 检查是否为 DynamicObject
    if (parsed.isObject()) {
        juce::var obj = parsed.getDynamicObject();
        return obj;
    } else {
        return juce::var();
    }
}

const char* DllUtils::object2Uint8t(juce::var obj) {

    // 2. 转换为 JSON 字符串（无多余空格，紧凑格式）
    //    必须用局部变量持有 juce::String，否则 .toRawUTF8() 指向临时对象的内部缓冲区，
    //    分号执行完后临时 String 销毁 → 野指针 → 下游读到垃圾数据
    juce::String json = juce::JSON::toString(obj);

    auto cString = sendString2Frontend(json);
    return cString;
}

juce::var DllUtils::charPtr2object(const char* ptr) {
    // 1. 解析 JSON
    juce::var parsed = juce::JSON::parse(juce::String::fromUTF8(ptr));

    // 2. 获取 DynamicObject
    juce::var obj = parsed.getDynamicObject();

    return obj;
}

void DllUtils::sendMessage2AudioProcess(juce::var obj) {
    auto jsonStr{juce::JSON::toString(obj).toStdString()};
    dllManager::getInstance().sendMessage2AudioProcess(jsonStr);
}

const char* DllUtils::sendString2Frontend(juce::String& str) {
    const char* ptr{str.toRawUTF8()};
    auto length{strlen(ptr)};
    char* copyPtr{static_cast<char*>(malloc(length + 1))};
    if (copyPtr) memcpy(copyPtr, ptr, length + 1);
    return copyPtr;
}
const char* DllUtils::sendString2Frontend(std::string& str) {
    const char* ptr{str.c_str()};
    auto length{strlen(ptr)};
    char* copyPtr{static_cast<char*>(malloc(length + 1))};
    if (copyPtr) memcpy(copyPtr, ptr, length + 1);
    return copyPtr;
}