#include "./dllUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>

// void asyncSendJuceVar2Dart(int64_t dart_port, const juce::DynamicObject::Ptr& data) {

//     // 1. 序列化为 JSON 字符串
//     juce::String jsonStr = juce::JSON::toString(data.get());

//     // 2. 发送给 Dart
//     Dart_CObject dart_object;
//     dart_object.type = Dart_CObject_kString;

//     dart_object.value.as_string = jsonStr.toRawUTF8();

//     Dart_PostCObject_DL(dart_port, &dart_object);
// }
juce::DynamicObject uint8t2Object(const char* str) {

    // 解析为 juce::var
    juce::var parsed = juce::JSON::parse(juce::String(str));

    // 检查是否为 DynamicObject
    if (parsed.isObject()) {
        juce::DynamicObject::Ptr obj = parsed.getDynamicObject();
        return *obj;
    } else {
        return juce::DynamicObject{};
    }
}

const char* object2Uint8t(juce::DynamicObject::Ptr obj) {

    // 1. 包装为 var
    juce::var varObj(obj);

    // 2. 转换为 JSON 字符串（无多余空格，紧凑格式）
    //    必须用局部变量持有 juce::String，否则 .toRawUTF8() 指向临时对象的内部缓冲区，
    //    分号执行完后临时 String 销毁 → 野指针 → 下游读到垃圾数据
    juce::String json = juce::JSON::toString(varObj);
    const char* jsonStr = json.toRawUTF8();

    // 申请堆内存
    size_t length{strlen(jsonStr)};
    char* cString{static_cast<char*>(malloc(length + 1))};
    if (cString) {
        memcpy(cString, jsonStr, length + 1);
    }
    return cString;
}

juce::DynamicObject::Ptr charPtr2object(const char* ptr) {
    // 1. 解析 JSON
    juce::var parsed = juce::JSON::parse(juce::String::fromUTF8(ptr));

    // 2. 获取 DynamicObject
    juce::DynamicObject::Ptr obj = parsed.getDynamicObject();

    return obj;
}