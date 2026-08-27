// =============================================================================
// ComboBox<T> — 通用下拉选择框
//
// 用法示例：
// ```dart
// final sortModes = ['添加时间', '歌曲名称', '播放次数'];
//
// ComboBox<String>(
//   value: '添加时间',
//   items: sortModes,
//   onChanged: (value) => print(value),
//   itemBuilder: (value) => Text(value),
//   selectedBuilder: (value) => Text(value),
// )
// ```
//
// 关键设计：
// - 点击整个 ComboBox（而非仅箭头）触发下拉
// - 下拉菜单通过 Overlay 渲染在 ComboBox 正下方
// - 开合时右侧图标旋转 180° 动画
// - 获得焦点时边框为 AppTheme.colorFocus，失焦时透明
// - 背景色、菜单单元格色、悬浮色、字体均来自 AppTheme
// =============================================================================

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:provider/provider.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

/// 通用 ComboBox 下拉组件
///
/// [T] 是选项的类型（可以是 String、int、enum、或任意自定义对象）
///
/// StatefulWidget：
///   因为需要维护"是否打开"这个内部可变状态（_isOpen），
///   所以必须是 StatefulWidget
class ComboBox<T> extends StatefulWidget {
  /// 当前选中的值（由父组件传入，控制受控模式）
  final T? value;

  /// 所有可选项的列表
  final List<T> items;

  /// 选中变化的回调
  final void Function(T value)? onChanged;

  /// 构建下拉菜单中每个选项的外观
  final Widget Function(T value) itemBuilder;

  /// 构建 ComboBox 闭合时显示的外观（可选，默认用 itemBuilder）
  final Widget Function(T value)? selectedBuilder;

  /// ComboBox 在闭合状态下的占位文字（当 value 为 null 时显示）
  final String placeholder;

  const ComboBox({
    super.key,
    this.value,
    required this.items,
    this.onChanged,
    required this.itemBuilder,
    this.selectedBuilder,
    this.placeholder = '',
  });

  @override
  State<ComboBox<T>> createState() => _ComboBoxState<T>();
}

class _ComboBoxState<T> extends State<ComboBox<T>>
        /// SingleTickerProviderStateMixin：
        /// 提供动画帧同步信号。State 混入这个 mixin 之后，
        /// AnimationController 的 vsync 参数才能填 `this`。
        with
        SingleTickerProviderStateMixin {
  // ──────────────────────────────────────────────────────────────
  // 状态
  // ──────────────────────────────────────────────────────────────

  /// 下拉菜单是否打开
  bool _isOpen = false;

  /// OverlayEntry 引用：用于插入/移除下拉菜单
  OverlayEntry? _overlayEntry;

  /// 动画控制器：控制右侧箭头的 180° 旋转
  late AnimationController _rotationController;

  /// ComboBox 的 GlobalKey：用于获取在屏幕上的位置，
  /// 从而把下拉菜单精确地放在 ComboBox 正下方
  final GlobalKey _comboBoxKey = GlobalKey();

  // ──────────────────────────────────────────────────────────────
  // 生命周期
  // ──────────────────────────────────────────────────────────────

  @override
  void initState() {
    super.initState();

    // 初始化旋转动画控制器
    // duration: 动画持续时间（与 AppTheme.easeTime 保持一致）
    // vsync: this — 来自 SingleTickerProviderStateMixin
    _rotationController = AnimationController(
      duration: Duration(milliseconds: 300),
      vsync: this,
    );
  }

  @override
  void dispose() {
    // 动画控制器必须手动释放（否则内存泄漏）
    _rotationController.dispose();
    // 如果下拉菜单还开着，先关掉再销毁
    _removeOverlay();
    super.dispose();
  }

  // ──────────────────────────────────────────────────────────────
  // Overlay 管理
  // ──────────────────────────────────────────────────────────────

  /// 打开下拉菜单
  ///
  /// 1. 获取 ComboBox 在屏幕上的位置和尺寸
  /// 2. 在 ComboBox 正下方创建一个 OverlayEntry
  /// 3. 播放箭头旋转动画（0 → 180°）
  void _openDropdown() {
    // 通过 GlobalKey 获取 RenderBox
    // RenderBox 提供了 widget 在屏幕上的精确位置和大小
    final renderBox =
        _comboBoxKey.currentContext?.findRenderObject() as RenderBox?;
    if (renderBox == null) return;

    // 获取 ComboBox 在全局坐标中的位置
    final offset = renderBox.localToGlobal(Offset.zero);
    final size = renderBox.size;

    // 创建 OverlayEntry（浮层条目）
    _overlayEntry = OverlayEntry(
      builder: (context) => _DropdownMenu<T>(
        // 菜单在屏幕上的位置：ComboBox 正下方
        left: offset.dx, // 与 ComboBox 左对齐
        top: offset.dy + size.height, // ComboBox 底部
        width: size.width, // 与 ComboBox 同宽
        items: widget.items,
        value: widget.value,
        itemBuilder: widget.itemBuilder,
        onSelect: _handleSelect,
        onDismiss: _closeDropdown,
      ),
    );

    // 把条目插入到 Overlay 最顶层
    Overlay.of(context).insert(_overlayEntry!);

    // 箭头旋转 180°（指向正上方）
    _rotationController.forward();

    setState(() => _isOpen = true);
  }

  /// 关闭下拉菜单
  void _closeDropdown() {
    _removeOverlay();

    // 箭头旋转回原位（0°）
    _rotationController.reverse();

    setState(() => _isOpen = false);
  }

  /// 移除 Overlay 条目
  void _removeOverlay() {
    _overlayEntry?.remove();
    _overlayEntry = null;
  }

  // ──────────────────────────────────────────────────────────────
  // 事件处理
  // ──────────────────────────────────────────────────────────────

  /// 用户选中某个选项
  void _handleSelect(T value) {
    widget.onChanged?.call(value);
    _closeDropdown();
  }

  /// 点击 ComboBox：打开或关闭下拉菜单
  void _toggle() {
    if (_isOpen) {
      _closeDropdown();
    } else {
      _openDropdown();
    }
  }

  // ──────────────────────────────────────────────────────────────
  // build
  // ──────────────────────────────────────────────────────────────

  @override
  Widget build(BuildContext context) {
    // 根据是否打开来决定边框颜色
    // isOpen = "获得焦点" → colorFocus
    // !isOpen = "失焦" → transparent
    final theme = context.watch<AppTheme>();
    final borderColor = _isOpen ? theme.colorFocus : Colors.transparent;

    return GestureDetector(
      // key 绑定在 ComboBox 本体的外层，用于定位
      key: _comboBoxKey,
      // 点击整个 ComboBox → 切换下拉
      onTap: _toggle,
      child: Container(
        height: 36,
        padding: const EdgeInsets.symmetric(horizontal: 12),
        // 装饰：背景色 + 边框（失焦透明，获焦强调色）
        decoration: BoxDecoration(
          color: theme.colorComboBox,
          borderRadius: BorderRadius.circular(5),
          border: Border.all(color: borderColor, width: 2),
        ),
        // Row：左边显示当前选中文本，右边显示箭头图标
        child: Row(
          children: [
            // ── 左侧：当前选中值 / 占位文本 ──
            Expanded(
              child: widget.value != null
                  ? (widget.selectedBuilder?.call(widget.value as T) ??
                        widget.itemBuilder(widget.value as T))
                  : Text(widget.placeholder, style: theme.midTextStyle),
            ),

            // ── 右侧：箭头图标（带旋转动画） ──
            // AnimatedBuilder 监听动画值变化，每次 tick 都重建图标
            AnimatedBuilder(
              animation: _rotationController,
              builder: (context, child) {
                // 动画值从 0 → 1（打开） 或 1 → 0（关闭）
                // 映射为旋转角度：0° → 180°
                return Transform.rotate(
                  angle: _rotationController.value * 3.14159, // π = 180°
                  child: Icon(
                    TablerIcons.caretDownFilled,
                    size: 20,
                    color: theme.colorTextMain,
                  ),
                );
              },
            ),
          ],
        ),
      ),
    );
  }
}

// =============================================================================
// _DropdownMenu<T> — 下拉菜单浮层
//
// 通过 OverlayEntry 渲染，因此不受父级布局约束，
// 可以精确定位到 ComboBox 正下方。
// =============================================================================

class _DropdownMenu<T> extends StatelessWidget {
  /// 菜单左边缘的屏幕 x 坐标
  final double left;

  /// 菜单顶部的屏幕 y 坐标
  final double top;

  /// 菜单宽度（与 ComboBox 同宽）
  final double width;

  /// 所有可选项
  final List<T> items;

  /// 当前选中值
  final T? value;

  /// 构建每个选项的 Widget
  final Widget Function(T value) itemBuilder;

  /// 选中回调
  final void Function(T value) onSelect;

  /// 点击外部关闭回调
  final VoidCallback onDismiss;

  const _DropdownMenu({
    required this.left,
    required this.top,
    required this.width,
    required this.items,
    required this.value,
    required this.itemBuilder,
    required this.onSelect,
    required this.onDismiss,
  });

  @override
  Widget build(BuildContext context) {
    // 获取屏幕尺寸（用于限制菜单高度）
    final screenHeight = MediaQuery.of(context).size.height;
    // 菜单最大高度 = 屏幕剩余空间（从 ComboBox 底部到屏幕底部）- 安全边距
    final maxMenuHeight = (screenHeight - top).clamp(0.0, 300.0);

    /// Stack：层叠布局
    /// 第 0 层：全屏透明 GestureDetector（点击即关闭菜单）
    /// 第 1 层：菜单本体（定位在 ComboBox 正下方）
    return Stack(
      children: [
        // ── 遮罩层：一个覆盖全屏的透明区域 ──
        // 用户点击 ComboBox 之外的任何地方 → 关闭菜单
        GestureDetector(
          onTap: onDismiss,
          // behavior: HitTestBehavior.translucent 使得透明区域也能
          // 接收点击事件（默认情况下透明区域不响应手势）
          behavior: HitTestBehavior.translucent,
          child: Container(color: Colors.transparent),
        ),

        // ── 菜单本体 ──
        Positioned(
          left: left,
          top: top,
          width: width,
          child: Material(
            // Material：Overlay 中的 Widget 必须包裹在 Material 中
            color: Colors.transparent,
            child: Container(
              // 限制最大高度，超出则滚动
              constraints: BoxConstraints(maxHeight: maxMenuHeight),
              decoration: BoxDecoration(
                borderRadius: BorderRadius.circular(6),
                boxShadow: const [
                  BoxShadow(
                    color: Color(0x29000000), // 黑色 16% 透明度
                    offset: Offset(0, 4),
                    blurRadius: 12,
                  ),
                ],
              ),
              // 使用 SingleChildScrollView 防止选项过多溢出
              child: SingleChildScrollView(
                child: Column(
                  // mainAxisSize: MainAxisSize.min
                  // → 高度 = 所有子元素高度之和（不会撑满父级）
                  mainAxisSize: MainAxisSize.min,
                  children: items.map((item) {
                    final isSelected = item == value;
                    return _DropdownItem<T>(
                      value: item,
                      isSelected: isSelected,
                      itemBuilder: itemBuilder,
                      onTap: () => onSelect(item),
                    );
                  }).toList(),
                ),
              ),
            ),
          ),
        ),
      ],
    );
  }
}

// =============================================================================
// _DropdownItem<T> — 下拉菜单中的单个选项
// =============================================================================

class _DropdownItem<T> extends StatefulWidget {
  final T value;
  final bool isSelected;
  final Widget Function(T value) itemBuilder;
  final VoidCallback onTap;

  const _DropdownItem({
    required this.value,
    required this.isSelected,
    required this.itemBuilder,
    required this.onTap,
  });

  @override
  State<_DropdownItem<T>> createState() => _DropdownItemState<T>();
}

class _DropdownItemState<T> extends State<_DropdownItem<T>> {
  /// 鼠标是否悬浮在此选项上
  bool _isHovered = false;

  @override
  Widget build(BuildContext context) {
    // 背景色优先级：
    // 1. 选中态 → 不做特殊处理（用户可在 itemBuilder 中自行处理）
    // 2. 悬浮/按下 → colorComboMenuSelected
    // 3. 默认 → 透明
    final backgroundColor = _isHovered
        ? context.watch<AppTheme>().colorComboMenuSelected
        : Colors.transparent;

    // MouseRegion：桌面端专用的鼠标检测组件
    // onEnter → 鼠标进入时设置 _isHovered = true
    // onExit  → 鼠标离开时设置 _isHovered = false
    return MouseRegion(
      onEnter: (_) => setState(() => _isHovered = true),
      onExit: (_) => setState(() => _isHovered = false),
      child: GestureDetector(
        onTap: widget.onTap,
        child: Container(
          width: double.infinity,
          padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
          color: backgroundColor,
          child: widget.itemBuilder(widget.value),
        ),
      ),
    );
  }
}

/// =============================================================================
/// AnimatedBuilder：
///
/// 这是我自己写的类名（本意是 AnimatedBuilder），但 Flutter 的标准 Widget 叫作
/// **AnimatedBuilder**（不过推荐用更简洁的 **TweenAnimationBuilder**）。
///
/// 在 ComboBox 中的箭头旋转实现如下：
/// ```dart
/// AnimatedBuilder(
///   animation: _rotationController,
///   builder: (context, child) {
///     return Transform.rotate(
///       angle: _rotationController.value * pi,  // 0 → π (0° → 180°)
///       child: Icon(TablerIcons.caretDownFilled),
///     );
///   },
/// )
/// ```
///
/// _rotationController.value 的取值范围是 0.0 ~ 1.0：
/// - 关闭时：forward() → 0 → 1（对应角度 0° → 180°）
/// - 打开时：reverse() → 1 → 0（对应角度 180° → 0°）
///
/// Transform.rotate 的 angle 参数单位是**弧度**（不是度数），
/// 180° = π ≈ 3.14159 弧度。
/// =============================================================================

/// =============================================================================
/// Overlay 工作原理简述：
///
/// Flutter 的 Overlay 系统类似 CSS 的 z-index 机制。
/// Navigator 和 MaterialApp 内部都有一个 Overlay 层。
///
/// OverlayEntry 是 Overlay 中的一个"浮层条目"：
/// - insert() → 显示到屏幕最上层
/// - remove() → 从屏幕移除
///
/// 下拉菜单必须用 Overlay 而非嵌套 widget，因为：
/// 嵌套 widget 会受到父级 ClipRect/Overflow 的限制，菜单会被裁切。
/// Overlay 渲染在独立的层中，不受任何父级约束。
///
/// 对应 Vue 的 Teleport to body。
/// =============================================================================

/// =============================================================================
/// MouseRegion — 桌面端鼠标悬浮检测：
///
/// Flutter 是跨平台框架，桌面端需要检测鼠标悬浮事件。
/// MouseRegion 提供 onEnter / onExit 回调，用来实现
/// CSS 的 :hover 效果。
///
/// 在移动端（Android/iOS），MouseRegion 不会触发任何事件，
/// 因为它只响应鼠标事件，不响应触屏事件。
/// =============================================================================
