/// 环形频谱（围着专辑封面画一圈）
///
/// ── 数据 ─────────────────────────────────────────────────────────────
/// 数据来自 [FfiSpectrumSource]（共享内存经 FFI），96 个对数频段，0~1。
/// 这里用 ValueListenableBuilder 监听，只有数据变化时才重画，不 setState。
///
/// ── 画法 ─────────────────────────────────────────────────────────────
///   每个频段一个角度（从正上方开始顺时针铺满 360°），
///   从半径 [radius] 向内/外伸出一根圆头线，长度 = 值 × [maxBarLength]。
///   峰值用一个小圆点画在线的末端，慢慢缩回去。
///
/// 注意：这一圈**不跟着封面转**（封面自己在里面转），否则刻度会糊成一团。
library;

import 'dart:math' as math;

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';

class SpectrumRing extends StatelessWidget {
  const SpectrumRing({
    super.key,
    required this.bands,
    required this.peaks,
    required this.radius,
    required this.color,
    this.maxBarLength = 40,
    this.barWidth = 3,
  });

  final ValueListenable<List<double>?> bands;
  final ValueListenable<List<double>?> peaks;

  /// 圆环半径（封面半径 + 一点间距）
  final double radius;
  final Color color;
  final double maxBarLength;
  final double barWidth;

  @override
  Widget build(BuildContext context) {
    return ValueListenableBuilder<List<double>?>(
      valueListenable: bands,
      builder: (context, values, _) {
        return CustomPaint(
          size: Size.square((radius + maxBarLength) * 2),
          painter: _RingPainter(
            bands: values,
            peaks: peaks.value,
            radius: radius,
            maxBarLength: maxBarLength,
            barWidth: barWidth,
            color: color,
          ),
        );
      },
    );
  }
}

class _RingPainter extends CustomPainter {
  _RingPainter({
    required this.bands,
    required this.peaks,
    required this.radius,
    required this.maxBarLength,
    required this.barWidth,
    required this.color,
  });

  final List<double>? bands;
  final List<double>? peaks;
  final double radius;
  final double maxBarLength;
  final double barWidth;
  final Color color;

  @override
  void paint(Canvas canvas, Size size) {
    final values = bands;
    if (values == null || values.isEmpty) return;

    final center = size.center(Offset.zero);
    final barPaint = Paint()
      ..strokeWidth = barWidth
      ..strokeCap = StrokeCap.round
      ..style = PaintingStyle.stroke;

    final peakPaint = Paint()..style = PaintingStyle.fill;

    final count = values.length;

    for (int i = 0; i < count; ++i) {
      // 从正上方开始顺时针铺满一圈
      final angle = -math.pi / 2 + 2 * math.pi * i / count;
      final direction = Offset(math.cos(angle), math.sin(angle));

      final value = values[i].clamp(0.0, 1.0);
      final length = value * maxBarLength;

      // 值越大越亮：0 → 25% 透明度，1 → 100%
      barPaint.color = color.withValues(alpha: 0.25 + 0.75 * value);

      final inner = center + direction * radius;
      final outer = center + direction * (radius + length);
      canvas.drawLine(inner, outer, barPaint);

      // 峰值小点
      final peakValue = (peaks != null && i < peaks!.length)
          ? peaks![i].clamp(0.0, 1.0)
          : 0.0;
      if (peakValue > 0.02) {
        peakPaint.color = color.withValues(alpha: 0.9);
        canvas.drawCircle(
          center + direction * (radius + peakValue * maxBarLength),
          barWidth * 0.8,
          peakPaint,
        );
      }
    }
  }

  @override
  bool shouldRepaint(_RingPainter oldDelegate) {
    return oldDelegate.bands != bands ||
        oldDelegate.peaks != peaks ||
        oldDelegate.radius != radius ||
        oldDelegate.color != color;
  }
}
