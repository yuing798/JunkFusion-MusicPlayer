#pragma once

#include "Utils/Yvar.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <string>

/**
 * @brief 参数下发通道（发送侧策略层）
 *
 * ── 文件意义 ──────────────────────────────────────────────────────────
 * 项目里所有"可以拖拽的参数"（滑块、EQ 的 band 点……）都需要同一套下发策略：
 *
 *   - 连续量（拖拽中）:  走 OSC/UDP，允许丢帧、以最新一帧为准，必须节流；
 *   - 离散量/最终值   :  走 JSON(ZMQ PUSH)，必达、有序，作为权威值；
 *   - 每条报文带一个全局单调递增的 rev，接收方丢弃旧帧，
 *     防止"松手之后迟到的 UDP 帧把最终值盖掉"。
 *
 * 把这三件事收在一个地方，其它参数只要"填地址 + 填数值"即可，不用各自实现节流。
 *
 * ── 边界 ─────────────────────────────────────────────────────────────
 * 本类**不持有 socket**：Utils 被两个进程共用，音频进程不该链接发送端代码。
 * 实际发送通过注入的两个回调完成（Coordinator 侧接 OscSender / AudioProcessPusher）。
 *
 * ── 用法 ─────────────────────────────────────────────────────────────
 *     ParamLink link;
 *     link.setTransports(
 *         [](const char* addr, const float* args, int n, uint32_t rev) { ...OSC... },
 *         [](const Yvar& payload) { ...ZMQ PUSH... });
 *
 *     link.sendLive("/eq/band/3", {1200.0f, 3.5f, 1.2f, 1.0f});  // 拖拽中，自动节流
 *     link.sendCommit("eqBand", EqWire::encodeBand(param));       // 松手，立即发
 *
 * ── 节流策略 ─────────────────────────────────────────────────────────
 * **丢弃式**：距离上次发送不足 1/rateLimitHz 秒就直接丢弃这一帧，不排队、不补发。
 * 之所以安全，是因为一次拖拽的结尾一定有一条 sendCommit 把最终值定死。
 * 若某一路参数只有 sendLive 没有 sendCommit，请不要靠调低 rateLimitHz 来"保证不丢"，
 * 而是补一条 sendCommit —— 这是本类契约的一部分。
 */
class ParamLink {
public:
    /** @brief OSC 发送：地址、float 参数数组、参数个数、版本号（写在最后一个参数） */
    using OscSend =
        std::function<void(const char* address, const float* args, int numArgs, uint32_t rev)>;
    /** @brief JSON 发送：已经带好 rev 的完整信封 */
    using JsonSend = std::function<void(const Yvar& envelope)>;

    /** @brief 接上两条通道；任一参数留空表示该通道不可用 */
    void setTransports(OscSend osc, JsonSend json);

    /** @brief 节流频率，默认 120Hz */
    void setRateLimitHz(double hz);
    double getRateLimitHz() const noexcept;

    /** @brief 拖拽中的连续量：可能被节流丢弃 */
    void sendLive(const char* address, std::initializer_list<float> values);

    /** @brief 结构变更/最终值：不节流，立即发送，信封为 {"<key>": <payload>, "rev": n} */
    void sendCommit(const char* key, const Yvar& payload);

    /** @brief OSC 通道是否可用（不可用时 sendLive 直接返回，不会崩） */
    bool hasOsc() const noexcept { return static_cast<bool>(mOsc); }

private:
    struct Slot {
        // 存 std::string 而不是 const char*：调用方可能用栈上的临时地址缓冲，
        // 存指针会悬垂。这里最多 32 个地址，一次分配、后续只做字符串比较。
        std::string address;
        double lastSentSec{-1.0};
    };
    static constexpr int kMaxSlots = 32;

    /** @brief 找到（或分配）该地址对应的节流槽位；槽位满时返回 nullptr（表示不节流，直发） */
    Slot* findSlot(const char* address);

    OscSend mOsc;
    JsonSend mJson;
    double mMinIntervalSec{1.0 / 120.0};
    uint32_t mRev{0};
    std::array<Slot, kMaxSlots> mSlots{};
};
