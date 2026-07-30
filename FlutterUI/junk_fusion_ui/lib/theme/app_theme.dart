/// ════════════════════════════════════════════════════════════════
/// app_theme.dart — 应用主题配置
///
/// 对应原 Vue 项目 assets/css/theme.css
/// 将 CSS 自定义属性（--color-* 等）映射为 Dart 常量
///
/// Dart 语法说明：
/// - `import 'package:flutter/material.dart'` 导入 Flutter 的 Material 组件库
/// - `class` 定义一个类（相当于 C++ 的 class/struct，TypeScript 的 class）
/// - `static const` 类级别常量，属于类本身而非实例，编译期确定
/// - `Color(0xFFxxxxxx)` Dart 的颜色构造：前 2 位 FF 是 alpha（不透明度），后 6 位是 RGB 十六进制
/// - `TextStyle` 是一个 immutable 对象（创建后不可变），描述文字样式
/// - `ThemeData` 是 Flutter Material 主题的配置对象
/// - `=>` 是箭头函数语法，等价于 `{ return xxx; }`，仅用于单表达式
/// ════════════════════════════════════════════════════════════════

// `import` 语句：引入其他 Dart 文件或 package 中的代码
// `package:flutter/material.dart` 是 Flutter 框架的核心 UI 库
import 'package:flutter/material.dart';

/// AppTheme 类 — 存放应用级别的主题常量和 ThemeData
///
/// `abstract class` 抽象类：不能被实例化（不能 new），只能被继承或用做静态成员的容器
/// 这里把它当作纯静态常量容器使用（类似 C++ namespace 或 TypeScript 的 const 对象）
abstract class AppTheme {
  // ════════════════════════════════════════════════════════════════
  // 颜色常量（对应 Vue theme.css 中的 CSS 自定义属性）
  //
  // Dart 的 `static const`：
  // - `static`: 属于类本身，不绑定到实例，通过 AppTheme.colorMain 访问
  // - `const`: 编译期常量，内存中只存一份，不可变（immutable）
  // ════════════════════════════════════════════════════════════════

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

  // ════════════════════════════════════════════════════════════════
  // 字体大小常量（对应 Vue :root 中的 CSS 变量）
  // ════════════════════════════════════════════════════════════════

  /// 大字号 — 25px（对应 --big-font）
  static const double bigFont = 25.0;

  /// 中等字号 — 18px（对应 --mid-font）
  static const double midFont = 18.0;

  /// 小字号 — 15px（对应 --little-font）
  static const double littleFont = 15.0;

  // ════════════════════════════════════════════════════════════════
  // 其他常量
  // ════════════════════════════════════════════════════════════════

  /// 圆角半径（对应 --border-radius: 6px）
  static const double borderRadius = 6.0;

  /// 过渡动画时长（对应 --ease-time: 0.15s），单位毫秒
  static const Duration easeTime = Duration(milliseconds: 150);

  /// SVG 按钮宽高（对应 --svg-btn-width: 30px）
  static const double svgBtnWidth = 30.0;

  /// 左侧导航栏宽度（对应 LeftColumn.vue 中的 220px）
  static const double leftColumnWidth = 220.0;

  /// 底部播放栏高度（对应 playBar.vue 中的 90px）
  static const double playBarHeight = 90.0;

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

  // ════════════════════════════════════════════════════════════════
  // ShinyBtn 样式辅助方法（对应 .shiny-btn CSS）
  //
  // 这里用 static 方法而非 const BoxDecoration，因为 BoxDecoration
  // 的 gradient 属性使用了 LinearGradient（动态对象，不能是 const）
  // ════════════════════════════════════════════════════════════════

  /// 构建 shiny-btn 的 BoxDecoration
  ///
  /// Dart 语法说明：
  /// - `static BoxDecoration shinyBtnDecoration()` 声明一个返回 BoxDecoration 的静态方法
  /// - `BoxDecoration` 是 Flutter 中的"盒子装饰"，包含背景、边框、圆角、阴影等
  /// - `LinearGradient` 线性渐变：从左上(145°) 到右下
  /// - `begin: Alignment.topLeft, end: Alignment.bottomRight` 定义渐变方向
  /// - `BoxShadow` 表示 CSS box-shadow 的等价物
  static BoxDecoration shinyBtnDecoration() {
    return BoxDecoration(
      // `gradient` 属性接受一个 Gradient 对象
      // 这里用 LinearGradient 实现 CSS `linear-gradient(145deg, ...)`
      gradient: const LinearGradient(
        begin: Alignment.topLeft, // 对应 CSS 145deg 的起始方向
        end: Alignment.bottomRight,
        colors: [
          colorStress,
          colorSuperStress,
        ],
      ),
      // `borderRadius` 圆角半径
      borderRadius: BorderRadius.circular(borderRadius),
      // `boxShadow` 阴影列表（对应 CSS box-shadow 多层阴影）
      // Dart 的 const 列表语法：[item1, item2, ...]
      boxShadow: const [
        // 底部硬阴影（模拟厚度，对应 0 6px 0 var(--color-shadow)）
        BoxShadow(
          color: colorShadow,
          offset: Offset(0, 6), // 向下偏移 6px，无水平偏移
          blurRadius: 0, // 不模糊（hard shadow）
        ),
        // 环境柔光阴影（对应 0 8px 20px rgb(0,0,0,25%)）
        BoxShadow(
          color: Color(0x40000000), // 黑色 25% 透明度
          offset: Offset(0, 8),
          blurRadius: 20,
        ),
      ],
    );
  }

  /// shiny-btn 按下时的 BoxDecoration（对应 .shiny-btn:active）
  ///
  /// 点击时：按钮下移 6px，底部硬阴影消失，环境阴影减弱
  static BoxDecoration shinyBtnActiveDecoration() {
    return BoxDecoration(
      gradient: const LinearGradient(
        begin: Alignment.topLeft,
        end: Alignment.bottomRight,
        colors: [
          colorStress,
          colorSuperStress,
        ],
      ),
      borderRadius: BorderRadius.circular(borderRadius),
      boxShadow: const [
        BoxShadow(
          color: Color(0x26000000), // 黑色 15% 透明度
          offset: Offset(0, 4),
          blurRadius: 12,
        ),
      ],
    );
  }

  // ════════════════════════════════════════════════════════════════
  // ThemeData — Flutter Material 主题配置
  //
  // ThemeData 是 Flutter 中定义全局主题的对象，它的作用等价于
  // Vue 的 :root CSS 变量 + theme.css 的结合体。
  //
  // Dart 语法说明：
  // - `static const` 表示这个变量在编译期就确定了，之后不会改变
  // - `ThemeData(...)` 构造函数调用，Flutter 中构造函数不需要 `new` 关键字
  // - 命名参数（如 `colorSchemeSeed: ...`）是 Dart 的特色：
  //   调用函数时显式写出参数名，顺序可随意，所有命名参数都是可选的
  // ════════════════════════════════════════════════════════════════

  /// 应用浅色主题
  ///
  /// `colorSchemeSeed` 是 Material 3 的种子色，Flutter 会自动由此推导
  /// 完整的 ColorScheme（primary, secondary, surface 等）
  static const ThemeData lightTheme = ThemeData(
    // `useMaterial3` 启用 Material Design 3（最新设计规范）
    useMaterial3: true,

    // `colorSchemeSeed` 设置种子颜色，Flutter 自动生成完整的调色板
    colorSchemeSeed: colorStress,

    // `brightness` 亮度：Brightness.light 表示浅色主题
    brightness: Brightness.light,

    // `scaffoldBackgroundColor` 是 Scaffold（页面脚手架）的默认背景色
    scaffoldBackgroundColor: colorMain,

    // `cardTheme` 卡片主题配置
    cardTheme: CardThemeData(
      color: colorCell,
      elevation: 0, // 无阴影（扁平化）
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(borderRadius),
      ),
    ),
  );
}

/// ════════════════════════════════════════════════════════════════
/// Dart 文件结构说明（帮助理解 Flutter 项目组织）：
///
/// 一个 .dart 文件通常包含：
/// 1. `library` 声明（可选）
/// 2. `import` 语句 — 引入其他库/文件
/// 3. 顶层变量/常量/函数
/// 4. 类定义（class）
///
/// 注意：
/// - Dart 中没有 `interface` 关键字，所有类都可以作为接口实现
/// - Dart 使用 `mixin` 实现代码复用（类似 C++ 多重继承，但更安全）
/// - 文件名的约定是小写+下划线（snake_case），如 app_theme.dart
/// - 类名、枚举名用大驼峰（PascalCase），如 AppTheme、Color
/// - 变量名、函数名用小驼峰（camelCase），如 colorMain、shinyBtnDecoration
/// ════════════════════════════════════════════════════════════════
