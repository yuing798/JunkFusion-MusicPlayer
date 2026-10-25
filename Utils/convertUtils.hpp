#pragma once
#include "Utils/Yvar.hpp"
#include "juce_core/juce_core.h"
#include <string>

namespace ConvertUtils {

    // 把 Yvar 对象转换为 const char*（堆上拷贝一份，由 freeString 释放）
    const char* object2Uint8t(const Yvar& obj);

    // 把字符串解析成 Yvar（解析失败返回空值，不会抛异常）
    Yvar charPtr2Yvar(const char* ptr);

    // 将cpp字符串转为字符串指针的形式并拷贝一份到堆上面，要传递给dart使用的字符串必须拷贝到堆上
    const char* copyStringOnHeap(std::string& str);

} // namespace ConvertUtils
