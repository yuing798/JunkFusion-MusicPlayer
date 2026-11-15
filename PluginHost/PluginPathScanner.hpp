/**
 * @brief 递归找插件目录里的 `.vst3` 模块（跨平台，只做"找"，不加载任何 DLL）
 *
 * ── 它解决什么 ──────────────────────────────────────────────────────────
 * 扫描的第一步是"把要扫的文件列出来"。这件事必须**在主进程**做（不是子进程）：
 *   1. 主进程要知道总数才能算进度、才能用 index 做断点续扫；
 *   2. 列目录不需要加载插件，放主进程里做没有崩溃风险；
 *   3. 列表写在临时文件里传给子进程，"从第几个开始"就是行号，不用序列化。
 *
 * ── 为什么认"文件名以 .vst3 结尾"就够（规格 §1.1）──────────────────────
 *   JUCE 的 VST3 宿主判断入口就是
 *       `f.hasFileExtension(".vst3") && f.exists()`
 *   也就是说非 `.vst3` 命名的二进制（实测 TyrellN6 的 bundle 里还有一个
 *   `TyrellN6.engine`）JUCE **加载不了**。为它们写兼容代码是纯浪费，
 *   所以这里只认 `.vst3`，和 JUCE 的能力边界严格对齐。
 *
 * ── bundle 与"二进制"的关系（Windows 上很容易踩）────────────────────────
 *   Windows 的 VST3 有三种落盘形态：
 *     a. bundle 目录：`Foo.vst3/Contents/x86_64-win/Foo.vst3`
 *     b. 裸 DLL：     `Foo.vst3`（直接就是一个文件）
 *     c. 上一种的 32/64 位分目录：`Foo.vst3/Contents/x86-win/Foo.vst3`
 *   VST3 SDK 的 `Module::create` 对**目录**走 bundle 分支、对**文件**走 DLL 分支，
 *   两者都能加载。所以这里统一返回 **bundle 目录**（有就返回目录，没有就返回
 *   那个文件），并且**不往 .vst3 目录里面钻**——否则同一个插件会被列两次
 *   （bundle 一次 + 里面那个二进制一次），扫出来两行一模一样的记录。
 *   macOS 的 bundle 里二进制没有 .vst3 后缀，这条规则顺手把 macOS 也覆盖了。
 *
 * ── 用法 ────────────────────────────────────────────────────────────────
 *     PluginHost::PathScanner scanner;
 *     auto result = scanner.scanAll({juce::File{"C:\\Program Files\\Common Files\\VST3"}});
 *     for (auto& f : result.modules) ... // 已去重，顺序稳定
 */
#pragma once

#include <juce_core/juce_core.h>

#include <vector>

namespace PluginHost {

    class PathScanner {
    public:
        struct Result {
            std::vector<juce::File> modules;
            int filesVisited{0};     // 看过多少个条目（诊断"扫得慢"用）
            int directoriesVisited{0};
        };

        /**
         * @brief 扫一个根目录
         *
         * @param root     不存在 / 不是目录时返回空结果，不抛异常
         * @param maxDepth 最大递归层数（防止有人把盘符根目录加进来）
         */
        Result scan(const juce::File& root, int maxDepth = 8) const;

        /** @brief 扫多个根目录并去重（同一个 bundle 被两个父目录包含时只留一份） */
        Result scanAll(const std::vector<juce::File>& roots, int maxDepth = 8) const;

    private:
        void scanDirectory(const juce::File& dir, int depth, int maxDepth, Result& out) const;

        /** @brief 名字以 .vst3 结尾（大小写不敏感，Windows 上 .VST3 也存在） */
        static bool hasVst3Extension(const juce::File& file);

        /** @brief 去重键：Windows 路径大小写不敏感，统一转小写再比 */
        static juce::String dedupeKey(const juce::File& file);
    };

} // namespace PluginHost
