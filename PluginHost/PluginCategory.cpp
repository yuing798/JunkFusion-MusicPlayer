#include "./PluginCategory.hpp"

#include <cctype>

namespace PluginHost {

    CategoryDecision classify(
        bool isInstrument,
        std::string_view category,
        int numInputs,
        int numOutputs
    ) {
        // 局部 lambda 而不是文件级工具函数：这里是唯一调用点，
        // 也不值得为「忽略大小写的子串查找」往 Utils/StringUtils 里塞一个
        // 只服务于插件分类的 API（那个文件是 juce::String 的地盘）。
        auto containsNoCase = [category](const char* needle) {
            const std::size_t needleLen = std::char_traits<char>::length(needle);
            if (needleLen == 0 || category.size() < needleLen) return false;

            for (std::size_t i = 0; i + needleLen <= category.size(); ++i) {
                std::size_t j = 0;
                for (; j < needleLen; ++j) {
                    const auto lhs = static_cast<unsigned char>(category[i + j]);
                    const auto rhs = static_cast<unsigned char>(needle[j]);
                    if (std::tolower(lhs) != std::tolower(rhs)) break;
                }
                if (j == needleLen) return true;
            }
            return false;
        };

        // 1. 元数据直接声明是音源（带音频输入的音源也靠这条兜住）
        if (isInstrument) return {Category::instrument, CategoryRule::isInstrument};

        // 2. VST3 的 kSynth / kInstrument 子分类
        if (containsNoCase("Synth") || containsNoCase("Instrument")) {
            return {Category::instrument, CategoryRule::categoryHint};
        }

        // 3. 没有音频输入却有输出：生成器
        if (numInputs == 0 && numOutputs > 0) {
            return {Category::instrument, CategoryRule::noInput};
        }

        // 4. 自称 Fx / Effect（只在元数据残缺的 0进0出 插件上才会生效）
        if (containsNoCase("Fx") || containsNoCase("Effect")) {
            return {Category::effect, CategoryRule::categoryFx};
        }

        // 5. 有输入就是效果器：监听 / 可视化插件（n进0出）也算效果器链的一种
        if (numInputs > 0) return {Category::effect, CategoryRule::hasInput};

        // 6. 0 进 0 出且毫无线索：进兜底桶，等用户手动改
        return {Category::other, CategoryRule::none};
    }

    CategoryDecision classifyMeta(const Meta& meta) {
        return classify(
            meta.isInstrument,
            meta.category,
            meta.numInputs,
            meta.numOutputs
        );
    }

    std::string describeDecision(const Meta& meta, const CategoryDecision& decision) {
        std::string text = categoryToString(decision.type);
        text += "(";
        text += ruleToString(decision.rule);
        text += ") ";
        text += std::to_string(meta.numInputs);
        text += "in/";
        text += std::to_string(meta.numOutputs);
        text += "out";
        return text;
    }

} // namespace PluginHost
