#pragma once

#include "Model/EqFilterMath.hpp"
#include "Model/EqInfo.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/paramLink.hpp"
#include <cstdint>
#include <vector>

/**
 * @brief EQ 参数的下发与状态持有（Coordinator 侧）
 *
 * ── 文件意义 ──────────────────────────────────────────────────────────
 * 前端只能通过 FFI 调到这里。本类做四件事：
 *   1. 持有"当前 EQ 参数"这份唯一真相（getEqAllState 直接返回它，不用回查音频进程）；
 *   2. 按参数的更新频率选通道：
 *        拖拽中的连续量 -> OSC/UDP（ParamLink 节流 + 自动补 rev）
 *        结构变更/最终值 -> ZMQ JSON（必达、有序、作为权威值）
 *   3. 让 ParamLink 递增 rev，音频进程据此丢弃迟到的旧帧；
 *   4. 用 EqFilterMath 算频响曲线给 UI 画（和 DSP 用同一份滤波数学，曲线与实际听感一致）。
 *
 * ── 为什么状态放在这里而不是音频进程 ─────────────────────────────────
 * 所有 EQ 变更都只从这里发出，所以这里天然是"最后写入者"；
 * 前端要读回状态（切页面/重连）时直接返回 mState，避免多一次跨进程往返。
 *
 * ── 用法 ─────────────────────────────────────────────────────────────
 *     mEqController->init(oscSendLambda, jsonSendLambda);   // dllManager::init 里一次
 *     mEqController->updateBandLive(3, 1200.0f, 3.5f, 1.2f, 1.0f); // 拖拽中
 *     mEqController->setBand(band);                                 // 松手/换类型
 *     const float* curve = mEqController->getCurve(256);            // 画曲线
 */
class EqController {
public:
    /** @brief 接上两条通道；不调用的话 sendLive/sendCommit 都是空操作 */
    void init(ParamLink::OscSend osc, ParamLink::JsonSend json);

    /** @brief 拖拽中的连续量：只改 freq/gain/q/shelfSlope，走 OSC */
    void updateBandLive(int idx, float freqHz, float gainDb, float q, float shelfSlope);

    /** @brief 一条 band 的完整参数（换类型、开关、拖拽结束的最终值），走 JSON */
    void setBand(const EqBandParam& band);

    /** @brief 整机开关 + 输出增益，走 JSON */
    void setMaster(bool enabled, float masterGainDb);

    /** @brief 整机参数（预设/初始化），走 JSON */
    void setAll(const EqAllParam& all);

    /** @brief 当前参数（前端读回、保存预设用） */
    const EqAllParam& getState() const noexcept { return mState; }

    /** @brief 音频进程上报的采样率（画曲线用），默认 48000 */
    void setSampleRate(double sampleRate);

    /**
     * @brief 计算整机频响曲线（dB），返回内部缓冲指针，长度 = numPoints
     *
     * 频率轴：freq(i) = 20 * (20000/20)^(i/(numPoints-1))，i = 0 .. numPoints-1
     * 指针在**下一次调用**时会被覆盖，调用方拿到后要立刻拷走。
     */
    const float* getCurve(int numPoints);

private:
    // 曲线的频率轴与点数范围（只在本类用，不放到 cpp 顶层）
    static constexpr double kCurveMinHz = 20.0;
    static constexpr double kCurveMaxHz = 20000.0;
    static constexpr int kCurveMinPoints = 2;
    static constexpr int kCurveMaxPoints = 2048;

    EqAllParam mState{};
    ParamLink mLink{};
    double mSampleRate{48000.0};
    std::vector<float> mCurve;
};
