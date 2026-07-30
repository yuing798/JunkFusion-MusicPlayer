/// ════════════════════════════════════════════════════════════════
/// left_column.dart — 左侧导航栏
///
/// 对应原 Vue 项目 components/LeftColumn.vue
/// 对应 C++ 中的 LeftColumn 类
///
/// 结构：
///   220px 宽的侧边栏，包含 Logo + 3 个分组按钮
///   选中按钮有八角形裁切 + 渐变背景 + 外发光效果
///
/// Dart 语法说明：
/// - `StatelessWidget`：不需要内部可变状态（selectedId 由父组件管理）
/// - `final void Function(int) onSelectionChanged;`：
///   Dart 的 callback 类型声明，类似 TypeScript 的 `(id: number) => void`
/// - `Scrollbar`：Flutter 内置的滚动条组件
/// - `ListView`：可滚动的线性列表
/// - `ClipPath`：自定义裁剪路径（用于八角形按钮形状）
/// - `CustomClipper<Path>`：自定义裁剪器，定义裁剪形状
/// ════════════════════════════════════════════════════════════════

import 'package:flutter/material.dart';
import '../theme/app_theme.dart';

/// LeftColumn — 左侧导航栏组件
///
/// 参数说明：
/// - `onSelectionChanged`：回调函数，当用户选择导航项时通知父组件
/// - `initialSelectedId`：初始选中的按钮 ID（默认 0 = 所有音乐）
class LeftColumn extends StatefulWidget {
  /// 选中事件回调（对应 Vue emit('selection-changed', id)）
  ///
  /// Dart 的函数类型语法：
  /// - `void Function(int)` 是一个函数类型，接收 int 参数，无返回值
  /// - `final` 表示这个字段创建后不可修改
  final void Function(int) onSelectionChanged;
  final int initialSelectedId;

  const LeftColumn({
    super.key,
    required this.onSelectionChanged,
    this.initialSelectedId = 0,
  });

  @override
  State<LeftColumn> createState() => _LeftColumnState();
}

class _LeftColumnState extends State<LeftColumn> {
  /// 当前选中的按钮 ID
  ///
  /// 对应原 Vue: const selectedId = ref(0)
  late int _selectedId;

  // ════════════════════════════════════════════════════════════════
  // 侧边栏分组数据（对应原 Vue sections 数组）
  //
  // Dart 的 `static const` 表示类级别编译期常量
  // `_NavSection` 是私有类（以 `_` 开头）
  // ════════════════════════════════════════════════════════════════

  static const List<_NavSection> _sections = [
    _NavSection(
      label: '曲库浏览',
      buttons: [
        _NavButton(id: 0, text: '所有音乐'),
        _NavButton(id: 1, text: '我喜欢'),
        _NavButton(id: 2, text: '最近播放'),
      ],
    ),
    _NavSection(
      label: '分类浏览',
      buttons: [
        _NavButton(id: 3, text: '作者'),
        _NavButton(id: 4, text: '专辑'),
        _NavButton(id: 5, text: '歌单'),
        _NavButton(id: 6, text: '风格'),
      ],
    ),
    _NavSection(
      label: '功能板块',
      buttons: [
        _NavButton(id: 7, text: 'AI助手'),
        _NavButton(id: 8, text: '效果器'),
        _NavButton(id: 9, text: '均衡器'),
        _NavButton(id: 10, text: '音箱阵列'),
        _NavButton(id: 11, text: '设置'),
      ],
    ),
  ];

  @override
  void initState() {
    super.initState();
    _selectedId = widget.initialSelectedId;
  }

  /// 处理按钮点击（收音机行为：不允许取消选中）
  ///
  /// 对应原 Vue handleSelect(id)
  void _handleSelect(int id) {
    if (_selectedId != id) {
      setState(() {
        _selectedId = id;
      });
      // 通知父组件
      widget.onSelectionChanged(id);
    }
  }

  @override
  Widget build(BuildContext context) {
    // Container — 类似 HTML 的 <div>，可设置宽高、颜色、边距等
    return Container(
      width: AppTheme.leftColumnWidth, // 220px
      color: AppTheme.colorNav,

      // Scrollbar — 滚动条（thumb 颜色使用强调色）
      child: Scrollbar(
        thumbVisibility: true, // 始终显示滚动条滑块
        child: ListView(
          // `padding` 是列表整体内边距
          // `EdgeInsets.only(bottom: 100)` 仅底部留 100px（对应原 CSS padding-bottom: 100px）
          padding: const EdgeInsets.only(bottom: 100),

          children: [
            // ── Logo 区域 ──
            _buildLogoArea(),

            // ── 三个分组 ──
            for (final section in _sections) _buildSection(section),
          ],
        ),
      ),
    );
  }

  /// 构建 Logo 区域
  ///
  /// 对应原 Vue: <div class="logo-area">
  ///              <img :src="logoImage" class="logo-image" />
  Widget _buildLogoArea() {
    return Container(
      height: 60,
      padding: const EdgeInsets.all(5),
      // `Center` widget：水平和垂直居中
      child: Image.asset(
        // TODO: 添加 Logo 资源文件到 pubspec.yaml 的 assets 中
        // 原 Vue 代码: import logoImage from '@/assets/image/junk-fusion.png'
        'assets/images/junk-fusion.png',

        // `fit: BoxFit.contain` 保持宽高比缩放，完整放入容器
        // 对应 CSS object-fit: contain
        fit: BoxFit.contain,

        // `errorBuilder`：图片加载失败时的 fallback
        errorBuilder: (context, error, stackTrace) {
          return const Text('Junk Fusion',
              style: TextStyle(fontWeight: FontWeight.bold));
        },
      ),
    );
  }

  /// 构建一个分组（标签 + 按钮列表）
  ///
  /// 对应原 Vue:
  ///   <div v-for="section in sections" class="navSection">
  ///     <div class="group-label">{{ section.label }}</div>
  ///     <button v-for="button in section.buttons" ...>
  Widget _buildSection(_NavSection section) {
    return Column(
      // `crossAxisAlignment: CrossAxisAlignment.stretch` 让子元素水平撑满
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        // --- 分组标签 ---
        _buildGroupLabel(section.label),

        // --- 分组按钮 ---
        for (final button in section.buttons)
          _buildNavButton(button),
      ],
    );
  }

  /// 构建分组标签
  ///
  /// 对应原 Vue: <div class="group-label">{{ section.label }}</div>
  /// 样式：50px 高，上下 5px 边框，居中粗体文字
  Widget _buildGroupLabel(String label) {
    return Container(
      height: 50,
      alignment: Alignment.center,
      // `BoxDecoration` 定义容器的装饰（背景、边框、圆角等）
      decoration: const BoxDecoration(
        // 上下边框（对应 CSS border-top + border-bottom）
        border: Border(
          top: BorderSide(color: AppTheme.colorEdge, width: 5),
          bottom: BorderSide(color: AppTheme.colorEdge, width: 5),
        ),
      ),
      child: Text(
        label,
        style: AppTheme.bigTextStyle,
      ),
    );
  }

  /// 构建单个导航按钮
  ///
  /// 对应原 Vue 的 .nav-button
  Widget _buildNavButton(_NavButton button) {
    final isActive = _selectedId == button.id;

    // `GestureDetector` — 检测手势（点击、滑动等）的 widget
    // 对应 Vue 的 @click 事件绑定
    return GestureDetector(
      onTap: () => _handleSelect(button.id),
      child: Container(
        width: double.infinity, // 撑满父容器宽度
        height: 40,
        margin: const EdgeInsets.all(4),
        alignment: Alignment.center,

        // `AnimatedContainer` — 带动画的容器
        // 在属性改变时会自动过渡（duration + curve）
        // 对应 Vue CSS transition
        decoration: isActive ? _activeDecoration() : null,
        child: Text(
          button.text,
          style: AppTheme.midTextStyle.copyWith(
            fontWeight: isActive ? FontWeight.bold : FontWeight.normal,
          ),
        ),
      ),
    );
  }

  /// 构建激活态按钮的装饰（八角形 + 渐变 + 发光）
  ///
  /// 对应原 Vue .nav-button.active 及其 ::before / ::after 伪元素
  BoxDecoration _activeDecoration() {
    return BoxDecoration(
      // 渐变背景（对应 CSS background-image 多层渐变）
      gradient: LinearGradient(
        begin: Alignment.topLeft,
        end: Alignment.bottomRight,
        colors: [
          AppTheme.colorStress,
          AppTheme.colorSuperStress,
        ],
      ),
      // 外发光（对应 ::after 的 filter: blur(8px)）
      // Flutter 的 BoxShadow 可以直接模拟外发光
      boxShadow: [
        BoxShadow(
          color: AppTheme.colorStress.withAlpha(153), // 60% 透明度
          blurRadius: 8,
        ),
      ],
    );

    // 注意：八角形裁剪路径在实际运行时用 ClipPath 实现
    // 此处简化为圆角矩形，完整八角形实现见 _OctagonClipper
  }
}

/// _NavSection — 导航分组的数据结构
///
/// Dart 语法说明：
/// - `class` 定义类（私有：`_` 开头）
/// - `final` 字段不可变
/// - `const` 构造函数允许编译期创建实例
/// - 这种小型的纯数据类在 Dart 中也叫"DTO"（Data Transfer Object）
class _NavSection {
  final String label;
  final List<_NavButton> buttons;

  const _NavSection({required this.label, required this.buttons});
}

/// _NavButton — 单个导航按钮数据
class _NavButton {
  final int id;
  final String text;

  const _NavButton({required this.id, required this.text});
}

/// _OctagonClipper — 八角形裁剪路径（对应 CSS clip-path: polygon(...)）
///
/// 用于激活态导航按钮的八角宝石形状
///
/// Dart 语法说明：
/// - `extends CustomClipper<Path>` 自定义裁剪器
/// - `Path` 是 Flutter 的路径描述对象
/// - `shouldReclip` 告诉框架是否需要重新裁剪（通常比较新旧即可）
class _OctagonClipper extends CustomClipper<Path> {
  /// 八角形在 4 个角的偏移量（对应 CSS --x-clip 和 --y-clip）
  final double clipSize;

  const _OctagonClipper({this.clipSize = 10.0});

  @override
  Path getClip(Size size) {
    final w = size.width;
    final h = size.height;
    final c = clipSize;

    // Path — 描述一个几何路径
    // moveTo: 移动起始点
    // lineTo: 画直线到指定点
    return Path()
      ..moveTo(c, 0) // 左上角起始（x偏移c, y=0）
      ..lineTo(w - c, 0) // 右上角
      ..lineTo(w, c) // 右边上切角
      ..lineTo(w, h - c) // 右边下切角
      ..lineTo(w - c, h) // 右下角
      ..lineTo(c, h) // 左下角
      ..lineTo(0, h - c) // 左边下切角
      ..lineTo(0, c) // 左边上切角
      ..closePath(); // 闭合路径回到起始点

    // `..` 是 Dart 的级联运算符（cascade operator）：
    // 在一个对象上连续调用多个方法/设置多个属性，最后返回该对象
    // 等价于：
    //   final path = Path();
    //   path.moveTo(c, 0);
    //   path.lineTo(w - c, 0);
    //   ...
    //   return path;
  }

  @override
  bool shouldReclip(covariant _OctagonClipper oldClipper) {
    return oldClipper.clipSize != clipSize;
  }
}

/// ════════════════════════════════════════════════════════════════
/// Dart 语法——Widget 构建方法命名约定：
///
/// - `_buildXxx()` 返回 Widget，是常见的私有方法命名模式
/// - 每个方法返回一个 Widget 子树
/// - 这等价于 Vue 的"抽离 template 片段到函数"
/// ════════════════════════════════════════════════════════════════
