/**
 * @brief 频谱共享内存的数据布局（音频进程写 / Coordinator 读）
 *
 * ── 谁写谁读 ──────────────────────────────────────────────────────────
 *   音频进程：分析线程算好 96 个对数频段的幅度，按 seqlock 写进来；
 *   Coordinator：每帧按 seqlock 读一次，通过 FFI 给前端；
 *   前端：只拿 float 数组，不关心这块内存怎么来的。
 *
 * ── 为什么是 seqlock 而不是锁 ─────────────────────────────────────────
 * 写方是实时线程（分析线程 30~50Hz），读方是 UI（60fps）。用互斥量会
 * 让读方可能阻塞写方；seqlock 的做法是"写完再验一次序号"，读方偶尔
 * 读到半帧就重试，写方永远不等读方。
 *
 * ── 布局 ──────────────────────────────────────────────────────────────
 *   Header：magic / version / bandCount / seq / sampleRate / updatedMs / enabled
 *   bands[96]：0~1 归一化，已做攻击/释放平滑
 *   peaks[96]：峰值保持（会缓慢衰减），前端可以直接画"峰值点"
 *
 * ⚠️ magic 最后写入，作为"初始化完成"的标志；读方先验 magic 再读数据。
 */
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace SpectrumShm {

// 'J' 'F' 'S' '1' 四个 ASCII 字节（小端下的 uint32）
inline constexpr uint32_t kMagic = 0x3146534Au;
inline constexpr uint32_t kVersion = 1u;

/// 频段数量（前端画环形/条形都用这个）
inline constexpr uint32_t kBandCount = 96u;

/// 频段覆盖范围（Hz），实际计算时上界会被采样率钳制
inline constexpr double kMinHz = 20.0;
inline constexpr double kMaxHz = 20000.0;

struct Header {
    /// seqlock 序号：写前 ++（奇数），写完 ++（偶数）
    std::atomic<uint32_t> seq;
    uint32_t magic;
    uint32_t version;
    uint32_t bandCount;
    float sampleRate;
    uint32_t updatedMs;
    uint32_t enabled;
    uint32_t reserved;
};

struct Frame {
    Header header;
    float bands[kBandCount];
    float peaks[kBandCount];
};

inline constexpr size_t kByteSize = sizeof(Frame);

/// 写开始：序号变奇数，标记"正在写"
inline void writeBegin(Frame& frame) noexcept {
    frame.header.seq.fetch_add(1, std::memory_order_acq_rel);
    std::atomic_thread_fence(std::memory_order_release);
}

/// 写结束：序号变偶数，标记"这一帧稳定了"
inline void writeEnd(Frame& frame) noexcept {
    std::atomic_thread_fence(std::memory_order_release);
    frame.header.seq.fetch_add(1, std::memory_order_acq_rel);
}

/**
 * @brief 读开始：序号是偶数才说明没有正在写
 *
 * @param seqOut 拿到的序号，读完之后要传给 readEnd
 * @return false 表示正在写，这一帧跳过
 */
inline bool readBegin(const Frame& frame, uint32_t& seqOut) noexcept {
    seqOut = frame.header.seq.load(std::memory_order_acquire);
    return (seqOut & 1u) == 0u;
}

/// 读结束：序号没变说明这一帧没被写方打断，可以放心用
inline bool readEnd(const Frame& frame, uint32_t seqValue) noexcept {
    std::atomic_thread_fence(std::memory_order_acquire);
    return frame.header.seq.load(std::memory_order_relaxed) == seqValue;
}

/**
 * @brief 校验这块内存是不是我们要的、而且已经初始化好了
 */
inline bool isValidFrame(const Frame& frame) noexcept {
    return frame.header.magic == kMagic && frame.header.version == kVersion &&
           frame.header.bandCount == kBandCount;
}

/**
 * @brief 初始化（创建方调用）：把所有字段清零后填好，magic 最后写
 */
inline void initialise(Frame& frame) noexcept {
    std::memset(&frame, 0, sizeof(Frame));
    frame.header.seq.store(0, std::memory_order_relaxed);
    frame.header.version = kVersion;
    frame.header.bandCount = kBandCount;
    frame.header.sampleRate = 0.0f;
    frame.header.updatedMs = 0;
    frame.header.enabled = 0;
    // magic 最后写：读方看到 magic 就认为整块内存已经可用了
    frame.header.magic = kMagic;
}

} // namespace SpectrumShm
