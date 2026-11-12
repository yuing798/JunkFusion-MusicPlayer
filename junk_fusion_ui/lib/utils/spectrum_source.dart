/// 频谱数据源
///
/// ── 数据从哪来 ────────────────────────────────────────────────────────
/// 音频进程的分析线程做 FFT + 对数分频（96 段）+ 平滑，写进一块**共享内存**；
/// Coordinator 创建这块内存、按 seqlock 读快照；Dart 通过 FFI 每帧取一次。
///
/// 这条链路刻意绕开了 ZMQ：频谱是 50Hz 的连续流，走消息队列要多一次
/// 序列化和线程切换，延迟和抖动都不划算。
///
/// ── 用法 ─────────────────────────────────────────────────────────────
///     final source = FfiSpectrumSource();
///     // 画图：source.bands / source.peaks 都是 ValueListenable<List<double>?>
///     source.dispose();   // 离开页面时必须调：会关掉后端的分析
///
/// ── 约定 ─────────────────────────────────────────────────────────────
/// [bands] 是**从左到右的对数频段**，值域 0~1（后端已经做过攻击/释放平滑）。
/// 长度 = 96（SpectrumShm::kBandCount），曲线的横轴被拉伸到画布宽度。
/// 值为 null 表示"现在没有数据"（没在播放 / 还没接上共享内存）。
/// [peaks] 是峰值保持值，衰减较慢，用来画"峰值小点"。
library;

import 'dart:async';
import 'dart:ffi';

import 'package:flutter/foundation.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';

abstract class SpectrumSource {
  /// 0~1 归一化的对数频段；null = 当前没有数据
  ValueListenable<List<double>?> get bands;

  /// 峰值保持值（同样 96 个）；null = 当前没有数据
  ValueListenable<List<double>?> get peaks;

  /// 停掉轮询并通知后端关闭分析
  void dispose();
}

/// 空实现：永远没有数据（不接频谱的页面用它）
class EmptySpectrumSource implements SpectrumSource {
  final ValueNotifier<List<double>?> _bands = ValueNotifier<List<double>?>(null);
  final ValueNotifier<List<double>?> _peaks = ValueNotifier<List<double>?>(null);

  @override
  ValueListenable<List<double>?> get bands => _bands;

  @override
  ValueListenable<List<double>?> get peaks => _peaks;

  @override
  void dispose() {
    _bands.dispose();
    _peaks.dispose();
  }
}

/// 从共享内存（经 FFI）读实时频谱
///
/// 每隔 [interval] 调一次 `getSpectrumFrame()`，拷贝 96 个 float 到
/// ValueNotifier。FFI 调用本身是 µs 级，60fps 完全没有压力。
///
/// TODO: 现在"谁创建谁开关"，多个页面同时用会互相把分析关掉。
///       等播放页和 EQ 页都要用的时候，再在这里加引用计数。
class FfiSpectrumSource implements SpectrumSource {
  FfiSpectrumSource({Duration interval = const Duration(milliseconds: 16)}) {
    // 打开后端的频谱分析（关掉能省 FFT 开销）
    bindings.setSpectrumEnabled(1);
    _timer = Timer.periodic(interval, (_) => _poll());
  }

  final ValueNotifier<List<double>?> _bands = ValueNotifier<List<double>?>(null);
  final ValueNotifier<List<double>?> _peaks = ValueNotifier<List<double>?>(null);

  Timer? _timer;
  bool _disposed = false;

  @override
  ValueListenable<List<double>?> get bands => _bands;

  @override
  ValueListenable<List<double>?> get peaks => _peaks;

  void _poll() {
    if (_disposed) return;

    final framePointer = bindings.getSpectrumFrame();
    if (framePointer == nullptr) {
      // 没数据：通知订阅者清空，免得画面冻在最后一帧
      if (_bands.value != null) _bands.value = null;
      if (_peaks.value != null) _peaks.value = null;
      return;
    }

    final bandCount = bindings.getSpectrumBandCount();
    if (bandCount <= 0) return;

    // 指针指向 Coordinator 内部缓冲，下次调用会被覆盖，必须立刻拷走
    _bands.value = framePointer.asTypedList(bandCount).toList(growable: false);

    final peaksPointer = bindings.getSpectrumPeaks();
    if (peaksPointer != nullptr) {
      _peaks.value = peaksPointer.asTypedList(bandCount).toList(growable: false);
    }
  }

  @override
  void dispose() {
    if (_disposed) return;
    _disposed = true;

    _timer?.cancel();
    _timer = null;

    // 关掉后端分析，别让它空转
    bindings.setSpectrumEnabled(0);

    _bands.dispose();
    _peaks.dispose();
  }
}
