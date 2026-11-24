#include "PluginHost/PluginInstanceFactory.hpp"

#include "Utils/constants.h"

#include <spdlog/spdlog.h>

namespace PluginHost {

    PluginInstanceFactory::PluginInstanceFactory() {
        // 带 UI 的默认格式：以后要在音频进程里开插件编辑器，所以这里不能用
        // addHeadlessDefaultFormatsToManager（那样 hasEditor() 恒为 false）
        juce::addDefaultFormatsToManager(mFormats);
    }

    bool PluginInstanceFactory::findDescription(
        const std::string& filePath,
        const std::string& identifier,
        juce::PluginDescription& out,
        std::string& error
    ) {
        const juce::String path{juce::String::fromUTF8(filePath.c_str())};
        const juce::File file{path};
        if (!file.exists()) {
            error = "插件文件不存在: " + filePath;
            return false;
        }

        bool claimed = false;
        for (int i = 0; i < mFormats.getNumFormats(); ++i) {
            auto* format = mFormats.getFormat(i);
            if (format == nullptr) continue;
            if (!format->fileMightContainThisPluginType(path)) continue;

            claimed = true;
            juce::OwnedArray<juce::PluginDescription> found;
            format->findAllTypesForFile(found, path);

            for (const auto* description : found) {
                if (description == nullptr) continue;
                if (identifier.empty() ||
                    description->createIdentifierString().toStdString() == identifier) {
                    out = *description;
                    return true;
                }
            }
        }

        error = claimed ? ("找不到匹配的插件类型: " + identifier)
                        : "没有格式认领这个文件（可能不是 .vst3）";
        return false;
    }

    std::unique_ptr<juce::AudioPluginInstance> PluginInstanceFactory::create(
        const std::string& filePath,
        const std::string& identifier,
        double sampleRate,
        int blockSize,
        std::string& error
    ) {
        error.clear();

        juce::PluginDescription description;
        if (!findDescription(filePath, identifier, description, error)) return nullptr;

        juce::String juceError;
        auto instance = mFormats.createPluginInstance(description, sampleRate, blockSize, juceError);
        if (instance == nullptr) {
            error = juceError.isNotEmpty() ? juceError.toStdString() : "createPluginInstance 返回空";
            if (auto logger = spdlog::get(LogAudioID))
                logger->error("[PluginChain] 实例化失败 {}: {}", filePath, error);
            return nullptr;
        }

        // 打开全部总线：侧链/多输出插件的总线默认是关的，不打开会没声
        instance->enableAllBuses();
        return instance;
    }

} // namespace PluginHost
