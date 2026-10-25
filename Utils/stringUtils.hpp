#pragma once

#include "juce_core/juce_core.h"
#include <cstddef>
#include <cstdint>
#include <string>
namespace StringUtils {

    // 去首尾空白（ASCII 空白，足够日常使用）
    std::string trim(const std::string& s);

    // 只转 ASCII 字母，避免破坏 UTF-8 多字节序列
    void toLowerCaseInPlace(std::string& s);

    void toUpperCaseInPlace(std::string& s);

    // 替换所有出现的子串（UTF-8 字节序列替换）
    void replaceAll(std::string& s, std::string_view from, std::string_view to);

    /**
     * @brief 对字符串进行标准化
     *
     * @param str
     * @return std::string
     */
    std::string normalizeString(const std::string& str);

    // 计算将一个字符串转换成另一个字符串所需的最少编辑操作次数（插入、删除、替换）。距离越小，字符串越相似。
    int levenshteinDistance(const std::string& s1, const std::string& s2);

    // 计算相似度分数（0 ~1）
    double stringSimilarity(const std::string& s1, const std::string& s2);

    // 是否为可用的艺术家，比如不是群星那些的
    bool isVariousArtists(const std::string& name);

    // 判断两个艺术家数组的匹配度
    double calcArtistsSimlarity(std::vector<std::string>& arr1, std::vector<std::string>& arr2);

    bool containsOnly(const std::string& s, std::string_view allowed);

    // 对字符串进行Lucene特殊字符筛选并在前面加上反斜杠
    std::string escapeLucene(const std::string& input);

    template <typename T>
    std::string vectorToString(const std::vector<T>& vec, const char* separtor) {
        static_assert(
            std::is_same_v<T, int> || std::is_same_v<T, int64_t> || std::is_same_v<T, double> ||
                std::is_same_v<T, float> || std::is_same_v<T, size_t> ||
                std::is_same_v<T, std::string>,
            "vectorToString throw type error"
        );
        std::string res;
        if constexpr (std::is_same_v<T, std::string>) {
            for (size_t i = 0; i < vec.size(); i++) {
                res += vec[i];
                if (i != vec.size() - 1) {
                    res += separtor;
                }
            }
        } else {
            for (size_t i = 0; i < vec.size(); i++) {
                res += std::to_string(vec[i]);
                if (i != vec.size() - 1) {
                    res += separtor;
                }
            }
        }
        return res;
    }

} // namespace StringUtils