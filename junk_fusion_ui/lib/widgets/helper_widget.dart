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
  double size_ = 32,
  Color color_ = AppTheme.colorTextMain,
}) {
  return Icon(svg, size: size_, color: color_);
}

class RectIconButton extends StatelessWidget {
  final IconData iconData;
  final VoidCallback onPressed;

  final String? tooltip;
  final Color? hoverColor;

  // 3. 构造函数（使用 super.key 支持 Key）
  const RectIconButton({
    super.key,
    required this.iconData,
    required this.onPressed,
    this.tooltip,
    this.hoverColor,
  });

  @override
  Widget build(BuildContext context) {
    return IconButton(
      icon: createIcon(iconData),
      onPressed: onPressed,
      // padding: EdgeInsets.all(0),
      tooltip: tooltip, // 如果为 null，IconButton 会自动忽略 tooltip
      style: IconButton.styleFrom(
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.zero, // 矩形
        ),
        hoverColor: hoverColor ?? AppTheme.colorHover,
      ),
    );
  }
}
