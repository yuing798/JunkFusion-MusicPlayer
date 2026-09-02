#pragma once
#include "juce_core/juce_core.h"
#include <string>

namespace DllUtils {

    // 把对象转换为const char*
    const char* object2Uint8t(juce::var obj);

    juce::var charPtr2object(const char* ptr);

    // 将cpp字符串转为字符串指针的形式并拷贝一份到堆上面
    const char* copyStringOnHeap(juce::String& str);
    // 将cpp字符串转为字符串指针的形式并拷贝一份到堆上面
    const char* copyStringOnHeap(std::string& str);

    // 对字符串进行Lucene特殊字符筛选并在前面加上反斜杠
    std::string escapeLucene(const std::string& input);

    // 将数组转换为/进行分隔的字符串，一般在艺术家数组和体裁数组出现
    std::string tagVector2String(std::vector<std::string>& tags);

} // namespace DllUtils