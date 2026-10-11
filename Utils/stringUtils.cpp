#include "./stringUtils.hpp"
#include "juce_core/juce_core.h"

juce::String StringUtils::normalizeString(const juce::String& str) {
    juce::String s = str.trim().toLowerCase();

    // 将各种 Unicode 连字符（如 ‐, –, —）统一替换为标准 ASCII 短横线 '-'
    s = s.replaceCharacters(
        juce::CharPointer_UTF8("\xE2\x80\x90-\xE2\x80\x93\xE2\x80\x94\xE2\x88\x92"),
        juce::CharPointer_UTF8("-----")
    );

    // 替换全角空格（0x3000）为普通半角空格
    s = s.replace(juce::String::charToString(0x3000), " ");

    return s;
}
int StringUtils::levenshteinDistance(const juce::String& s1, const juce::String& s2) {
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
double StringUtils::stringSimilarity(const juce::String& s1, const juce::String& s2) {
    if (s1.isEmpty() && s2.isEmpty()) return 1.0;
    int maxLen = std::max(s1.length(), s2.length());
    if (maxLen == 0) return 1.0; // 两者都空
    int distance = levenshteinDistance(s1, s2);
    return 1.0 - static_cast<double>(distance) / maxLen;
}

bool StringUtils::isVariousArtists(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return (
        lower.find("various") != std::string::npos || lower.find("群星") != std::string::npos ||
        lower == "v.a." || lower == "va"
    );
}
double StringUtils::calcArtistsSimlarity(juce::StringArray& arr1, juce::StringArray& arr2) {

    double totalArtistScore = 0.0;

    for (const auto& artistStr1 : arr1) {
        juce::String str1Normalized = normalizeString(artistStr1);
        double bestMatchForThisArtist = 0.0;

        for (const auto& artistStr2 : arr2) {
            juce::String str2Normalized = StringUtils::normalizeString(artistStr2);

            // A. 基础字符串相似度（Levenshtein）
            double sim = StringUtils::stringSimilarity(str1Normalized, str2Normalized);

            // B. 跨语种/音译启发式处理（如 周杰伦 vs Jay Chou, 字符集交集为 0）
            if (sim < 0.2) {
                bool isLocalASCII =
                    str1Normalized.containsOnly("abcdefghijklmnopqrstuvwxyz0123456789 -_.");
                bool isRemoteASCII =
                    str2Normalized.containsOnly("abcdefghijklmnopqrstuvwxyz0123456789 -_.");

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
std::string StringUtils::escapeLucene(const std::string& input) {
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