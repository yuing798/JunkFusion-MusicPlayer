#include "./dllUtils.hpp"
#include "dllManager.hpp"
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

    // 注意这里是指针，实际数据还是在juce::String里面，所以必须申请堆内存
    const char* jsonStr = json.toRawUTF8();

    // 申请堆内存
    size_t length{strlen(jsonStr)};
    char* cString{static_cast<char*>(malloc(length + 1))};
    if (cString) {
        memcpy(cString, jsonStr, length + 1);
    }
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