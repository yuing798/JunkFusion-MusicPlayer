import 'package:flutter/material.dart';
import 'package:flutter_root_context_menu/flutter_root_context_menu.dart';

class CustomComboBox<T> extends StatefulWidget {
  final List<T> items;
  final T? value;
  final String Function(T) itemLabel;
  final ValueChanged<T?>? onChanged;
  final String? hint;
  final double? width;
  final double menuMaxHeight;

  const CustomComboBox({
    super.key,
    required this.items,
    this.value,
    required this.itemLabel,
    this.onChanged,
    this.hint,
    this.width,
    this.menuMaxHeight = 280,
  });

  @override
  State<CustomComboBox<T>> createState() => _CustomComboBoxState<T>();
}

class _CustomComboBoxState<T> extends State<CustomComboBox<T>> {
  final GlobalKey _buttonKey = GlobalKey();

  void _showMenu() {
    final RenderBox? box =
        _buttonKey.currentContext?.findRenderObject() as RenderBox?;
    if (box == null) return;

    final Offset position = box.localToGlobal(Offset.zero);
    final Size size = box.size;

    // 菜单出现在按钮正下方，左对齐
    final Offset menuPosition = Offset(
      position.dx,
      position.dy + size.height + 4, // 加一点间距
    );

    showRootContextMenu(
      context: context,
      position: menuPosition,
      useBarrier: true, // 建议开启，点击外部关闭更自然
      config: ContextMenuConfig(
        minWidth: widget.width ?? size.width,
        maxWidth: widget.width ?? size.width + 40,
        itemHeight: 44,
        borderRadius: BorderRadius.circular(10),
        elevation: 8,
        animationDuration: const Duration(milliseconds: 180), // 可自己调
        animationBuilder:
            ContextMenuAnimations.popup, // 可换成 fade / slideDown / scale 等
        backgroundColor: Theme.of(context).colorScheme.surface,
        hoverColor: Theme.of(context).colorScheme.primary,
      ),
      items: widget.items.map((item) {
        final bool isSelected = item == widget.value;
        return ContextMenuItem(
          label: widget.itemLabel(item),
          // 可以加图标表示选中
          icon: isSelected
              ? Icon(
                  Icons.check,
                  size: 18,
                  color: Theme.of(context).colorScheme.primary,
                )
              : const SizedBox(width: 18),
          onTap: () {
            widget.onChanged?.call(item);
            // 菜单会自动关闭
          },
        );
      }).toList(),
    );
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);

    return SizedBox(
      width: widget.width,
      child: Material(
        color: theme.colorScheme.surfaceContainerHighest,
        borderRadius: BorderRadius.circular(10),
        child: InkWell(
          key: _buttonKey,
          borderRadius: BorderRadius.circular(10),
          onTap: _showMenu,
          child: Padding(
            padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 12),
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
                      fontSize: 15,
                    ),
                    overflow: TextOverflow.ellipsis,
                  ),
                ),
                const SizedBox(width: 8),
                Icon(Icons.keyboard_arrow_down_rounded, color: theme.hintColor),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
