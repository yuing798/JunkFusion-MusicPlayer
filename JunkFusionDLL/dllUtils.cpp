#include "./dllUtils.hpp"
#include "juce_core/system/juce_PlatformDefs.h"

void asyncSendJuceVar2Dart(int64_t dart_port, const juce::var& data) {

    jassert(
        data.isObject() || data.isArray() || data.isVoid()
    ); // 禁止非对象或者非数组或者非空值传递

    // 1. 序列化为 JSON 字符串
    juce::String jsonStr = juce::JSON::toString(data);

    // 2. 发送给 Dart
    Dart_CObject dart_object;
    dart_object.type = Dart_CObject_kString;

    dart_object.value.as_string = jsonStr.toRawUTF8();

    Dart_PostCObject_DL(dart_port, &dart_object);
}