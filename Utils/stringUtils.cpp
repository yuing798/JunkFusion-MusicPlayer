#include "./stringUtils.hpp"
#include "juce_core/juce_core.h"
#include <string>
#include <vector>

namespace StringUtils {
    // 去首尾空白（ASCII 空白，足够日常使用）
    std::string trim(const std::string& s) {
        const char* ws = " \t\n\r\f\v";
        auto start = s.find_first_not_of(ws);
        if (start == std::string::npos) return "";
        auto end = s.find_last_not_of(ws);
        return s.substr(start, end - start + 1);
    }

    void toUpperCaseInPlace(std::string& s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') ? static_cast<char>(c - ('a' - 'A'))
                                          : static_cast<char>(c);
        });
    }

    // 只转 ASCII 字母，避免破坏 UTF-8 多字节序列
    void toLowerCaseInPlace(std::string& s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A'))
                                          : static_cast<char>(c);
        });
    }

    // 替换所有出现的子串（UTF-8 字节序列替换）
    void replaceAll(std::string& s, std::string_view from, std::string_view to) {
        if (from.empty()) return;
        std::size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
    }

    std::string normalizeString(const std::string& str) {
        std::string s = trim(str);
        toLowerCaseInPlace(s);

        // 各种 Unicode 连字符 -> ASCII '-'
        replaceAll(s, "\xE2\x80\x90", "-"); // U+2010 ‐
        replaceAll(s, "\xE2\x80\x93", "-"); // U+2013 –
        replaceAll(s, "\xE2\x80\x94", "-"); // U+2014 —
        replaceAll(s, "\xE2\x88\x92", "-"); // U+2212 −

        // 全角空格 U+3000 -> 半角空格
        replaceAll(s, "\xE3\x80\x80", " ");

        return s;
    }
    int levenshteinDistance(const std::string& s1, const std::string& s2) {
        const int len1 = s1.length();
        const int len2 = s2.length();
        std::vector<std::vector<int>> dp(len1 + 1, std::vector<int>(len2 + 1));

        for (int i = 0; i <= len1; ++i)
            dp[i][0] = i;
        for (int j = 0; j <= len2; ++j)
            dp[0][j] = j;

        for (int i = 1; i <= len1; ++i) {
            for (int j = 1; j <= len2; ++j) {
                int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
                dp[i][j] = std::min(
                    {dp[i - 1][j] + 1, // 删除
                     dp[i][j - 1] + 1, // 插入
                     dp[i - 1][j - 1] + cost}
                ); // 替换
            }
        }
        return dp[len1][len2];
    }

    // 计算相似度分数（0~1）
    double stringSimilarity(const std::string& s1, const std::string& s2) {
        if (s1.empty() && s2.empty()) return 1.0;
        int maxLen = std::max(s1.length(), s2.length());
        if (maxLen == 0) return 1.0; // 两者都空
        int distance = levenshteinDistance(s1, s2);
        return 1.0 - static_cast<double>(distance) / maxLen;
    }

    bool isVariousArtists(const std::string& name) {
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        return (
            lower.find("various") != std::string::npos || lower.find("群星") != std::string::npos ||
            lower == "v.a." || lower == "va"
        );
    }

    bool containsOnly(const std::string& s, std::string_view allowed) {
        return s.find_first_not_of(allowed) == std::string::npos;
    }

    double calcArtistsSimlarity(std::vector<std::string>& arr1, std::vector<std::string>& arr2) {

        double totalArtistScore = 0.0;

        for (const auto& artistStr1 : arr1) {
            std::string str1Normalized = normalizeString(artistStr1);
            double bestMatchForThisArtist = 0.0;

            for (const auto& artistStr2 : arr2) {
                std::string str2Normalized = StringUtils::normalizeString(artistStr2);

                // A. 基础字符串相似度（Levenshtein）
                double sim = StringUtils::stringSimilarity(str1Normalized, str2Normalized);

                // B. 跨语种/音译启发式处理（如 周杰伦 vs Jay Chou, 字符集交集为 0）
                if (sim < 0.2) {
                    bool isLocalASCII =
                        containsOnly(str1Normalized, "abcdefghijklmnopqrstuvwxyz0123456789 -_.");
                    bool isRemoteASCII =
                        containsOnly(str2Normalized, "abcdefghijklmnopqrstuvwxyz0123456789 -_.");

                    // 如果一方是纯英文/罗马音，另一方是非英文（中文/日文/韩文等），大概率是音译或跨国别名
                    if (isLocalASCII != isRemoteASCII) {
                        sim = 0.6; // 给予中立的跨语言预估分，防止判定为 0 分
                    }
                }

                if (sim > bestMatchForThisArtist) {
                    bestMatchForThisArtist = sim;
                }
            }
            // 累加本地每个艺术家的最高得分
            totalArtistScore += bestMatchForThisArtist;
        }

        // 计算平均匹配分 (0.0 ~ 1.0)
        double baseArtistScore = totalArtistScore / arr1.size();

        // 计算数量比率 (0.0 ~ 1.0)
        double sizeRatio =
            (double)std::min(arr1.size(), arr1.size()) / (double)std::max(arr2.size(), arr2.size());

        // 综合原始得分
        double rawArtistScore = (baseArtistScore * 0.8) + (sizeRatio * 0.2);

        // C. 保底平滑映射 (Floor Scaling)
        // 将 rawArtistScore [0.0 ~ 1.0] 线性缩放映射至 [0.5 ~ 1.0]
        // 效果：即使名字完全不匹配(0.0)，也只扣除 50% 权重，保留 0.5
        // 的保底分，绝不抹杀 AcoustID 的指纹匹配成果
        double safeMultiplier = 0.5 + (rawArtistScore * 0.5);
        return safeMultiplier;
    }
    std::string escapeLucene(const std::string& input) {
        // 构建一个用于快速查找的哈希表（仅初始化一次）
        static const std::unordered_set<char> specials = {
            '+',
            '-',
            '&',
            '|',
            '!',
            '(',
            ')',
            '{',
            '}',
            '[',
            ']',
            '^',
            '"',
            '~',
            '*',
            '?',
            ':',
            '\\',
            '/'
        };

        std::string output;
        output.reserve(input.size() * 2); // 预分配内存，防止频繁扩容

        for (char c : input) {
            if (specials.find(c) != specials.end()) {
                output.push_back('\\'); // 在前面加反斜杠
            }
            output.push_back(c);
        }
        return output;
    }
} // namespace StringUtils