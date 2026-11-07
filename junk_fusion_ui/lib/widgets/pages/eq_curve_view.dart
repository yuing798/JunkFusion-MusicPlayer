/// EQ 曲线视图：网格 + 频响曲线 + 可拖拽的 band 圆点
///
/// ── 交互 ──────────────────────────────────────────────────────────────
///   拖圆点        → 改频率(x) + 增益(y)；拖动中走 OSC(updateBandLive)，松手走 JSON(commitBand)
///   滚轮在圆点上  → 改 Q 或斜率（架式/倾斜类改斜率），直接走 JSON
///   双击圆点      → 开关这一段
///   双击空白处    → 在鼠标位置激活一条还没用的 band
///   单击空白处    → 取消选中
///
/// ── 曲线从哪来 ────────────────────────────────────────────────────────
/// 不是本地算的，也不是从频谱拿的：后端 EqController 用和 DSP 完全同一份
/// 滤波数学（Model/EqFilterMath.hpp）算出 dB 数组，这里只负责画。
/// 频谱背景是另一回事，通过 [spectrum] 注入，本轮没有实现数据通路。
library;

import 'dart:math' as math;

import 'package:flutter/foundation.dart';
import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/eq_param.dart';
import 'package:junk_fusion_ui/providers/eq_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/spectrum_source.dart';
import 'package:provider/provider.dart';

class EqCurveView extends StatefulWidget {
  const EqCurveView({super.key, this.spectrum});

  /// 频谱背景数据源（预留接口）；null = 不画背景
  final SpectrumSource? spectrum;

  /// 画布纵轴范围（dB）。和 inspector 的增益滑杆保持同一范围
  static const double minDb = -18;
  static const double maxDb = 18;

  /// 横轴范围（Hz）
  static const double minHz = 20;
  static const double maxHz = 20000;

  @override
  State<EqCurveView> createState() => _EqCurveViewState();
}

class _EqCurveViewState extends State<EqCurveView> {
  static const double _minHz = EqCurveView.minHz;
  static const double _maxHz = EqCurveView.maxHz;
  static const double _minDb = EqCurveView.minDb;
  static const double _maxDb = EqCurveView.maxDb;

  /// 正在拖的那条 band；null = 没在拖
  int? _draggingBand;

  /// 双击落点（onDoubleTapDown 记下来，onDoubleTap 里用）
  Offset? _doubleTapPosition;

  // ── 坐标换算 ──────────────────────────────────────────────────────
  double _xForFreq(double freqHz, double width) {
    final t = (math.log(freqHz / _minHz) / math.log(_maxHz / _minHz)).clamp(
      0.0,
      1.0,
    );
    return t * width;
  }

  double _freqForX(double x, double width) {
    final t = (x / width).clamp(0.0, 1.0);
    return _minHz * math.pow(_maxHz / _minHz, t).toDouble();
  }

  double _yForDb(double db, double height) {
    final t = ((_maxDb - db) / (_maxDb - _minDb)).clamp(0.0, 1.0);
    return t * height;
  }

  double _dbForY(double y, double height) {
    final t = (y / height).clamp(0.0, 1.0);
    return _maxDb - t * (_maxDb - _minDb);
  }

  /// 找鼠标位置附近最靠近的 band（关着的只能在选中时才看得见，也允许点）
  int? _bandAt(Offset local, Size size, EqProvider provider) {
    int? best;
    var bestDistance = 18.0;

    for (var i = 0; i < provider.bands.length; i++) {
      final band = provider.bands[i];
      final isVisible = band.enabled || provider.selectedBand == i;
      if (!isVisible) continue;

      final center = Offset(
        _xForFreq(band.freqHz, size.width),
        _yForDb(band.gainDb, size.height),
      );
      final distance = (center - local).distance;
      if (distance < bestDistance) {
        bestDistance = distance;
        best = i;
      }
    }
    return best;
  }

  // ── 手势 ──────────────────────────────────────────────────────────
  void _handlePanStart(DragStartDetails details, Size size) {
    final provider = context.read<EqProvider>();
    final index = _bandAt(details.localPosition, size, provider);
    if (index == null) return;

    provider.selectBand(index);
    setState(() => _draggingBand = index);
  }

  void _handlePanUpdate(DragUpdateDetails details, Size size) {
    final index = _draggingBand;
    if (index == null) return;

    final provider = context.read<EqProvider>();
    provider.dragBandLive(
      index,
      freqHz: _freqForX(details.localPosition.dx, size.width),
      gainDb: _dbForY(details.localPosition.dy, size.height),
    );
  }

  void _handlePanEnd() {
    final index = _draggingBand;
    setState(() => _draggingBand = null);
    if (index == null) return;

    // 拖拽结束必须把最终值走 JSON 定死（OSC 那一帧可能被节流丢掉）
    context.read<EqProvider>().commitBand(index);
  }

  void _handleTapUp(TapUpDetails details, Size size) {
    final provider = context.read<EqProvider>();
    provider.selectBand(_bandAt(details.localPosition, size, provider));
  }

  void _handleDoubleTap(Size size) {
    final position = _doubleTapPosition;
    if (position == null) return;

    final provider = context.read<EqProvider>();
    final index = _bandAt(position, size, provider);

    if (index != null) {
      // 双击已有的点 → 开关
      provider.setBandEnabled(index, !provider.bands[index].enabled);
      return;
    }

    // 双击空白 → 找一条还没用过的 band，放到鼠标位置并打开
    final free = provider.bands.indexWhere((band) => !band.enabled);
    if (free < 0) return;

    provider.updateBand(
      free,
      (band) => band.copyWith(
        enabled: true,
        freqHz: _freqForX(position.dx, size.width),
        gainDb: band.type.usesGain ? _dbForY(position.dy, size.height) : 0,
      ),
    );
    provider.selectBand(free);
  }

  /// 滚轮：改 Q 或斜率（架式类没有 Q，改斜率）
  void _handlePointerSignal(PointerSignalEvent event, Size size) {
    if (event is! PointerScrollEvent) return;

    final provider = context.read<EqProvider>();
    final index = _bandAt(event.localPosition, size, provider);
    if (index == null) return;

    final band = provider.bands[index];
    final step = band.type.usesQ ? 0.15 : 0.08;
    final delta = event.scrollDelta.dy > 0 ? -step : step;
    final next = (band.shapeValue + delta).clamp(band.shapeMin, band.shapeMax);

    provider.selectBand(index);
    provider.setBandShape(index, next);
  }

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<EqProvider>();
    final theme = context.watch<AppTheme>();

    return LayoutBuilder(
      builder: (context, constraints) {
        final size = Size(constraints.maxWidth, constraints.maxHeight);

        return MouseRegion(
          cursor: _draggingBand == null
              ? SystemMouseCursors.precise
              : SystemMouseCursors.grabbing,
          child: Listener(
            onPointerSignal: (event) => _handlePointerSignal(event, size),
            child: GestureDetector(
              behavior: HitTestBehavior.opaque,
              onTapUp: (details) => _handleTapUp(details, size),
              onDoubleTapDown: (details) =>
                  _doubleTapPosition = details.localPosition,
              onDoubleTap: () => _handleDoubleTap(size),
              onPanStart: (details) => _handlePanStart(details, size),
              onPanUpdate: (details) => _handlePanUpdate(details, size),
              onPanEnd: (_) => _handlePanEnd(),
              onPanCancel: _handlePanEnd,
              child: CustomPaint(
                painter: _EqCurvePainter(
                  state: provider.state,
                  curve: provider.curve,
                  selectedBand: provider.selectedBand,
                  theme: theme,
                  spectrumBands: widget.spectrum?.bands,
                ),
                child: const SizedBox.expand(),
              ),
            ),
          ),
        );
      },
    );
  }
}

// ════════════════════════════════════════════════════════════════
// 画
// ════════════════════════════════════════════════════════════════
class _EqCurvePainter extends CustomPainter {
  _EqCurvePainter({
    required this.state,
    required this.curve,
    required this.selectedBand,
    required this.theme,
    this.spectrumBands,
  });

  final EqAllParam state;
  final List<double> curve;
  final int? selectedBand;
  final AppTheme theme;

  /// 预留：频谱背景（0~1 的对数频段）；null = 不画
  final ValueListenable<List<double>?>? spectrumBands;

  static const List<double> _gridFreqs = [
    20,
    50,
    100,
    200,
    500,
    1000,
    2000,
    5000,
    10000,
    20000,
  ];

  static const List<double> _gridDbs = [
    EqCurveView.minDb,
    -12,
    -6,
    0,
    6,
    12,
    EqCurveView.maxDb,
  ];

  double _xForFreq(double freqHz, Size size) {
    final t =
        (math.log(freqHz / EqCurveView.minHz) /
                math.log(EqCurveView.maxHz / EqCurveView.minHz))
            .clamp(0.0, 1.0);
    return t * size.width;
  }

  double _yForDb(double db, Size size) {
    final t =
        ((EqCurveView.maxDb - db) / (EqCurveView.maxDb - EqCurveView.minDb))
            .clamp(0.0, 1.0);
    return t * size.height;
  }

  String _freqLabel(double freq) {
    if (freq >= 1000) {
      return '${(freq / 1000).toStringAsFixed(0)}k';
    }
    return freq.toStringAsFixed(0);
  }

  @override
  void paint(Canvas canvas, Size size) {
    if (size.width <= 0 || size.height <= 0) return;

    final rect = Offset.zero & size;

    // 背景（比主背景稍微亮一点，让曲线区有"面板"感）
    canvas.drawRect(rect, Paint()..color = theme.colorCell);

    _paintSpectrum(canvas, size);
    _paintGrid(canvas, size);
    _paintCurve(canvas, size);
    _paintBands(canvas, size);

    // 边框
    canvas.drawRect(
      rect.deflate(0.5),
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1
        ..color = theme.colorEdge,
    );
  }

  void _paintGrid(Canvas canvas, Size size) {
    final thin = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1
      ..color = theme.colorEdge.withValues(alpha: 0.35);

    final zero = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1.2
      ..color = theme.colorEdge.withValues(alpha: 0.75);

    final labelStyle = TextStyle(fontSize: 10, color: theme.colorTextSecond);

    for (final freq in _gridFreqs) {
      final x = _xForFreq(freq, size);
      canvas.drawLine(Offset(x, 0), Offset(x, size.height), thin);

      final painter = TextPainter(
        text: TextSpan(text: _freqLabel(freq), style: labelStyle),
        textDirection: TextDirection.ltr,
      )..layout();
      painter.paint(canvas, Offset(x + 3, size.height - painter.height - 2));
    }

    for (final db in _gridDbs) {
      final y = _yForDb(db, size);
      canvas.drawLine(
        Offset(0, y),
        Offset(size.width, y),
        db == 0 ? zero : thin,
      );

      if (db == EqCurveView.minDb || db == EqCurveView.maxDb) continue;

      final painter = TextPainter(
        text: TextSpan(
          text: db > 0 ? '+${db.toStringAsFixed(0)}' : db.toStringAsFixed(0),
          style: labelStyle,
        ),
        textDirection: TextDirection.ltr,
      )..layout();
      painter.paint(canvas, Offset(3, y - painter.height - 1));
    }
  }

  /// 预留：频谱背景。本轮没有数据通路，spectrumBands 为 null 时整段跳过
  void _paintSpectrum(Canvas canvas, Size size) {
    final listenable = spectrumBands;
    if (listenable == null) return;

    final bands = listenable.value;
    if (bands == null || bands.isEmpty) return;

    final path = Path()..moveTo(0, size.height);
    for (var i = 0; i < bands.length; i++) {
      final x = size.width * i / (bands.length - 1);
      final y = size.height * (1 - bands[i].clamp(0.0, 1.0));
      path.lineTo(x, y);
    }
    path
      ..lineTo(size.width, size.height)
      ..close();

    canvas.drawPath(
      path,
      Paint()..color = theme.colorSuperStress.withValues(alpha: 0.12),
    );
  }

  void _paintCurve(Canvas canvas, Size size) {
    if (curve.length < 2) return;

    final line = Path();
    for (var i = 0; i < curve.length; i++) {
      final x = size.width * i / (curve.length - 1);
      final y = _yForDb(curve[i], size);
      if (i == 0) {
        line.moveTo(x, y);
      } else {
        line.lineTo(x, y);
      }
    }

    // 曲线和 0dB 线之间的填充，让形状更好读
    final fill = Path.from(line)
      ..lineTo(size.width, _yForDb(0, size))
      ..lineTo(0, _yForDb(0, size))
      ..close();

    canvas.drawPath(
      fill,
      Paint()..color = theme.colorStress.withValues(alpha: 0.10),
    );
    canvas.drawPath(
      line,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 2
        ..color = theme.colorStress,
    );
  }

  void _paintBands(Canvas canvas, Size size) {
    for (var i = 0; i < state.bands.length; i++) {
      final band = state.bands[i];
      final isSelected = selectedBand == i;
      if (!band.enabled && !isSelected) continue;

      final center = Offset(
        _xForFreq(band.freqHz, size),
        _yForDb(band.gainDb, size),
      );
      final radius = isSelected ? 9.0 : 7.0;

      final fillColor = !band.enabled
          ? Colors.transparent
          : (isSelected ? theme.colorSuperStress : theme.colorStress);

      if (band.enabled) {
        canvas.drawCircle(center, radius, Paint()..color = fillColor);
      }

      canvas.drawCircle(
        center,
        radius,
        Paint()
          ..style = PaintingStyle.stroke
          ..strokeWidth = isSelected ? 2.5 : 1.5
          ..color = isSelected ? theme.colorTextMain : theme.colorSuperStress,
      );

      // 段号
      final painter = TextPainter(
        text: TextSpan(
          text: '${i + 1}',
          style: TextStyle(
            fontSize: 10,
            fontWeight: FontWeight.bold,
            color: band.enabled ? theme.colorTextMain : theme.colorTextSecond,
          ),
        ),
        textDirection: TextDirection.ltr,
      )..layout();
      painter.paint(
        canvas,
        center + Offset(-painter.width / 2, -painter.height / 2),
      );
    }
  }

  @override
  bool shouldRepaint(_EqCurvePainter oldDelegate) {
    return oldDelegate.state != state ||
        oldDelegate.curve != curve ||
        oldDelegate.selectedBand != selectedBand ||
        oldDelegate.spectrumBands != spectrumBands ||
        oldDelegate.theme != theme;
  }
}
