/// KnobControl — VST 风格的旋钮
///
/// ── 为什么用它 ────────────────────────────────────────────────────────
/// 一排参数（EQ 每段 3~4 个、音箱阵列更多）用滑杆时，"标签 + 数值 + 滑杆"
/// 挤在一行很容易溢出错位；旋钮把三者竖着排，"旋钮在上、标签在下"，
/// 宽度固定、高度固定，一排能塞下更多参数，也是 VST 插件的通用交互。
///
/// ── 用法 ─────────────────────────────────────────────────────────────
///     KnobControl(
///       label: '频率',
///       value: band.freqHz,
///       min: 20,
///       max: 20000,
///       defaultValue: 1000,
///       curve: KnobCurve.logarithmic,   // 频率类参数用对数，否则 1kHz 挤在最左边
///       valueText: '85 Hz',
///       onChanged: (v) => provider.dragBandLive(index, freqHz: v),
///       onChangeEnd: (v) => provider.commitBand(index),
///     )
///
/// ── 交互（VST 惯例）──────────────────────────────────────────────────
///   - 竖直拖动改值：向上增大，鼠标位移约 150px 走完全量程
///   - 按住 Shift 拖动：精细调整（1/10 灵敏度）
///   - 双击：恢复 [defaultValue]
///   - 滚轮：按 1% 量程微调
///
/// 拖动中会连续回调 [onChanged]，松手时回调 [onChangeEnd] —— 这一点很重要：
/// EQ 那边靠它区分"OSC 高频下发"和"松手 JSON 定死"。
library;

import 'dart:math' as math;

import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:provider/provider.dart';

/// 旋钮的取值曲线
enum KnobCurve {
  /// 线性：数值 = min + t * (max - min)
  linear,

  /// 对数：适合频率、Q 这类听感上是倍频关系的参数
  logarithmic,
}

class KnobControl extends StatefulWidget {
  const KnobControl({
    super.key,
    required this.label,
    required this.value,
    required this.min,
    required this.max,
    required this.defaultValue,
    required this.onChanged,
    this.onChangeEnd,
    this.valueText,
    this.curve = KnobCurve.linear,
    this.diameter = 38,
    this.enabled = true,
  });

  final String label;
  final double value;
  final double min;
  final double max;

  /// 双击复位到的值
  final double defaultValue;
  final ValueChanged<double> onChanged;
  final ValueChanged<double>? onChangeEnd;

  /// 显示在标签下面的数值文本（调用方决定单位和小数位）
  final String? valueText;
  final KnobCurve curve;
  final double diameter;
  final bool enabled;

  @override
  State<KnobControl> createState() => _KnobControlState();
}

class _KnobControlState extends State<KnobControl> {
  /// 走完全量程需要的鼠标位移（像素）
  static const double _fullRangePixels = 150;

  /// 精细调整的倍率（按住 Shift）
  static const double _fineFactor = 0.1;

  bool _isDragging = false;
  bool _isFine = false;

  // ── 数值 ↔ 0~1 ────────────────────────────────────────────────────
  double _toNormalized(double value) {
    final clamped = value.clamp(widget.min, widget.max);

    if (widget.curve == KnobCurve.logarithmic) {
      if (widget.min <= 0 || widget.max <= widget.min) return 0;
      return (math.log(clamped / widget.min) /
              math.log(widget.max / widget.min))
          .clamp(0.0, 1.0);
    }

    if (widget.max == widget.min) return 0;
    return ((clamped - widget.min) / (widget.max - widget.min)).clamp(
      0.0,
      1.0,
    );
  }

  double _fromNormalized(double t) {
    final value = switch (widget.curve) {
      KnobCurve.logarithmic =>
        widget.min * math.pow(widget.max / widget.min, t).toDouble(),
      KnobCurve.linear => widget.min + t * (widget.max - widget.min),
    };
    return value.clamp(widget.min, widget.max);
  }

  // ── 手势 ──────────────────────────────────────────────────────────
  void _applyDrag(double deltaY, {required bool commit}) {
    if (!widget.enabled) return;

    final pixels = _isFine ? _fullRangePixels / _fineFactor : _fullRangePixels;
    // 向上拖（deltaY 为负）要增大，所以取反
    final next = (_toNormalized(widget.value) - deltaY / pixels).clamp(0.0, 1.0);
    final value = _fromNormalized(next);

    widget.onChanged(value);
    if (commit) widget.onChangeEnd?.call(value);
  }

  void _handleWheel(PointerSignalEvent event) {
    if (!widget.enabled || event is! PointerScrollEvent) return;

    final step = event.scrollDelta.dy > 0 ? -0.01 : 0.01;
    final next = (_toNormalized(widget.value) + step).clamp(0.0, 1.0);
    final value = _fromNormalized(next);

    widget.onChanged(value);
    widget.onChangeEnd?.call(value);
  }

  void _handleDoubleTap() {
    if (!widget.enabled) return;
    widget.onChanged(widget.defaultValue);
    widget.onChangeEnd?.call(widget.defaultValue);
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final normalized = _toNormalized(widget.value);

    final labelColor = widget.enabled
        ? theme.colorTextMain
        : theme.colorTextSecond;

    return Listener(
      onPointerSignal: _handleWheel,
      child: MouseRegion(
        cursor: widget.enabled
            ? (_isDragging
                  ? SystemMouseCursors.grabbing
                  : SystemMouseCursors.resizeUpDown)
            : SystemMouseCursors.basic,
        child: GestureDetector(
          behavior: HitTestBehavior.opaque,
          onVerticalDragStart: (_) {
            _isFine = HardwareKeyboard.instance.isShiftPressed;
            setState(() => _isDragging = true);
          },
          onVerticalDragUpdate: (details) =>
              _applyDrag(details.delta.dy, commit: false),
          onVerticalDragEnd: (_) {
            setState(() => _isDragging = false);
            widget.onChangeEnd?.call(widget.value);
          },
          onVerticalDragCancel: () {
            setState(() => _isDragging = false);
            widget.onChangeEnd?.call(widget.value);
          },
          onDoubleTap: _handleDoubleTap,
          child: SizedBox(
            width: widget.diameter + 26,
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                SizedBox(
                  width: widget.diameter,
                  height: widget.diameter,
                  child: CustomPaint(
                    painter: _KnobPainter(
                      normalized: normalized,
                      trackColor: theme.colorEdge,
                      arcColor: widget.enabled
                          ? theme.colorSuperStress
                          : theme.colorEdge,
                      pointerColor: labelColor,
                      isActive: _isDragging,
                    ),
                  ),
                ),
                const SizedBox(height: 2),
                Text(
                  widget.label,
                  style: theme.littleTextStyle.copyWith(
                    color: labelColor,
                    fontSize: 12,
                  ),
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
                if (widget.valueText != null)
                  Text(
                    widget.valueText!,
                    style: theme.littleTextStyle.copyWith(
                      fontSize: 11,
                      color: theme.colorTextSecond,
                    ),
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

// ════════════════════════════════════════════════════════════════
// 画旋钮：270° 圆弧（135° → 405°）+ 指针
// ════════════════════════════════════════════════════════════════
class _KnobPainter extends CustomPainter {
  _KnobPainter({
    required this.normalized,
    required this.trackColor,
    required this.arcColor,
    required this.pointerColor,
    required this.isActive,
  });

  final double normalized;
  final Color trackColor;
  final Color arcColor;
  final Color pointerColor;
  final bool isActive;

  /// 起点 135°、总扫过 270°（Flutter 的角度 0 在 3 点钟方向、顺时针为正）
  static const double _startAngle = 3 * math.pi / 4;
  static const double _sweepAngle = 3 * math.pi / 2;

  @override
  void paint(Canvas canvas, Size size) {
    final center = size.center(Offset.zero);
    final radius = size.shortestSide / 2 - 3;
    if (radius <= 0) return;

    final rect = Rect.fromCircle(center: center, radius: radius);
    final stroke = isActive ? 3.5 : 3.0;

    // 底轨
    canvas.drawArc(
      rect,
      _startAngle,
      _sweepAngle,
      false,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = stroke
        ..strokeCap = StrokeCap.round
        ..color = trackColor,
    );

    // 值弧（从起点画到当前值）
    if (normalized > 0.001) {
      canvas.drawArc(
        rect,
        _startAngle,
        _sweepAngle * normalized,
        false,
        Paint()
          ..style = PaintingStyle.stroke
          ..strokeWidth = stroke
          ..strokeCap = StrokeCap.round
          ..color = arcColor,
      );
    }

    // 中心小圆
    canvas.drawCircle(
      center,
      radius * 0.55,
      Paint()..color = arcColor.withValues(alpha: 0.10),
    );

    // 指针
    final angle = _startAngle + _sweepAngle * normalized;
    final direction = Offset(math.cos(angle), math.sin(angle));
    canvas.drawLine(
      center + direction * (radius * 0.25),
      center + direction * (radius * 0.82),
      Paint()
        ..color = pointerColor
        ..strokeWidth = 2.2
        ..strokeCap = StrokeCap.round,
    );
  }

  @override
  bool shouldRepaint(_KnobPainter oldDelegate) {
    return oldDelegate.normalized != normalized ||
        oldDelegate.trackColor != trackColor ||
        oldDelegate.arcColor != arcColor ||
        oldDelegate.pointerColor != pointerColor ||
        oldDelegate.isActive != isActive;
  }
}
