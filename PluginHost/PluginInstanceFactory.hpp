/**
 * @brief 音频进程里的插件加载器：filePath + identifier → 一个可用的 AudioProcessor 实例
 *
 * ── 它和 PluginEnumerate 的区别 ───────────────────────────────────────
 *   PluginEnumerate 跑在**扫描子进程**里，用 headless 格式管理器，永远不 prepare、
 *   不开编辑器，只读元数据。
 *   本类跑在**音频进程**里，用带 UI 的格式管理器（`addDefaultFormatsToManager`），
 *   因为最终要在音频进程里开插件编辑器（见 host-integration.md §8 的结论）。
 *
 * ── 为什么要重新枚举一遍（规格 §3.5）─────────────────────────────────
 *   `PluginDescription` 不能跨进程传。数据库里只有 filePath + identifier，
 *   真正加载时按 filePath 现场 `findAllTypesForFile()` 再按 identifier 挑一个。
 *
 * ── 线程 ─────────────────────────────────────────────────────────────
 *   实例化会 `LoadLibrary` 插件 DLL，耗时几十到几百毫秒，**绝不能在音频线程调**。
 *   本类的所有方法都只应在 puller 线程（或其它非实时线程）调用。
 */
#pragma once

#include "PluginHost/PluginTypes.hpp"

#include "juce_audio_processors/juce_audio_processors.h"

#include <memory>
#include <string>

namespace PluginHost {

    class PluginInstanceFactory {
    public:
        PluginInstanceFactory();
        PluginInstanceFactory(const PluginInstanceFactory&) = delete;
        PluginInstanceFactory& operator=(const PluginInstanceFactory&) = delete;

        /**
         * @brief 按 filePath + identifier 创建实例
         *
         * @param filePath   模块路径（数据库 plugins.filePath）
         * @param identifier `PluginDescription::createIdentifierString()`，空串 = 取第一个
         * @param sampleRate 初始采样率（后面还会 prepare 一次）
         * @param blockSize  初始块大小
         * @param error      失败原因（UTF-8）
         * @return 成功返回实例；失败返回 nullptr 并填 error，不抛异常
         */
        std::unique_ptr<juce::AudioPluginInstance> create(
            const std::string& filePath,
            const std::string& identifier,
            double sampleRate,
            int blockSize,
            std::string& error
        );

    private:
        /// 枚举 filePath 下的所有类型并挑出 identifier 对应的那个
        bool findDescription(
            const std::string& filePath,
            const std::string& identifier,
            juce::PluginDescription& out,
            std::string& error
        );

        juce::AudioPluginFormatManager mFormats;
    };

} // namespace PluginHost
