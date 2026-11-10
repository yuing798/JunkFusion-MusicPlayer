/// app_theme.dart — 全局主题（flex_color_scheme 驱动）
///
/// ── 设计 ──────────────────────────────────────────────────────────────
/// 用 `flex_color_scheme` 生成 **Material 3 ColorScheme**，再由 [AppTheme]
/// 把它的语义色映射成项目里一直在用的旧名字（colorMain / colorCell / ...）。
/// 这样做的好处：
///   · 页面里几百处 `theme.colorXxx` 一行都不用改；
///   · 颜色是"算"出来的（surface / surfaceContainer / outline / onSurface
///     以及它们的对比度），亮暗两套自动一致，不用手调；
///   · 以后加主题切换只需要换 `themeMode`。
///
/// ── 两套配色 ──────────────────────────────────────────────────────────
///   明亮（默认）：蓝白   primary = 蓝，背景接近白
///   暗黑：黑金           primary = 金，darkIsTrueBlack 纯黑底
///
/// ── 用法 ─────────────────────────────────────────────────────────────
///     // main.dart
///     theme: AppTheme.lightThemeData,
///     darkTheme: AppTheme.darkThemeData,
///     themeMode: appTheme.themeMode,
///
///     // 页面里
///     final theme = context.watch<AppTheme>();
///     Container(color: theme.colorCell, child: Text(..., style: theme.midTextStyle))
///
/// ── 切换逻辑（还没做）────────────────────────────────────────────────
/// 以后设置页只需要调 `appTheme.setThemeMode(...)`，MaterialApp 那边
/// 已经接了 `themeMode`，会整棵树重建。
library;

import 'package:flex_color_scheme/flex_color_scheme.dart';
import 'package:flutter/material.dart';

// ════════════════════════════════════════════════════════════════
// 两套配色的种子色
//
// flex_color_scheme 会从这几个颜色推导出完整的 ColorScheme：
// 背景、卡片、边框、文本、容器、禁用态全都是算出来的，
// 所以这里只写"性格色"，不要在这里做细节微调。
// ════════════════════════════════════════════════════════════════

/// 明亮模式：蓝白
const FlexSchemeColor _blueWhiteColors = FlexSchemeColor(
  primary: Color.fromARGB(255, 134, 188, 234), // 蓝：沿用原来 --color-shadow 的主蓝
  primaryContainer: Color(0xFFCFE6FF),
  secondary: Color(0xFF4FACFE), // 浅蓝：原来的 --color-super-stress
  secondaryContainer: Color(0xFFDCEFFF),
  tertiary: Color(0xFF00B8C4),
  tertiaryContainer: Color(0xFFD2F7FA),
  appBarColor: Color(0xFFF7FBFF),
);

/// 暗黑模式：黑金
const FlexSchemeColor _blackGoldColors = FlexSchemeColor(
  primary: Color(0xFFD4AF37), // 金
  primaryContainer: Color(0xFF3B2F13),
  secondary: Color(0xFFE8C96A), // 浅金
  secondaryContainer: Color(0xFF2E2614),
  tertiary: Color(0xFFB8860B),
  tertiaryContainer: Color(0xFF2A2009),
  appBarColor: Color(0xFF0D0D0D),
);

class AppTheme extends ChangeNotifier {
  /// 默认主题：蓝白
  static const ThemeMode defaultThemeMode = ThemeMode.light;

  static const String _fontFamily = 'OpenSans';
  static const List<String> _fontFamilyFallback = <String>['NotoSansSC'];

  /// 明亮主题（蓝白）
  static final ThemeData lightThemeData = FlexThemeData.light(
    colors: _blueWhiteColors,
    useMaterial3: true,
    fontFamily: _fontFamily,
    fontFamilyFallback: _fontFamilyFallback,
  );

  /// 暗黑主题（黑金）
  ///
  /// darkIsTrueBlack = true：背景用纯黑，金色才显得"亮"。
  static final ThemeData darkThemeData = FlexThemeData.dark(
    colors: _blackGoldColors,
    darkIsTrueBlack: true,
    useMaterial3: true,
    fontFamily: _fontFamily,
    fontFamilyFallback: _fontFamilyFallback,
  );

  ThemeMode _themeMode;
  ColorScheme _scheme;

  AppTheme({ThemeMode mode = defaultThemeMode})
    : _themeMode = mode,
      _scheme =
          (mode == ThemeMode.dark ? darkThemeData : lightThemeData).colorScheme;

  // ── 主题模式 ──────────────────────────────────────────────────────
  ThemeMode get themeMode => _themeMode;

  /// 当前生效的 Material 3 色板（想直接用 M3 语义色的地方读这个）
  ColorScheme get colorScheme => _scheme;

  /// 切换主题（设置页以后调；现在没人调，默认蓝白）
  void setThemeMode(ThemeMode mode) {
    if (mode == _themeMode) return;
    _themeMode = mode;
    _syncColorScheme();
    notifyListeners();
  }

  void _syncColorScheme() {
    _scheme = (_themeMode == ThemeMode.dark ? darkThemeData : lightThemeData)
        .colorScheme;
  }

  // ── 背景色 ────────────────────────────────────────────────────────
  /// 页面主背景
  Color get colorMain => _scheme.surface;

  /// 导航栏背景（比主背景略深一档）
  Color get colorNav => _scheme.surfaceContainer;

  /// 悬浮态背景
  Color get colorHover => _scheme.surfaceContainerHigh;

  /// 列表行 / 卡片背景（比主背景略亮一档）
  Color get colorCell => _scheme.surfaceContainerLowest;

  /// 边框
  Color get colorEdge => _scheme.outlineVariant;

  // ── 文本 ──────────────────────────────────────────────────────────
  /// 主要文本
  Color get colorTextMain => _scheme.onSurface;

  /// 次级文本
  Color get colorTextSecond => _scheme.onSurfaceVariant;

  /// 主色背景上的文本
  Color get colorOnStress => _scheme.onPrimary;

  // ── 强调色 ────────────────────────────────────────────────────────
  /// 强调色（开关、选中、主按钮）
  Color get colorStress => _scheme.primary;

  /// 次强调色（渐变副色、次要标记）
  Color get colorSuperStress => _scheme.secondary;

  /// 阴影
  Color get colorShadow => _scheme.shadow;

  /// 错误
  Color get colorError => _scheme.error;

  /// 「喜欢」的红心：两套主题下都要够醒目，所以固定一个偏暖的红
  ///
  /// 做成 static const，是给拿不到 BuildContext 的地方（比如右键菜单）用的。
  static const Color likeColor = Color(0xFFE5484D);

  Color get colorLike => likeColor;

  // ── 派生 ──────────────────────────────────────────────────────────
  /// 输入框获得焦点时的边框色
  Color get colorFocus => _scheme.primary;

  /// 下拉框按钮背景
  Color get colorComboBox => colorHover;

  /// 下拉菜单里鼠标悬浮/按下时的背景
  Color get colorComboMenuSelected => _scheme.primaryContainer;

  /// 下拉框文字
  TextStyle get comboTextStyle => midTextStyle;

  // ── 文本样式 ──────────────────────────────────────────────────────
  /// 大号粗体（页面标题）
  TextStyle get bigTextStyle => TextStyle(
    fontSize: 23,
    fontWeight: FontWeight.bold,
    color: colorTextMain,
  );

  /// 中号（导航按钮、歌曲名）
  TextStyle get midTextStyle => TextStyle(fontSize: 17, color: colorTextMain);

  /// 小号（次级信息、艺术家名）
  TextStyle get littleTextStyle =>
      TextStyle(fontSize: 14, color: colorTextSecond);
}
