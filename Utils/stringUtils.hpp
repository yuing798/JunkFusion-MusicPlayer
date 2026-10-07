#pragma once

#include "juce_core/juce_core.h"
namespace StringUtils {

    /**
     * @brief 对字符串进行标准化
     *
     * @param str
     * @return juce::String
     */
    juce::String normalizeString(const juce::String& str);

    // 转化字符串为UTF8
    inline juce::String utf8(const char* name) { return juce::String::fromUTF8(name); };

    // 计算将一个字符串转换成另一个字符串所需的最少编辑操作次数（插入、删除、替换）。距离越小，字符串越相似。
    int levenshteinDistance(const juce::String& s1, const juce::String& s2);

    // 计算相似度分数（0 ~1）
    double stringSimilarity(const juce::String& s1, const juce::String& s2);

    // 是否为可用的艺术家，比如不是群星那些的
    bool isVariousArtists(const std::string& name);

    // 判断两个艺术家数组的匹配度
    double calcArtistsSimlarity(juce::StringArray& arr1, juce::StringArray arr2);

} // namespace StringUtils