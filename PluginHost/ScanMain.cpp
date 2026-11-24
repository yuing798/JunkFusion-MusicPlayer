/**
 * @brief `JunkFusionPluginScan.exe` 的入口 —— 批量扫描里的**临时子进程**
 *
 * ── 它是什么 ────────────────────────────────────────────────────────────
 * 一个只会"枚举 .vst3 元数据"的极简可执行文件。它由 Coordinator 按批次拉起，
 * 干完活就退出，**不和任何其它进程通信**（结果全部写进一个文本文件，见下）。
 *
 * ── 为什么必须是独立进程（规格 §3.3）────────────────────────────────────
 *   枚举本身就会 `LoadLibrary` 那个插件 DLL。坏插件在枚举阶段照样能把进程带走
 *   ——如果它和音频进程在同一个进程里，用户会当场听到播放中断/整个播放器消失。
 *   所以扫描进程是**可以随便死**的，死了主进程重新拉一个继续扫剩下的。
 *
 * ── 命令行 ──────────────────────────────────────────────────────────────
 *   扫描模式（一批模块）：
 *     --list <文件>        主进程写好的待扫模块列表，一行一个绝对路径（UTF-8）
 *     --start-index <n>    从这个下标开始（断点续扫；主进程崩溃恢复靠它）
 *     --progress <文件>    结果写这里（一行一个事件，格式见 PluginScanProtocol.hpp）
 *     --instance           可选：额外实例化一次拿**实测**通道数（规格 §3.2，
 *                          只用于排查，默认不开）
 *
 *   预检模式（单个插件，host-integration.md §7.1）：
 *     --preflight          进入预检模式
 *     --file <路径>        插件模块
 *     --identifier <id>    PluginDescription::createIdentifierString()
 *     --report <文件>      预检结果写这里（UTF-8 JSON，格式见 runPreflight）
 *     --sample-rate <hz>   可选，默认 44100
 *     --block <n>          可选，默认 441（真实设备块大小）
 *
 *   预检模式的退出码：0 = 写出过报告（不管 ok 是 true 还是 false）；
 *   非 0 / 进程消失 = 插件把子进程带走了（崩溃），父进程按"崩溃"处理。
 *
 * ── 两个刻意的设计 ──────────────────────────────────────────────────────
 *   1. **不往 stdout 输出结果**：第三方插件会往 stdout 乱打东西，而且如果主进程
 *      用管道收，插件打满 64KB 管道缓冲就会把子进程卡死。这里启动时把自己的
 *      stdout/stderr 全部重定向到 NUL，结果只走文件。
 *   2. **不用 argv**：缓存目录在中文用户名下是 `C:\Users\张三\...`，而
 *      `main(argc, argv)` 拿到的是 ANSI 串，会乱码。改用 JUCE 的
 *      `getCommandLineParameterArray()`，它从宽字符命令行取参数。
 *
 * ── 退出码 ──────────────────────────────────────────────────────────────
 *   0 = 本批次跑完了（不管其中个别插件是不是失败，失败会写成 FAIL 事件）
 *   2 = 参数不对 / 列表读不出来（主进程会把它判成"整批异常"）
 */
#include "./PluginCategory.hpp"
#include "./PluginEnumerate.hpp"
#include "./PluginScanProtocol.hpp"

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

class ScanRunner {
public:
    int run(const juce::StringArray& args) {
        redirectStandardStreams();
        if (!parseCommandLine(args)) return 2;

        // 预检模式：一个插件一次，写完报告就退出（报告里带 ok=false 也算正常退出）
        if (mPreflightMode) return runPreflight();

        if (!loadFileList()) return 2;
        if (!openProgressFile()) return 2;

        runBatch();
        return 0;
    }

private:
    bool mPreflightMode{false};
    juce::File mPluginFile;
    juce::String mIdentifier;
    juce::File mReportFile;
    double mPreflightSampleRate{44100.0};
    int mPreflightBlockSize{441};

    /**
     * @brief 预检：在**本进程**里实例化一次，把结果写进 --report
     *
     * 报告一定在实例析构**之前**落盘：有的插件析构会挂死（实验台：Dexed），
     * 先写报告的话，父进程至少知道"它本身是好的，只是收不干净"。
     */
    int runPreflight() {
        if (mReportFile == juce::File{}) return 2;

        mReportFile.deleteFile();
        juce::FileOutputStream out{mReportFile, 4096};
        if (!out.openedOk()) return 2;

        auto writeReport = [this, &out](const PluginHost::Enumerator::PreflightResult& result) {
            juce::String json;
            json << "{\"ok\":" << (result.ok ? "true" : "false")
                 << ",\"error\":\"" << escapeJSON(result.error) << "\""
                 << ",\"name\":\"" << escapeJSON(result.name) << "\""
                 << ",\"inputs\":" << result.inputChannels
                 << ",\"outputs\":" << result.outputChannels
                 << ",\"latency\":" << result.latencySamples
                 << ",\"params\":" << result.numParameters
                 << ",\"hasEditor\":" << (result.hasEditor ? "true" : "false") << "}";

            const auto utf8 = json.toStdString();
            out.write(utf8.data(), utf8.size());
            out.write("\n", 1);
            out.flush();
            // 不打日志：扫描子进程的 stdout 被丢到 NUL，spdlog 也没初始化，
            // 结果只走 --report 文件（父进程读它）。
        };

        PluginHost::Enumerator enumerator;
        enumerator.preflight(
            mPluginFile,
            mIdentifier.toStdString(),
            mPreflightSampleRate,
            mPreflightBlockSize,
            writeReport
        );

        return 0;
    }

    /** @brief JSON 字符串转义（只处理会破坏 JSON 的几个字符，足够我们的字段用）
     *
     * 注意按**字节**处理：juce::String 是 UTF-16，直接 `out << c` 会把一个
     * UTF-8 字节当成一个字符（中文错误信息会变成"妯″潡..."那种乱码）。
     * 这里先拼 std::string 字节，最后整体 fromUTF8。
     */
    static juce::String escapeJSON(const std::string& text) {
        std::string out;
        out.reserve(text.size() + 8);

        for (unsigned char c : text) {
            switch (c) {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (c < 0x20) break; // 控制字符丢掉
                    out += static_cast<char>(c);
                    break;
            }
        }

        return juce::String::fromUTF8(out.c_str());
    }

    /**
     * @brief 把自己的 stdout/stderr 丢掉
     *
     * 插件往 stdout/stderr 乱打会污染控制台，也可能填满父进程的管道缓冲把它卡死。
     * 结果只走 --progress 文件，所以这里把标准流直接换成 NUL。
     */
    static void redirectStandardStreams() {
#if JUCE_WINDOWS
        FILE* dummy = nullptr;
        freopen_s(&dummy, "NUL", "w", stdout);
        freopen_s(&dummy, "NUL", "w", stderr);
#else
        std::freopen("/dev/null", "w", stdout);
        std::freopen("/dev/null", "w", stderr);
#endif
    }

    // ── 命令行 ──
    juce::File mListFile;
    juce::File mProgressFile;
    int mStartIndex{0};
    bool mInstanceMode{false};

    // ── 运行时 ──
    juce::StringArray mModules;
    std::unique_ptr<juce::FileOutputStream> mOut;

    bool parseCommandLine(const juce::StringArray& args) {
        // 参数由 main/wmain 传进来（已经是 UTF-8/宽字符转换后的结果）。
        // 不能用 JUCEApplicationBase::getCommandLineParameterArray()：
        // 它读的是 juce_argc/juce_argv，那是 START_JUCE_APPLICATION 生成入口时
        // 才填的；我们这种裸 main 的 console 程序里它恒为空数组。
        for (int i = 0; i < args.size(); ++i) {
            const auto& arg = args[i];
            const bool hasValue = (i + 1) < args.size();

            if (arg == "--list" && hasValue) {
                mListFile = juce::File{args[++i]};
            } else if (arg == "--start-index" && hasValue) {
                mStartIndex = juce::jmax(0, args[++i].getIntValue());
            } else if (arg == "--progress" && hasValue) {
                mProgressFile = juce::File{args[++i]};
            } else if (arg == "--instance") {
                mInstanceMode = true;
            } else if (arg == "--preflight") {
                mPreflightMode = true;
            } else if (arg == "--file" && hasValue) {
                mPluginFile = juce::File{args[++i]};
            } else if (arg == "--identifier" && hasValue) {
                mIdentifier = args[++i];
            } else if (arg == "--report" && hasValue) {
                mReportFile = juce::File{args[++i]};
            } else if (arg == "--sample-rate" && hasValue) {
                mPreflightSampleRate = juce::jmax(8000.0, args[++i].getDoubleValue());
            } else if (arg == "--block" && hasValue) {
                mPreflightBlockSize = juce::jlimit(16, 8192, args[++i].getIntValue());
            }
        }

        if (mPreflightMode) {
            return mPluginFile != juce::File{} && mReportFile != juce::File{};
        }

        return mListFile != juce::File{} && mProgressFile != juce::File{};
    }

    bool loadFileList() {
        if (!mListFile.existsAsFile()) return false;
        // fromLines 会顺手吃掉 \r 和空行，Windows / Unix 换行都能读
        mModules = juce::StringArray::fromLines(mListFile.loadFileAsString());
        // 主进程写列表时末尾一定带一个换行，而 StringArray::addLines 会在
        // 结尾补一个**空串**。不清掉的话最后一个下标是空路径，会多出一条
        // "模块不存在"的假失败记录（实测踩过）。
        mModules.removeEmptyStrings();
        return true;
    }

    bool openProgressFile() {
        mProgressFile.deleteFile();
        mOut = std::make_unique<juce::FileOutputStream>(mProgressFile, 4096);
        return mOut != nullptr && mOut->openedOk();
    }

    void emit(const std::string& line) {
        if (mOut == nullptr) return;
        // 直接写字节：编码那边产出的已经是 UTF-8，交给 writeText 反而要再过一遍
        // String 转换（而且它的第四个参数 lineEndings 会改我的换行）
        mOut->write(line.data(), line.size());
        mOut->write("\n", 1);
        // 每个事件立刻落盘：主进程靠"文件里有没有新行"判断进度，
        // 攒着不写会让 15s 超时把正常插件误杀。
        mOut->flush();
    }

    void runBatch() {
        const int total = mModules.size();
        if (mStartIndex >= total) {
            emit(PluginHost::encodeDone(0));
            return;
        }

        PluginHost::Enumerator enumerator;
        int processed = 0;

        for (int index = mStartIndex; index < total; ++index) {
            const juce::File module{mModules[index]};
            scanOne(enumerator, index, module);
            ++processed;
        }

        emit(PluginHost::encodeDone(processed));
    }

    void scanOne(
        PluginHost::Enumerator& enumerator,
        int index,
        const juce::File& module
    ) {
        std::vector<PluginHost::Record> records;
        std::string error;

        // 第三方 DLL 在静态初始化阶段崩溃是常态，但 C++ 异常不常见；
        // 这里仍然兜一层，避免一个插件的异常弄掉整批。
        bool ok = false;
        try {
            ok = enumerator.enumerate(module, records, error);
        } catch (const std::exception& e) {
            ok = false;
            error = std::string{"枚举抛出异常: "} + e.what();
        } catch (...) {
            ok = false;
            error = "枚举抛出未知异常";
        }

        if (!ok || records.empty()) {
            emit(PluginHost::encodeFail(index, error.empty() ? "枚举失败" : error));
            return;
        }

        for (auto& record : records) {
            const auto decision = PluginHost::classifyMeta(record.meta);
            record.type = decision.type;
            record.rule = decision.rule;

            // 规格 §3.2 的可选路径：拿实测通道数覆盖元数据（会慢 2.6 倍）
            if (mInstanceMode) {
                int measuredIn = 0;
                int measuredOut = 0;
                std::string measureError;
                if (enumerator.measureChannels(
                        module,
                        record.meta.identifier,
                        measuredIn,
                        measuredOut,
                        measureError
                    )) {
                    record.meta.numInputs = measuredIn;
                    record.meta.numOutputs = measuredOut;
                } else {
                    emit(PluginHost::encodeLog(
                        "实例化失败 " + record.meta.name + ": " + measureError
                    ));
                }
            }

            emit(PluginHost::encodeOk(index, record));
        }

        // 显式的"这个下标成功了"：主进程靠它推进断点，
        // 不靠"又收到一条 OK"去猜上一个文件有没有扫完（见 Protocol 头文件）。
        emit(PluginHost::encodeEnd(index));
    }
};

// ── 入口 ────────────────────────────────────────────────────────────────
//
// Windows 下刻意用 wmain + /ENTRY:wmainCRTStartup（见 CMakeLists）：
//   · 缓存目录在中文用户名下是 `C:\Users\张三\AppData\...`；
//   · `main(argc, argv)` 拿到的是**系统 ANSI 代码页**(GBK)的字节，按 UTF-8 解
//     会变成乱码，按 GBK 解又只能在中文系统上用；
//   · 只有 `wmain` 能拿到 UTF-16 的原始命令行，转成 juce::String 就永远正确。
#if JUCE_WINDOWS
int wmain(int argc, wchar_t** argv) {
    juce::StringArray args;
    for (int i = 1; i < argc; ++i) {
        args.add(juce::String{argv[i]});
    }

    ScanRunner runner;
    return runner.run(args);
}
#else
int main(int argc, char** argv) {
    juce::StringArray args;
    for (int i = 1; i < argc; ++i) {
        args.add(juce::String::fromUTF8(argv[i]));
    }

    ScanRunner runner;
    return runner.run(args);
}
#endif
