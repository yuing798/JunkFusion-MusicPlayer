import 'package:flutter/widgets.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';

Widget createIcon(
  IconData svg, {
  double s = 32,
  Color c = AppTheme.colorTextMain,
}) {
  return Icon(svg, size: s, color: c);
}
