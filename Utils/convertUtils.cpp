#include "./convertUtils.hpp"
#include "Utils/Yvar.hpp"
#include <cstdlib>
#include <cstring>
#include <string>

const char* ConvertUtils::object2Uint8t(const Yvar& obj) {
    std::string jsonStr = obj.dump();
    return copyStringOnHeap(jsonStr);
}

Yvar ConvertUtils::charPtr2Yvar(const char* ptr) {
    if (ptr == nullptr) return Yvar{};
    return Yvar::parse(ptr);
}

const char* ConvertUtils::copyStringOnHeap(std::string& str) {
    const char* ptr{str.c_str()};
    auto length{strlen(ptr)};
    char* copyPtr{static_cast<char*>(malloc(length + 1))};
    if (copyPtr) memcpy(copyPtr, ptr, length + 1);
    return copyPtr;
}
