/// 频谱数据源（**预留接口，本轮不实现数据通路**）
///
/// ── 为什么先留接口 ────────────────────────────────────────────────────
/// EQ 页想把实测频谱当背景层画（pro-Q 那样），播放页以后也要同一份数据。
/// 但频谱的数据通路（大概率走共享内存而不是 JSON）还没设计，所以这一轮
/// 只约定接口：EQ 页永远只依赖 [SpectrumSource]，拿不到数据就不画背景。
///
/// 将来音频进程把 FFT 结果算好、通过共享内存送到 Coordinator，再通过 FFI
/// 回调推到 Dart 时，写一个实现类注入进来即可，页面代码一行不用改。
///
/// ── 约定 ─────────────────────────────────────────────────────────────
/// [bands] 是**从左到右的对数频段**，值域 0~1（已经做过归一化和时间平滑）。
/// 长度不限（建议 64~128），曲线的横轴会被拉伸到画布宽度。
/// 值为 null 表示"现在没有数据"，此时不画背景。
library;

import 'package:flutter/foundation.dart';

abstract class SpectrumSource {
  /// 0~1 归一化的对数频段；null = 当前没有数据
  ValueListenable<List<double>?> get bands;
}

/// 空实现：永远没有数据（EQ 页默认用它）
class EmptySpectrumSource implements SpectrumSource {
  final ValueNotifier<List<double>?> _bands = ValueNotifier<List<double>?>(null);

  @override
  ValueListenable<List<double>?> get bands => _bands;

  void dispose() => _bands.dispose();
}
