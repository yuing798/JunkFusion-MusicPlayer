#pragma once

// #include "DartApi/dart_api_dl.h"
// #include "DartApi/dart_native_api.h"
#include "juce_core/juce_core.h"
#include <cstdint>
#include <string>

// 异步返回给dart端使用
// void asyncSendJuceVar2Dart(int64_t dart_port, const juce::var& data);

// 把const char*转换为对象
juce::DynamicObject uint8t2Object(const char* str);

// 把对象转换为const char*
const char* object2Uint8t(juce::DynamicObject::Ptr obj);

juce::DynamicObject::Ptr charPtr2object(const char* ptr);