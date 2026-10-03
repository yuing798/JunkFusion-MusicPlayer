#pragma once
#include "juce_core/juce_core.h"
#include <optional>
#include <string>
#include <vector>

namespace ConvertUtils {

    // 把对象转换为const char*
    const char* object2Uint8t(juce::var obj);

    juce::var charPtr2object(const char* ptr);

    // 将cpp字符串转为字符串指针的形式并拷贝一份到堆上面
    const char* copyStringOnHeap(juce::String& str);
    // 将cpp字符串转为字符串指针的形式并拷贝一份到堆上面
    const char* copyStringOnHeap(std::string& str);

    // 对字符串进行Lucene特殊字符筛选并在前面加上反斜杠
    std::string escapeLucene(const std::string& input);

    // juce::StringArray转换为juce::Array<juce::var>
    juce::Array<juce::var> stringArray2ArrayVar(juce::StringArray arr);

    juce::var optionalIntToVar(const std::optional<int>& value);
    juce::var juceStringToVar(const juce::String& value);
    std::vector<std::string> stringArrayToVector(const juce::StringArray& arr);

} // namespace ConvertUtils