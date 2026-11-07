/// CustomComboBox — 项目自己实现的下拉选择器
///
/// ── 为什么不用 Flutter 原生的 DropdownButton ──────────────────────────
/// 原生 DropdownButton 的展开动画是"从按钮位置拉伸"，菜单出现的位置也不可控
/// （总是贴着按钮下方，靠近屏幕底部时要靠它自己的 overflow 逻辑硬挪），
/// 视觉上很别扭。这个组件用 `showRootContextMenu` 在**根 Overlay** 上弹菜单，
/// 位置由我们精确计算：贴着按钮上下沿、可以指定向上或向下、可以调整动画。
///
/// ── 用法 ─────────────────────────────────────────────────────────────
///     CustomComboBox<BandType>(
///       items: BandType.values,
///       value: band.type,
///       itemLabel: (type) => type.displayName,
///       onChanged: (type) { ... },
///       width: 96,
///       menuDirection: ComboBoxMenuDirection.auto,   // 下方放不下就往上弹
///       screenPadding: EdgeInsets.only(bottom: 100), // 预留给底部悬浮栏
///     )
///
/// ── 位置规则 ─────────────────────────────────────────────────────────
/// 1. 默认贴着按钮：向下 = 按钮下沿 + menuGap；向上 = 按钮上沿 − 菜单高度 − menuGap；
/// 2. [menuDirection] 可以强制向下 / 向上，`auto` 时按上下可用空间自动翻转；
/// 3. 传了 [menuPositionOverride] 就用那个全局坐标当菜单左上角（可从任意位置弹菜单）；
/// 4. 菜单高度按"条目数 × 单项高 + 内边距"精确估算，所以向上弹时底边正好贴住按钮；
/// 5. 越界时由 flutter_root_context_menu 自己再做一次屏幕内钳制。
library;

import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:flutter_root_context_menu/flutter_root_context_menu.dart';

/// 菜单展开方向
enum ComboBoxMenuDirection {
  /// 下方放不下且上方更宽裕时自动向上弹
  auto,

  /// 永远向下弹
  down,

  /// 永远向上弹
  up,
}

class CustomComboBox<T> extends StatefulWidget {
  const CustomComboBox({
    super.key,
    required this.items,
    this.value,
    required this.itemLabel,
    this.onChanged,
    this.hint,
    this.width,
    this.menuMaxHeight = 280,
    this.menuDirection = ComboBoxMenuDirection.auto,
    this.menuItemHeight = 44,
    this.menuGap = 4,
    this.screenPadding = const EdgeInsets.all(8),
    this.menuPositionOverride,
    this.contentPadding = const EdgeInsets.symmetric(horizontal: 14, vertical: 12),
    this.fontSize = 15,
    this.enabled = true,
  });

  final List<T> items;
  final T? value;
  final String Function(T) itemLabel;
  final ValueChanged<T?>? onChanged;
  final String? hint;
  final double? width;

  /// 只用于"向上还是向下"的可用空间判断（这个包的菜单本身不支持滚动）
  final double menuMaxHeight;
  final ComboBoxMenuDirection menuDirection;
  final double menuItemHeight;

  /// 菜单和按钮之间的间距
  final double menuGap;

  /// 距离屏幕边缘的安全距离；想让开底部悬浮栏就把 bottom 设成栏高
  final EdgeInsets screenPadding;

  /// 手动指定菜单弹出的全局坐标（左上角）；null = 跟着按钮走
  final Offset? menuPositionOverride;

  /// 按钮内部的 padding（想做得紧凑就调小）
  final EdgeInsets contentPadding;
  final double fontSize;
  final bool enabled;

  @override
  State<CustomComboBox<T>> createState() => _CustomComboBoxState<T>();
}

class _CustomComboBoxState<T> extends State<CustomComboBox<T>> {
  final GlobalKey _buttonKey = GlobalKey();

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final isEnabled = widget.enabled;

    return SizedBox(
      width: widget.width,
      child: Material(
        color: theme.colorScheme.surfaceContainerHighest.withValues(
          alpha: isEnabled ? 1.0 : 0.5,
        ),
        borderRadius: BorderRadius.circular(8),
        child: InkWell(
          key: _buttonKey,
          borderRadius: BorderRadius.circular(8),
          onTap: isEnabled ? _showMenu : null,
          child: Padding(
            padding: widget.contentPadding,
            child: Row(
              children: [
                Expanded(
                  child: Text(
                    widget.value != null
                        ? widget.itemLabel(widget.value as T)
                        : (widget.hint ?? '请选择'),
                    style: TextStyle(
                      color: widget.value != null
                          ? theme.colorScheme.onSurface
                          : theme.hintColor,
                      fontSize: widget.fontSize,
                    ),
                    overflow: TextOverflow.ellipsis,
                  ),
                ),
                const SizedBox(width: 6),
                Icon(
                  Icons.keyboard_arrow_down_rounded,
                  size: widget.fontSize + 5,
                  color: theme.hintColor,
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }

  // ── 弹菜单 ────────────────────────────────────────────────────────
  void _showMenu() {
    final box = _buttonKey.currentContext?.findRenderObject() as RenderBox?;
    if (box == null) return;

    final buttonTopLeft = box.localToGlobal(Offset.zero);
    final buttonSize = box.size;

    // 先建 config：菜单高度要用它的 itemHeight / itemMargin / menuPadding 精确算
    final config = ContextMenuConfig(
      minWidth: widget.width ?? buttonSize.width,
      maxWidth: widget.width != null ? widget.width! + 40 : buttonSize.width + 40,
      itemHeight: widget.menuItemHeight,
      borderRadius: BorderRadius.circular(10),
      elevation: 8,
      animationDuration: const Duration(milliseconds: 120),
      animationBuilder: ContextMenuAnimations.fade,
      backgroundColor: Theme.of(context).colorScheme.surface,
      hoverColor: Theme.of(context).colorScheme.primaryContainer,
      screenPadding: widget.screenPadding,
    );

    final menuHeight = _estimateMenuHeight(config);

    final position = widget.menuPositionOverride ??
        _resolveAnchor(buttonTopLeft, buttonSize, menuHeight);

    showRootContextMenu(
      context: context,
      position: position,
      useBarrier: true,
      config: config,
      items: [
        for (final item in widget.items) _buildMenuItem(context, item),
      ],
    );
  }

  /// 菜单高度 = 条目高（含 itemMargin）× 条目数 + 容器 menuPadding
  double _estimateMenuHeight(ContextMenuConfig config) {
    final perItem = config.itemHeight + config.itemMargin.vertical;
    return widget.items.length * perItem + config.menuPadding.vertical;
  }

  /// 算出菜单左上角的全局坐标
  Offset _resolveAnchor(Offset buttonTopLeft, Size buttonSize, double menuHeight) {
    final mediaQuery = MediaQuery.of(context);
    final topLimit = mediaQuery.padding.top + widget.screenPadding.top;
    final bottomLimit = mediaQuery.size.height -
        mediaQuery.padding.bottom -
        widget.screenPadding.bottom;

    final spaceBelow =
        bottomLimit - (buttonTopLeft.dy + buttonSize.height) - widget.menuGap;
    final spaceAbove = buttonTopLeft.dy - topLimit - widget.menuGap;

    final openUp = switch (widget.menuDirection) {
      ComboBoxMenuDirection.up => true,
      ComboBoxMenuDirection.down => false,
      // 下方放不下、且上方更宽裕时才翻上去；两边都放不下就选宽的那边
      ComboBoxMenuDirection.auto =>
        spaceBelow < menuHeight && spaceAbove > spaceBelow,
    };

    final double top;
    if (openUp) {
      // 向上弹：菜单底边贴住按钮上沿
      top = buttonTopLeft.dy - menuHeight - widget.menuGap;
    } else {
      top = buttonTopLeft.dy + buttonSize.height + widget.menuGap;
    }

    return Offset(buttonTopLeft.dx, math.max(top, topLimit));
  }

  ContextMenuItem _buildMenuItem(BuildContext context, T item) {
    final isSelected = item == widget.value;
    final primary = Theme.of(context).colorScheme.primary;

    return ContextMenuItem(
      label: widget.itemLabel(item),
      icon: isSelected
          ? Icon(Icons.check, size: 18, color: primary)
          : const SizedBox(width: 18),
      onTap: () => widget.onChanged?.call(item),
    );
  }
}
