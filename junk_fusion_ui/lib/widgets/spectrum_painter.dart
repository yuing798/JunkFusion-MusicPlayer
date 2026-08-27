import 'package:flutter/material.dart';
import 'dart:math' as math;

import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/global_key_defs.dart';
import 'package:provider/provider.dart';

class SpectrumPainter extends CustomPainter {
  final List<double> data; // 原始数据（未归一化）

  SpectrumPainter(this.data);

  @override
  void paint(Canvas canvas, Size size) {
    if (data.isEmpty) return;

    // 1. 找最大值（用于归一化），防止除以0
    final maxVal = data.reduce(math.max);
    if (maxVal == 0) return;

    // 2. 计算绘图参数
    final barCount = data.length; // 128
    final space = 1.0; // 柱子之间的间隔
    final totalWidth = size.width;
    final totalHeight = size.height;

    // 每个柱子的总宽度
    final barTotalWidth = (totalWidth - (barCount - 1) * space) / barCount;
    // 限制最小宽度，避免太细
    final barWidth = math.max(barTotalWidth, 2.0);
    // 重新计算实际间隔，让柱子居中
    final realSpace = (totalWidth - barWidth * barCount) / (barCount - 1);

    // 画笔配置（带圆角）
    final paint = Paint()
      ..color = navigatorKey.currentContext!.watch<AppTheme>().colorHover
      ..style = PaintingStyle.fill;

    // 3. 开始绘制每一个柱子
    for (int i = 0; i < barCount; i++) {
      // 归一化值 (0.0 ~ 1.0)
      double normalized = data[i] / maxVal;

      // 计算柱子高度（保留底部留白，看起来更灵动）
      double barHeight = normalized * totalHeight;
      if (barHeight == 0.0) barHeight = 1.0;

      // 计算x坐标
      final x = i * (barWidth + realSpace);
      // 计算y坐标（因为Canvas原点在左上角，波形通常从底部画起）
      final y = totalHeight - barHeight;

      // 画圆角矩形
      final rect = Rect.fromLTWH(x, y, barWidth, barHeight);
      final rrect = RRect.fromRectAndRadius(
        rect,
        Radius.circular(barWidth / 2),
      );
      canvas.drawRRect(rrect, paint);
    }
  }

  @override
  bool shouldRepaint(covariant SpectrumPainter oldDelegate) {
    // 数据变了就重绘
    return oldDelegate.data != data;
  }
}
