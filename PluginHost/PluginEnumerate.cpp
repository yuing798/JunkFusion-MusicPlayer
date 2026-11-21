#include "./PluginEnumerate.hpp"

namespace PluginHost {

    namespace {
        /** juce::String 内部是 UTF-8，toStdString() 直接给出 UTF-8 字节 */
        std::string toStd(const juce::String& text) {
            return text.toStdString();
        }
    } // namespace

    Enumerator::Enumerator() {
        // JUCE 8 把 addDefaultFormats() 删了（= delete），必须用新入口。
        // headless 版本装出来的插件实例 hasEditor() 恒为 false——
        // 正是扫描进程要的语义（见头文件"为什么是 headless"）。
        juce::addHeadlessDefaultFormatsToManager(mFormats);
    }

    bool Enumerator::collectDescriptions(
        const juce::File& module,
        juce::OwnedArray<juce::PluginDescription>& out,
        std::string& error
    ) const {
        if (!module.exists()) {
            error = "模块不存在: " + toStd(module.getFullPathName());
            return false;
        }

        const auto path = module.getFullPathName();
        bool anyFormatClaims = false;

        for (int i = 0; i < mFormats.getNumFormats(); ++i) {
            auto* format = mFormats.getFormat(i);
            if (format == nullptr) continue;

            // 规格 §1.1：VST3 这一关的实现就是「后缀是 .vst3 且存在」，
            // 所以非 .vst3 命名的二进制在这里就被挡掉了，不需要额外兼容。
            if (!format->fileMightContainThisPluginType(path)) continue;

            anyFormatClaims = true;
            format->findAllTypesForFile(out, path);
        }

        if (!anyFormatClaims) {
            error = "没有格式认领这个文件（可能不是 .vst3）";
            return false;
        }

        if (out.isEmpty()) {
            error = "枚举不到任何插件类型（模块可能损坏或架构不匹配）";
            return false;
        }

        return true;
    }

    bool Enumerator::enumerate(
        const juce::File& module,
        std::vector<Record>& out,
        std::string& error
    ) {
        out.clear();
        error.clear();

        juce::OwnedArray<juce::PluginDescription> descriptions;
        if (!collectDescriptions(module, descriptions, error)) return false;

        int ordinal = 0;
        for (const auto* description : descriptions) {
            if (description == nullptr) continue;

            Record record;
            record.ordinal = ordinal++;
            record.meta.name = toStd(description->name);
            record.meta.vendor = toStd(description->manufacturerName);
            record.meta.category = toStd(description->category);
            record.meta.identifier = toStd(description->createIdentifierString());
            record.meta.numInputs = description->numInputChannels;
            record.meta.numOutputs = description->numOutputChannels;
            record.meta.isInstrument = description->isInstrument;

            // 名字实在拿不到时用文件名兜底，避免库里出现空名字的行
            if (record.meta.name.empty()) {
                record.meta.name = toStd(module.getFileNameWithoutExtension());
            }

            out.push_back(std::move(record));
        }

        return !out.empty();
    }

    bool Enumerator::measureChannels(
        const juce::File& module,
        const std::string& identifier,
        int& numInputs,
        int& numOutputs,
        std::string& error
    ) {
        error.clear();

        juce::OwnedArray<juce::PluginDescription> descriptions;
        if (!collectDescriptions(module, descriptions, error)) return false;

        const juce::PluginDescription* target = nullptr;
        for (const auto* description : descriptions) {
            if (description == nullptr) continue;
            if (identifier.empty() || toStd(description->createIdentifierString()) == identifier) {
                target = description;
                break;
            }
        }

        if (target == nullptr) {
            error = "找不到匹配的插件类型";
            return false;
        }

        juce::String juceError;
        auto instance = mFormats.createPluginInstance(*target, 48000.0, 512, juceError);
        if (instance == nullptr) {
            error = juceError.isNotEmpty() ? toStd(juceError) : "createPluginInstance 返回空";
            return false;
        }

        // 只读总线布局，不 prepareToPlay
        instance->enableAllBuses();
        numInputs = instance->getTotalNumInputChannels();
        numOutputs = instance->getTotalNumOutputChannels();
        return true;
    }

} // namespace PluginHost
