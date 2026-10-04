import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';
import 'package:flutter_root_context_menu/flutter_root_context_menu.dart';

class MorePlayInfoWidget extends StatefulWidget {
  const MorePlayInfoWidget({super.key});
  @override
  State<StatefulWidget> createState() {
    return MorePlayInfoState();
  }
}

class MorePlayInfoState extends State<MorePlayInfoWidget> {
  final speedShifter = ValueNotifier<double>(1.0); //变速
  final pitchShifter = ValueNotifier<int>(0); //变调
  final speedShifterDivider = [0.5, 0.75, 1.0, 1.5, 2.0];
  final pitchShifterDivider = [-12.0, -8.0, -4.0, 0.0, 4.0, 8.0, 12.0];

  @override
  void dispose() {
    speedShifter.dispose();
    pitchShifter.dispose();

    super.dispose();
  }

  //找到刻度尺上面最近的刻度
  double _nearestRulerDivider(
    double localX,
    double width,
    List<double> listOfPreset,
  ) {
    final usableWidth = width - 20 * 2;
    if (usableWidth <= 0) return listOfPreset.first;

    final ratio = ((localX - 20) / usableWidth).clamp(0.0, 1.0);
    final rawV =
        listOfPreset.first + ratio * (listOfPreset.last - listOfPreset.first);

    double nearest = listOfPreset.first;
    double minDist = double.infinity;
    for (final d in listOfPreset) {
      final dist = (d - rawV).abs();
      if (dist < minDist) {
        minDist = dist;
        nearest = d;
      }
    }
    return nearest;
  }

  ContextMenuItem _buildSpeedShifterArea() {
    return ContextMenuItem.submenu(
      label: "变速设置",
      icon: Icon(TablerIcons.arrowBadgeLeft),
      children: [
        ContextMenuItem.custom(
          customHeight: 120,

          builder: (_) {
            return ValueListenableBuilder<double>(
              valueListenable: speedShifter,
              builder: (context, speed, _) {
                return SizedBox(
                  width: 240,
                  child: Padding(
                    padding: const EdgeInsets.symmetric(
                      horizontal: 12,
                      vertical: 8,
                    ),
                    child: Column(
                      mainAxisSize: MainAxisSize.min,
                      crossAxisAlignment: CrossAxisAlignment.stretch,
                      children: [
                        // 当前倍速显示
                        Center(
                          child: Text(
                            '${speed.toStringAsFixed(2)}x',
                            style: const TextStyle(
                              fontSize: 14,
                              fontWeight: FontWeight.bold,
                            ),
                          ),
                        ),
                        const SizedBox(height: 4),

                        // 横向滑块：0.5 ~ 2.0
                        Slider(
                          min: speedShifterDivider.first,
                          max: speedShifterDivider.last,
                          value: speed.clamp(
                            speedShifterDivider.first,
                            speedShifterDivider.last,
                          ),
                          onChanged: (v) {
                            speedShifter.value = v;
                            // TODO: 调用后端接口，把变速值 v 同步给音频引擎
                            // 例如: audioEngine.setSpeed(v);
                          },
                        ),

                        // 刻度尺：点击任意位置跳转
                        SizedBox(
                          height: 30,

                          child: Padding(
                            padding: EdgeInsets.symmetric(horizontal: 5),
                            child: LayoutBuilder(
                              builder: (context, constraints) {
                                final w = constraints.maxWidth;
                                return GestureDetector(
                                  behavior: HitTestBehavior.opaque,
                                  onTapDown: (details) {
                                    final v = _nearestRulerDivider(
                                      details.localPosition.dx,
                                      w,
                                      speedShifterDivider,
                                    );
                                    speedShifter.value = v;
                                    // TODO: 调用后端接口，把变速值 v 同步给音频引擎
                                  },
                                  child: CustomPaint(
                                    size: Size(w, 30),
                                    painter: _SpeedRulerPainter(
                                      dividers: speedShifterDivider,
                                      min: speedShifterDivider.first,
                                      max: speedShifterDivider.last,
                                      current: speed,
                                    ),
                                  ),
                                );
                              },
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                );
              },
            );
          },
        ),
      ],
    );
  }

  ContextMenuItem _buildPitchShifterArea() {
    return ContextMenuItem.submenu(
      label: "变调设置",
      icon: Icon(TablerIcons.arrowBadgeLeft),
      children: [
        ContextMenuItem.custom(
          customHeight: 120,
          builder: (_) {
            return ValueListenableBuilder<int>(
              valueListenable: pitchShifter,
              builder: (context, pitchShift, _) {
                return SizedBox(
                  width: 240,
                  child: Padding(
                    padding: const EdgeInsets.symmetric(
                      horizontal: 12,
                      vertical: 8,
                    ),
                    child: Column(
                      mainAxisSize: MainAxisSize.min,
                      crossAxisAlignment: CrossAxisAlignment.stretch,
                      children: [
                        Center(
                          child: Text(
                            pitchShift == 0
                                ? '原调'
                                : '${pitchShift > 0 ? '+' : ''}$pitchShift st',
                            style: const TextStyle(
                              fontSize: 14,
                              fontWeight: FontWeight.bold,
                            ),
                          ),
                        ),
                        const SizedBox(height: 4),

                        Slider(
                          min: pitchShifterDivider.first, // -12
                          max: pitchShifterDivider.last, //  12
                          divisions: 24, // 每次 1 个半音
                          value: pitchShift.toDouble().clamp(
                            pitchShifterDivider.first,
                            pitchShifterDivider.last,
                          ),
                          onChanged: (v) {
                            // 关键：改的是 notifier，不是 pitchShift
                            pitchShifter.value = v.round();
                            // TODO: 调用后端接口
                          },
                        ),

                        SizedBox(
                          height: 30,
                          child: Padding(
                            padding: const EdgeInsets.symmetric(horizontal: 6),
                            child: LayoutBuilder(
                              builder: (context, constraints) {
                                final w = constraints.maxWidth;
                                return GestureDetector(
                                  behavior: HitTestBehavior.opaque,
                                  onTapDown: (details) {
                                    final v = _nearestRulerDivider(
                                      details.localPosition.dx,
                                      w,
                                      pitchShifterDivider,
                                    );
                                    // 关键：改的是 notifier
                                    pitchShifter.value = v.toInt();
                                    // TODO
                                  },
                                  child: CustomPaint(
                                    size: Size(w, 30),
                                    painter: _SpeedRulerPainter(
                                      dividers: pitchShifterDivider,
                                      min: pitchShifterDivider.first,
                                      max: pitchShifterDivider.last,
                                      current: pitchShift.toDouble(),
                                    ),
                                  ),
                                );
                              },
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                );
              },
            );
          },
        ),
      ],
    );
  }

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTapDown: (details) {
        showRootContextMenu(
          context: context,
          position: Offset(
            details.globalPosition.dx - 160,
            details.globalPosition.dy - 60,
          ),
          config: const ContextMenuConfig(
            submenu: SubmenuConfig(
              icon: SizedBox.shrink(),
              horizontalOffset: 10.0,
            ),
          ),
          items: [_buildSpeedShifterArea(), _buildPitchShifterArea()],
        );
      },
      child: Icon(TablerIcons.dots),
    );
  }
}

class _SpeedRulerPainter extends CustomPainter {
  _SpeedRulerPainter({
    required this.dividers,
    required this.min,
    required this.max,
    required this.current,
    this.sidePadding = 20.0, // 新增：左右预留的空白
  });

  final List<double> dividers;
  final double min;
  final double max;
  final double current;
  final double sidePadding;

  @override
  void paint(Canvas canvas, Size size) {
    final range = max - min;
    if (range <= 0) return;

    // 真正的"刻度可用宽度"
    final usableWidth = size.width - sidePadding * 2;
    if (usableWidth <= 0) return;

    final tickPaint = Paint()
      ..color = const Color(0xFF9E9E9E)
      ..strokeWidth = 1
      ..strokeCap = StrokeCap.round;

    final activePaint = Paint()
      ..color = const Color(0xFF2196F3)
      ..strokeWidth = 2
      ..strokeCap = StrokeCap.round;

    const tickTop = 0.0;
    const tickHeight = 6.0;
    const labelGap = 2.0;

    for (final d in dividers) {
      final ratio = ((d - min) / range).clamp(0.0, 1.0);
      // 关键变化：起点从 sidePadding 开始，向右缩进 usableWidth
      final x = sidePadding + ratio * usableWidth;

      final isCurrent = (d - current).abs() < 0.01;

      canvas.drawLine(
        Offset(x, tickTop),
        Offset(x, tickTop + tickHeight),
        isCurrent ? activePaint : tickPaint,
      );

      final tp = TextPainter(
        text: TextSpan(
          text: _formatLabel(d),
          style: TextStyle(
            fontSize: 10,
            color: isCurrent
                ? const Color(0xFF2196F3)
                : const Color(0xFF757575),
            fontWeight: isCurrent ? FontWeight.bold : FontWeight.normal,
          ),
        ),
        textDirection: TextDirection.ltr,
      )..layout();

      // 关键变化：直接居中画，不再 clamp
      tp.paint(
        canvas,
        Offset(x - tp.width / 2, tickTop + tickHeight + labelGap),
      );
    }
  }

  String _formatLabel(double v) {
    return v.toStringAsFixed(v == v.roundToDouble() ? 1 : 2);
  }

  @override
  bool shouldRepaint(covariant _SpeedRulerPainter old) {
    return old.current != current ||
        old.min != min ||
        old.max != max ||
        old.sidePadding != sidePadding || // 记得也加进比较
        !listEquals(old.dividers, dividers);
  }
}
