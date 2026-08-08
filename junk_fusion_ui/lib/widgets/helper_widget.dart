import 'package:flutter/material.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';

/// =============================================================================
/// createIcon — 快捷创建 Icon 的辅助函数
///
/// ```dart
/// createIcon(Icons.play_arrow, s: 40, c: Colors.red);
/// ```
/// =============================================================================
Widget createIcon(
  IconData svg, {
  double s = 32,
  Color c = AppTheme.colorTextMain,
}) {
  return Icon(svg, size: s, color: c);
}
