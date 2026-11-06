#pragma once

#include "Utils/Yvar.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

/**
 * @brief EQ 均衡器的参数模型 + 跨进程通信协议
 *
 * ── 文件意义 ──────────────────────────────────────────────────────────
 * Coordinator(UI 进程) 和 AudioProcessor(音频进程) 都要知道"一条 EQ band 有哪些字段"
 * 以及"这些字段怎么序列化"。为了让两边不各写一套字符串，这里把
 * **参数结构 + 报文编解码** 放在同一个类型里：字符串 key 只出现在 EqInfo.cpp。
 *
 * BandType 也放在这里而不是 DSP 层：它属于"参数模型"（要被序列化、要存预设），
 * 不是一个实现细节。DSP 层的 Eq 反过来 include 本文件。
 *
 * ── 通信协议 ─────────────────────────────────────────────────────────
 *  1. 拖拽中的连续量(高频) —— OSC/UDP，地址 /eq/band/<idx>，参数依次是:
 *         freqHz, gainDb, q, shelfSlope, rev(int32)
 *
 *  2. 结构变更 / 拖拽结束的最终值 / 预设 —— TCP JSON(ZMQ PUSH)，信封:
 *         {"eqBand":   {...一条 band...},          "rev": 12}
 *         {"eqMaster": {"on":true,"gainDb":0.0},   "rev": 13}
 *         {"eqAll":    {"on":true,...bands...},    "rev": 14}
 *
 * rev 是全局单调递增的版本号(由发送方 ParamLink 生成)。接收方丢弃 rev <= 已见过的报文，
 * 用于防止"UDP 迟到的一帧盖掉 TCP 的最终值"。rev == 0 表示无版本信息，无条件应用。
 *
 * ── 用法 ─────────────────────────────────────────────────────────────
 *   发送(Coordinator):
 *       Yvar payload = EqWire::encodeBand(param);
 *       // 交给 ParamLink::sendCommit("eqBand", payload)，它会自己补 rev
 *   接收(AudioProcessor):
 *       if (auto msg = EqWire::parse(envelope)) { ... msg->kind / msg->band ... }
 */

/** @brief 一条 band 的滤波器形状（wire 格式里用字符串表示，见 typeName） */
enum class BandType : uint8_t {
    Bell,      // 钟型（峰值）
    LowPass,   // 低通
    HighPass,  // 高通
    Notch,     // 陷波
    BandPass,  // 带通
    AllPass,   // 全通
    LowShelf,  // 低架
    HighShelf, // 高架
    Tile,      // 双边反向搁架：低频降、高频升，两端各自变平（占 2 段 biquad）
    FlatTile,  // 整体倾斜：全频段固定斜率，两端不归平（占 1 段 biquad）
};

/**
 * @brief 一条 band 的全部参数(超集)
 *
 * 为什么一个结构体装下所有类型的参数：音频线程不能用运行时多态，
 * "band 是什么形状"只在算系数的那一刻用 switch 分派一次，
 * 之后音频线程只看到 5 个 biquad 系数，不再关心类型。
 *
 * 各字段的适用类型：
 *   freqHz              —— 所有类型
 *   gainDb              —— Bell / LowShelf / HighShelf / Tile / FlatTile
 *   q                   —— Bell / Notch / BandPass / AllPass / LowPass / HighPass(谐振量)
 *   shelfSlope          —— LowShelf / HighShelf / Tile(RBJ 公式里的 S，0.1~1.0 保证单调)
 *   cutSlopeDbPerOct    —— LowPass / HighPass(12 = 1 段，24 = 2 段级联)
 */
struct EqBandParam {
    static constexpr int kMaxBands = 8;

    // 单条报文里的 band 序号；在 eqAll 的 bands 数组里以数组下标为准，此字段被忽略
    int idx{0};
    bool enabled{false};
    BandType type{BandType::Bell};
    float freqHz{1000.0f};
    float gainDb{0.0f};
    float q{0.70710678f};
    float shelfSlope{1.0f};
    int cutSlopeDbPerOct{12};

    /** @brief 序列化用的类型名（"bell" / "lowShelf" / ...） */
    const char* typeName() const;

    /** @brief 解析类型名，认不出来的返回 nullopt（调用方保持原类型即可） */
    static std::optional<BandType> typeFromName(std::string_view name);
};

/** @brief 整机参数：8 条 band + 旁通 + 输出增益。这份 JSON 本身就是预设格式 */
struct EqAllParam {
    bool enabled{true};
    float masterGainDb{0.0f};
    std::array<EqBandParam, EqBandParam::kMaxBands> bands{};
};

/** @brief EQ 报文编解码：字符串 key 只在本类的 .cpp 里出现 */
struct EqWire {
    enum class Kind { none, band, master, all };

    struct Message {
        Kind kind{Kind::none};
        uint32_t rev{0};
        EqBandParam band{};
        bool masterEnabled{true};
        float masterGainDb{0.0f};
        EqAllParam all{};
    };

    /** @brief 解析一条报文信封；不是 EQ 报文返回 nullopt */
    static std::optional<Message> parse(const Yvar& envelope);

    /** @brief 编码一条 band 的参数体（不含信封、不含 rev） */
    static Yvar encodeBand(const EqBandParam& band);
    /** @brief 编码整机参数体（不含信封、不含 rev） */
    static Yvar encodeAll(const EqAllParam& all);
    /** @brief 编码整机开关 + 输出增益 */
    static Yvar encodeMaster(bool enabled, float masterGainDb);

    /** @brief 从参数体解析出整机参数（缺字段走默认值，不抛异常） */
    static EqAllParam decodeAll(const Yvar& body);
    /** @brief 从参数体解析出一条 band */
    static EqBandParam decodeBand(const Yvar& body);

    /**
     * @brief 音频进程上报采样率（prepareToPlay / 换设备时发一次）
     *
     * Coordinator 画频响曲线要用它；不依赖它也能画，只是高频段会有一点偏差。
     */
    static Yvar encodeSampleRate(double sampleRate);
    /** @brief 解析采样率报文；不是这种报文返回 nullopt */
    static std::optional<double> decodeSampleRate(const Yvar& envelope);
};
