import 'package:flutter/material.dart';

/// AppTheme 类 — 存放应用级别的主题常量和 ThemeData
abstract class AppTheme {
  /// 主要背景颜色（对应 --color-main: #f0f0f0）
  static const Color colorMain = Color(0xFFF0F0F0);

  /// 导航窗颜色（对应 --color-nav: #e2e4e4）
  static const Color colorNav = Color(0xFFE2E4E4);

  /// 悬浮颜色（对应 --color-hover: #c5c4c4）
  static const Color colorHover = Color(0xFFC5C4C4);

  /// 单元格背景色（对应 --color-cell: #f2fcff）
  static const Color colorCell = Color(0xFFF2FCFF);

  /// 鼠标按下去时的颜色（对应 --color-clicked: #b0c4de）
  static const Color colorClicked = Color(0xFFB0C4DE);

  /// 边框颜色（对应 --color-edge: #b2b1b1）
  static const Color colorEdge = Color(0xFFB2B1B1);

  /// 主要文本颜色（对应 --color-text-main: #222222）
  static const Color colorTextMain = Color(0xFF222222);

  /// 次级文本颜色（对应 --color-text-second: #555555）
  static const Color colorTextSecond = Color(0xFF555555);

  /// 强调色（对应 --color-stress: #00f2fe）
  static const Color colorStress = Color(0xFF00F2FE);

  /// 超级强调色 / 渐变辅色（对应 --color-super-stress: #4facfe）
  static const Color colorSuperStress = Color(0xFF4FACFE);

  /// 阴影色（对应 --color-shadow: #2f76b4）
  static const Color colorShadow = Color(0xFF2F76B4);

  /// 错误提示文字颜色（对应 --color-error: #991f1f）
  static const Color colorError = Color(0xFF991F1F);

  /// 大字号 — 25px（对应 --big-font）
  static const double bigFont = 25.0;

  /// 中等字号 — 18px（对应 --mid-font）
  static const double midFont = 18.0;

  /// 小字号 — 16px（对应 --little-font）
  static const double littleFont = 16.0;

  /// 圆角半径（对应 --border-radius: 6px）
  static const double borderRadius = 6.0;

  /// 过渡动画时长（对应 --ease-time: 0.15s），单位毫秒
  static const Duration easeTime = Duration(milliseconds: 150);

  // ════════════════════════════════════════════════════════════════
  // 预定义 TextStyle（TextStyle 对象是不可变的，所以用 const）
  // ════════════════════════════════════════════════════════════════

  /// 大号粗体文本样式（用于页面标题等）
  static const TextStyle bigTextStyle = TextStyle(
    fontSize: bigFont,
    fontWeight: FontWeight.bold,
    color: colorTextMain,
  );

  /// 中等字号文本样式（用于导航按钮、歌曲名等）
  static const TextStyle midTextStyle = TextStyle(
    fontSize: midFont,
    color: colorTextMain,
  );

  /// 小号文本样式（用于次级信息、艺术家名等）
  static const TextStyle littleTextStyle = TextStyle(
    fontSize: littleFont,
    color: colorTextSecond,
  );
}
