#pragma once

#include "Model/EqInfo.hpp"
#include <algorithm>
#include <array>
#include <cmath>

/**
 * @brief EQ 滤波器的系数计算 + 频响评估（DSP 与 UI 曲线共用同一份数学）
 *
 * ── 文件意义 ──────────────────────────────────────────────────────────
 * 这份数学有两个使用者：
 *   1. 音频进程的 Eq（把 EqBandParam 算成 biquad 系数，交给音频线程）；
 *   2. Coordinator 的 EqController（把同一份参数算成频响曲线，给 Flutter 画）。
 * 如果两边各写一套 RBJ 公式，加一种滤波器形状时迟早会只改一边，
 * 于是曲线和实际听感对不上——所以这里只留一份实现。
 *
 * ── 为什么放在 Model 而不是 Utils ────────────────────────────────────
 * 本文件依赖 EqBandParam/BandType（在 Model 里）。CMake 里 Models 链接 Utils，
 * 依赖方向是 Utils -> Model 的单向，所以不能把本文件放进 Utils。
 * Models 被 JunkFusionCoordinator 和 JunkFusionAudioProcess 同时链接，正好是共享层。
 *
 * 全部为 static 内联函数（类内定义），无需 .cpp。
 *
 * ── 用法 ─────────────────────────────────────────────────────────────
 *     std::array<EqFilterMath::Coefficients, EqFilterMath::kMaxSections> sections;
 *     EqFilterMath::buildBand(band, sampleRate, sections);      // 参数 -> 系数
 *     double db = EqFilterMath::sectionsMagnitudeDb(sections, 1000.0, sampleRate);
 */
struct EqFilterMath {
    static constexpr int kMaxSections = 2; // Tile 与 24dB/oct 切需要两段
    static constexpr double kPi = 3.14159265358979323846;

    /** @brief 一个二阶节的系数（已除过 a0）：y = b0x + b1x1 + b2x2 - a1y1 - a2y2 */
    struct Coefficients {
        float b0{1.0f}, b1{0.0f}, b2{0.0f}, a1{0.0f}, a2{0.0f};
    };

    /** @brief 直通 */
    static Coefficients makeIdentity() noexcept { return Coefficients{}; }

    static Coefficients
        normalize(double b0, double b1, double b2, double a0, double a1, double a2) noexcept {
        Coefficients c{};
        // a0 为 0 或 NaN 时退化成直通，绝不能把无效系数交给音频线程
        if (!(std::abs(a0) > 1.0e-12)) return c;

        const double inv = 1.0 / a0;
        c.b0 = static_cast<float>(b0 * inv);
        c.b1 = static_cast<float>(b1 * inv);
        c.b2 = static_cast<float>(b2 * inv);
        c.a1 = static_cast<float>(a1 * inv);
        c.a2 = static_cast<float>(a2 * inv);
        return c;
    }

    /** @brief 钟型（RBJ peaking EQ） */
    static Coefficients makePeaking(double freqHz, double q, double gainDb, double fs) noexcept {
        const double w0 = 2.0 * kPi * freqHz / fs;
        const double cosw = std::cos(w0);
        const double sinw = std::sin(w0);
        const double a = std::pow(10.0, gainDb / 40.0);
        const double alpha = sinw / (2.0 * q);

        return normalize(
            1.0 + alpha * a,
            -2.0 * cosw,
            1.0 - alpha * a,
            1.0 + alpha / a,
            -2.0 * cosw,
            1.0 - alpha / a
        );
    }

    static Coefficients makeLowPass(double freqHz, double q, double fs) noexcept {
        const double w0 = 2.0 * kPi * freqHz / fs;
        const double cosw = std::cos(w0);
        const double sinw = std::sin(w0);
        const double alpha = sinw / (2.0 * q);
        const double cw = (1.0 - cosw) * 0.5;

        return normalize(cw, 1.0 - cosw, cw, 1.0 + alpha, -2.0 * cosw, 1.0 - alpha);
    }

    static Coefficients makeHighPass(double freqHz, double q, double fs) noexcept {
        const double w0 = 2.0 * kPi * freqHz / fs;
        const double cosw = std::cos(w0);
        const double sinw = std::sin(w0);
        const double alpha = sinw / (2.0 * q);
        const double cw = (1.0 + cosw) * 0.5;

        return normalize(cw, -(1.0 + cosw), cw, 1.0 + alpha, -2.0 * cosw, 1.0 - alpha);
    }

    static Coefficients makeNotch(double freqHz, double q, double fs) noexcept {
        const double w0 = 2.0 * kPi * freqHz / fs;
        const double cosw = std::cos(w0);
        const double sinw = std::sin(w0);
        const double alpha = sinw / (2.0 * q);

        return normalize(1.0, -2.0 * cosw, 1.0, 1.0 + alpha, -2.0 * cosw, 1.0 - alpha);
    }

    static Coefficients makeBandPass(double freqHz, double q, double fs) noexcept {
        const double w0 = 2.0 * kPi * freqHz / fs;
        const double cosw = std::cos(w0);
        const double sinw = std::sin(w0);
        const double alpha = sinw / (2.0 * q);

        // 0 dB 峰值增益的带通
        return normalize(alpha, 0.0, -alpha, 1.0 + alpha, -2.0 * cosw, 1.0 - alpha);
    }

    static Coefficients makeAllPass(double freqHz, double q, double fs) noexcept {
        const double w0 = 2.0 * kPi * freqHz / fs;
        const double cosw = std::cos(w0);
        const double sinw = std::sin(w0);
        const double alpha = sinw / (2.0 * q);

        return normalize(
            1.0 - alpha,
            -2.0 * cosw,
            1.0 + alpha,
            1.0 + alpha,
            -2.0 * cosw,
            1.0 - alpha
        );
    }

    static Coefficients makeLowShelf(double freqHz, double slope, double gainDb, double fs
    ) noexcept {
        const double w0 = 2.0 * kPi * freqHz / fs;
        const double cosw = std::cos(w0);
        const double sinw = std::sin(w0);
        const double a = std::pow(10.0, gainDb / 40.0);
        const double s = std::clamp(slope, 0.1, 1.0);

        // RBJ 的 shelf 斜率公式
        const double alpha = sinw * 0.5 * std::sqrt((a + 1.0 / a) * (1.0 / s - 1.0) + 2.0);
        const double beta = 2.0 * std::sqrt(a) * alpha;

        return normalize(
            a * ((a + 1.0) - (a - 1.0) * cosw + beta),
            2.0 * a * ((a - 1.0) - (a + 1.0) * cosw),
            a * ((a + 1.0) - (a - 1.0) * cosw - beta),
            (a + 1.0) + (a - 1.0) * cosw + beta,
            -2.0 * ((a - 1.0) + (a + 1.0) * cosw),
            (a + 1.0) + (a - 1.0) * cosw - beta
        );
    }

    static Coefficients makeHighShelf(double freqHz, double slope, double gainDb, double fs
    ) noexcept {
        const double w0 = 2.0 * kPi * freqHz / fs;
        const double cosw = std::cos(w0);
        const double sinw = std::sin(w0);
        const double a = std::pow(10.0, gainDb / 40.0);
        const double s = std::clamp(slope, 0.1, 1.0);

        const double alpha = sinw * 0.5 * std::sqrt((a + 1.0 / a) * (1.0 / s - 1.0) + 2.0);
        const double beta = 2.0 * std::sqrt(a) * alpha;

        return normalize(
            a * ((a + 1.0) + (a - 1.0) * cosw + beta),
            -2.0 * a * ((a - 1.0) + (a + 1.0) * cosw),
            a * ((a + 1.0) + (a - 1.0) * cosw - beta),
            (a + 1.0) - (a - 1.0) * cosw + beta,
            2.0 * ((a - 1.0) - (a + 1.0) * cosw),
            (a + 1.0) - (a - 1.0) * cosw - beta
        );
    }

    /**
     * @brief 整体倾斜（FlatTile）：全频段固定斜率、两端不归平
     *
     * 一阶低通把信号分成低/高两半再分别乘增益，传递函数可写成
     *   y[n] = b0*x[n] + b1*x[n-1] + p*y[n-1]
     * 直流增益 = gLow，奈奎斯特增益 = gHigh，中间平滑过渡。
     */
    static Coefficients makeFlatTile(double freqHz, double gainDb, double fs) noexcept {
        const double gLow = std::pow(10.0, -gainDb * 0.5 / 20.0);
        const double gHigh = std::pow(10.0, gainDb * 0.5 / 20.0);

        const double a = 1.0 - std::exp(-2.0 * kPi * freqHz / fs);
        const double p = 1.0 - a;

        const double b0 = a * gLow + (1.0 - a) * gHigh;
        const double b1 = -p * gHigh;
        const double a1 = -p;

        return normalize(b0, b1, 0.0, 1.0, a1, 0.0);
    }

    /** @brief 把一条 band 的参数译成 1~2 个二阶节（关掉的 band 全是直通） */
    static void buildBand(
        const EqBandParam& param,
        double fs,
        std::array<Coefficients, kMaxSections>& out
    ) {
        out[0] = makeIdentity();
        out[1] = makeIdentity();

        if (!param.enabled || !(fs > 0.0)) return;

        const double freq =
            std::clamp(static_cast<double>(param.freqHz), 10.0, fs * 0.49);
        const double q = std::clamp(static_cast<double>(param.q), 0.1, 18.0);
        const double gainDb = std::clamp(static_cast<double>(param.gainDb), -30.0, 30.0);
        const double slope = std::clamp(static_cast<double>(param.shelfSlope), 0.1, 1.0);

        switch (param.type) {
            case BandType::Bell:
                out[0] = makePeaking(freq, q, gainDb, fs);
                break;
            case BandType::LowPass:
                out[0] = makeLowPass(freq, q, fs);
                if (param.cutSlopeDbPerOct >= 24) out[1] = out[0]; // 24dB/oct = 两段级联
                break;
            case BandType::HighPass:
                out[0] = makeHighPass(freq, q, fs);
                if (param.cutSlopeDbPerOct >= 24) out[1] = out[0];
                break;
            case BandType::Notch:
                out[0] = makeNotch(freq, q, fs);
                break;
            case BandType::BandPass:
                out[0] = makeBandPass(freq, q, fs);
                break;
            case BandType::AllPass:
                out[0] = makeAllPass(freq, q, fs);
                break;
            case BandType::LowShelf:
                out[0] = makeLowShelf(freq, slope, gainDb, fs);
                break;
            case BandType::HighShelf:
                out[0] = makeHighShelf(freq, slope, gainDb, fs);
                break;
            case BandType::Tile: {
                // 双边反向搁架：低频降一半 + 高频升一半，两端各自变平，占 2 段
                const double half = gainDb * 0.5;
                out[0] = makeLowShelf(freq, slope, -half, fs);
                out[1] = makeHighShelf(freq, slope, half, fs);
                break;
            }
            case BandType::FlatTile:
                // TODO: 这里按"全频段固定斜率、两端不归平"实现（一阶高/低通并联）。
                // 如果你想要的是别的曲线（比如两端也要归平），改 makeFlatTile 就行。
                out[0] = makeFlatTile(freq, gainDb, fs);
                break;
            default:
                break;
        }
    }

    /** @brief dB 转线性增益 */
    static float dbToLinear(float db) noexcept {
        return static_cast<float>(std::pow(10.0, static_cast<double>(db) * 0.05));
    }

    /**
     * @brief 单个二阶节在 freqHz 处的幅度响应（dB），与滤波器类型无关
     *
     * ⚠️ 必须用复数形式直接求 H(e^jw) = B/A，**不要**用 |B|²/|A|² 的"平方和展开式"。
     * 低频段（远离中心频率、|H|≈1）时分子分母都趋近 0，展开式里的加减抵消会把
     * float 系数的舍入误差放大几十倍：曲线会抖成 0dB 杂波、偶发尖峰，
     * EQ 点设在 100Hz 以下时特别明显（实测 85Hz 的钟型能冲出画布）。
     * 复数形式只有一次累加，误差不会被平方放大。
     */
    static double magnitudeDb(const Coefficients& c, double freqHz, double fs) noexcept {
        if (!(fs > 0.0)) return 0.0;

        const double w = 2.0 * kPi * freqHz / fs;
        const double cosW = std::cos(w);
        const double sinW = std::sin(w);
        // 倍角公式，省一次三角函数调用
        const double cos2W = cosW * cosW - sinW * sinW;
        const double sin2W = 2.0 * cosW * sinW;

        // 分子：b0 + b1·e^-jw + b2·e^-2jw
        const double numRe = c.b0 + c.b1 * cosW + c.b2 * cos2W;
        const double numIm = -(c.b1 * sinW + c.b2 * sin2W);
        // 分母：1 + a1·e^-jw + a2·e^-2jw
        const double denRe = 1.0 + c.a1 * cosW + c.a2 * cos2W;
        const double denIm = -(c.a1 * sinW + c.a2 * sin2W);

        const double numMag = numRe * numRe + numIm * numIm;
        const double denMag = denRe * denRe + denIm * denIm;

        const double safeNum = std::max(numMag, 1.0e-30);
        const double safeDen = std::max(denMag, 1.0e-30);
        return 10.0 * std::log10(safeNum / safeDen);
    }

    /** @brief 多个二阶节级联后的总响应（dB） */
    static double sectionsMagnitudeDb(
        const std::array<Coefficients, kMaxSections>& sections,
        double freqHz,
        double fs
    ) noexcept {
        return magnitudeDb(sections[0], freqHz, fs) + magnitudeDb(sections[1], freqHz, fs);
    }
};
