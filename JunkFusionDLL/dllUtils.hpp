#pragma once

#include "DartApi/dart_api_dl.h"
#include "DartApi/dart_native_api.h"
#include "juce_core/juce_core.h"
#include <cstdint>
#include <string>

// 异步返回给dart端使用
void send_object_to_dart(int64_t dart_port, const juce::var& data);