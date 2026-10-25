#include "./convertUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <vector>

const char* ConvertUtils::object2Uint8t(nlohmann::json obj) {

    std::string json = obj.dump();

    auto cPtr = copyStringOnHeap(json);
    return cPtr;
}

nlohmann::json ConvertUtils::charPtr2object(const char* ptr) {
    // 1. 解析 JSON
    nlohmann::json parsed = nlohmann::json::parse(ptr);

    return parsed;
}

const char* ConvertUtils::copyStringOnHeap(std::string& str) {
    const char* ptr{str.c_str()};
    auto length{strlen(ptr)};
    char* copyPtr{static_cast<char*>(malloc(length + 1))};
    if (copyPtr) memcpy(copyPtr, ptr, length + 1);
    return copyPtr;
}
