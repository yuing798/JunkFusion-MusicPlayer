import 'package:flutter/material.dart';

/// AppTheme 类 — 存放应用级别的主题常量和 ThemeData
class AppTheme extends ChangeNotifier {
  /// 主要背景颜色（对应 --color-main: #f0f0f0）
  Color colorMain = Color(0xFFF0F0F0);

  /// 导航窗颜色（对应 --color-nav: #e2e4e4）
  Color colorNav = Color(0xFFE2E4E4);

  /// 悬浮颜色（对应 --color-hover: #c5c4c4）
  Color colorHover = Color(0xFFC5C4C4);

  /// 单元格背景色（对应 --color-cell: #f2fcff）
  Color colorCell = Color(0xFFF2FCFF);

  /// 边框颜色（对应 --color-edge: #b2b1b1）
  Color colorEdge = Color(0xFFB2B1B1);

  /// 主要文本颜色（对应 --color-text-main: #222222）
  Color colorTextMain = Color(0xFF222222);

  /// 次级文本颜色（对应 --color-text-second: #555555）
  Color colorTextSecond = Color(0xFF555555);

  /// 强调色（对应 --color-stress: #00f2fe）
  Color colorStress = Color(0xFF00F2FE);

  /// 超级强调色 / 渐变辅色（对应 --color-super-stress: #4facfe）
  Color colorSuperStress = Color(0xFF4FACFE);

  /// 阴影色（对应 --color-shadow: #2f76b4）
  Color colorShadow = Color(0xFF2F76B4);

  /// 错误提示文字颜色（对应 --color-error: #991f1f）
  Color colorError = Color(0xFF991F1F);

  //获得焦点的边框的颜色
  Color get colorFocus => colorStress;
  //comboBox的背景以及每个comboBox的下拉菜单单元格的背景色
  Color get colorComboBox => colorHover;
  //comboBox的菜单中鼠标悬浮或者按下的时候的颜色
  Color get colorComboMenuSelected => colorEdge;
  //comboBox中的字体风格
  TextStyle get comboTextStyle => midTextStyle;

  /// 大号粗体文本样式（用于页面标题等）
  TextStyle get bigTextStyle => TextStyle(
    fontSize: 23,
    fontWeight: FontWeight.bold,
    color: colorTextMain,
  );

  /// 中等字号文本样式（用于导航按钮、歌曲名等）
  TextStyle get midTextStyle => TextStyle(fontSize: 17, color: colorTextMain);

  /// 小号文本样式（用于次级信息、艺术家名等）
  TextStyle get littleTextStyle =>
      TextStyle(fontSize: 14, color: colorTextSecond);
}
