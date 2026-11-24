/// 效果器链的「箭头 + 卡片」编辑区（**全局链和预设对话框共用**）
///
/// ── 长什么样 ──────────────────────────────────────────────────────────
/// ```
/// [输入] → [+ 卡片 ⠿] → [卡片 ⠿] → [输出 EQ]
/// ```
/// 两端是假卡（输入 / 输出），中间是真正可拖动的插件卡。
/// 拖动排序**只改前端的顺序**（标脏），点「应用」才重建 —— 重建要重新实例化
/// 插件（实测 CHOWTapeModel 1.86 秒），每拖一次就重建的话没法用。
///
/// ── 为什么单独抽出来 ──────────────────────────────────────────────────
/// 页面主区和"预设编辑对话框"是同一个东西：8 张卡、箭头、拖动、增删。
/// 抽成一个纯展示 + 回调的控件，两处只差 `showRuntime`（对话框里不显示
/// "已加载/延迟"，因为预设没有实例在跑）。
///
/// ── 用法 ─────────────────────────────────────────────────────────────
/// ```dart
/// EffectChainEditor(
///   slots: provider.slots,
///   status: provider.status,          // 预设对话框里传 null
///   onReorder: provider.moveSlot,
///   onRemove: provider.removeSlot,
///   onBypass: provider.toggleSlotBypass,
///   onOpenEditor: provider.openEditor,
///   onAdd: () => _pickPlugin(...),
/// )
/// ```
library;

import 'package:animated_reorderable_list/animated_reorderable_list.dart';
import 'package:flutter/gestures.dart' show DragStartBehavior;
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/plugin_chain.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:provider/provider.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

class EffectChainEditor extends StatelessWidget {
  const EffectChainEditor({
    super.key,
    required this.slots,
    this.status,
    this.onReorder,
    this.onRemove,
    this.onBypass,
    this.onOpenEditor,
    this.onAdd,
    this.maxSlots = 8,
  });

  /// 链（有序，0~8）
  final List<ChainSlot> slots;

  /// 运行状态；传 null = 不显示"已加载/延迟"（预设对话框里就是 null）
  final ChainStatus? status;

  final void Function(int oldIndex, int newIndex)? onReorder;
  final void Function(int position)? onRemove;
  final void Function(int position)? onBypass;
  final void Function(int position)? onOpenEditor;
  final VoidCallback? onAdd;
  final int maxSlots;

  /// 卡片尺寸：改这里就能整体放大/缩小
  static const double _cardWidth = 204;
  static const double _cardHeight = 124;

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final showRuntime = status != null;

    return SizedBox(
      height: _cardHeight + 16,
      child: Row(
        children: [
          _buildTerminal(theme, '输入', '交叉淡化之后'),
          _buildArrow(theme),
          Expanded(
            child: slots.isEmpty
                ? Align(
                    alignment: Alignment.centerLeft,
                    child: _buildEmptyCard(theme),
                  )
                : AnimatedReorderableListView<ChainSlot>(
                    scrollDirection: Axis.horizontal,
                    shrinkWrap: true,
                    // 自己画拖动手柄（桌面端长按拖动很别扭）
                    buildDefaultDragHandles: false,
                    padding: EdgeInsets.zero,
                    dragStartBehavior: DragStartBehavior.start,
                    items: slots,
                    // 配置对象每次改都会换新实例，所以身份靠前端分配的 uid
                    isSameItem: (a, b) => a.uid == b.uid,
                    onReorder: onReorder ?? (_, __) {},
                    itemBuilder: (context, index) {
                      final slot = slots[index];
                      return Row(
                        key: ValueKey<int>(slot.uid),
                        children: [
                          _buildCard(theme, index, slot, showRuntime),
                          _buildArrow(theme),
                        ],
                      );
                    },
                  ),
          ),
          _buildArrow(theme),
          _buildTerminal(theme, '输出', '→ 均衡器'),
        ],
      ),
    );
  }

  // ── 两端假卡 ──────────────────────────────────────────────────────
  Widget _buildTerminal(AppTheme theme, String title, String subtitle) {
    return Container(
      width: 96,
      height: _cardHeight,
      decoration: BoxDecoration(
        color: theme.colorCell.withValues(alpha: 0.4),
        border: Border.all(color: theme.colorEdge, style: BorderStyle.solid),
        borderRadius: BorderRadius.circular(6),
      ),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Text(title, style: theme.midTextStyle),
          const SizedBox(height: 4),
          Text(
            subtitle,
            textAlign: TextAlign.center,
            style: theme.littleTextStyle.copyWith(fontSize: 11),
          ),
        ],
      ),
    );
  }

  Widget _buildArrow(AppTheme theme) {
    return SizedBox(
      width: 26,
      height: _cardHeight,
      child: Icon(
        TablerIcons.arrowRight,
        size: 18,
        color: theme.colorTextSecond.withValues(alpha: 0.7),
      ),
    );
  }

  // ── 空链 ──────────────────────────────────────────────────────────
  Widget _buildEmptyCard(AppTheme theme) {
    return InkWell(
      onTap: onAdd,
      borderRadius: BorderRadius.circular(6),
      child: Container(
        width: _cardWidth,
        height: _cardHeight,
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(6),
          border: Border.all(color: theme.colorEdge),
        ),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(Icons.add, size: 26, color: theme.colorTextSecond),
            const SizedBox(height: 6),
            Text('添加效果器', style: theme.littleTextStyle),
          ],
        ),
      ),
    );
  }

  // ── 一张插件卡 ────────────────────────────────────────────────────
  Widget _buildCard(
    AppTheme theme,
    int position,
    ChainSlot slot,
    bool showRuntime,
  ) {
    final slotStatus = status?.slots[position];
    final error = slotStatus?.error ?? '';
    final loaded = slotStatus?.loaded ?? false;

    // 状态行：错误 > 已加载(+延迟) > 未应用
    final String statusText;
    final Color statusColor;
    if (error.isNotEmpty) {
      statusText = error;
      statusColor = theme.colorError;
    } else if (!showRuntime) {
      statusText = slot.bypass ? '已旁通' : '保存后生效';
      statusColor = theme.colorTextSecond;
    } else if (loaded) {
      statusText = slotStatus!.latency > 0
          ? '已加载 · 延迟 ${slotStatus.latency}'
          : '已加载';
      statusColor = theme.colorTextSecond;
    } else {
      statusText = '未应用';
      statusColor = theme.colorTextSecond;
    }

    return Container(
      width: _cardWidth,
      height: _cardHeight,
      margin: const EdgeInsets.only(right: 2),
      padding: const EdgeInsets.fromLTRB(8, 6, 8, 4),
      decoration: BoxDecoration(
        color: theme.colorCell,
        borderRadius: BorderRadius.circular(6),
        border: Border.all(
          color: slot.bypass ? theme.colorEdge : theme.colorStress,
          width: slot.bypass ? 1 : 1.5,
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              // 拖动手柄
              ReorderableDragStartListener(
                index: position,
                child: MouseRegion(
                  cursor: SystemMouseCursors.grab,
                  child: Icon(
                    TablerIcons.gripVertical,
                    size: 16,
                    color: theme.colorTextSecond,
                  ),
                ),
              ),
              const SizedBox(width: 4),
              Container(
                width: 18,
                height: 18,
                alignment: Alignment.center,
                decoration: BoxDecoration(
                  color: theme.colorHover,
                  borderRadius: BorderRadius.circular(4),
                ),
                child: Text(
                  '${position + 1}',
                  style: theme.littleTextStyle.copyWith(fontSize: 11),
                ),
              ),
              const SizedBox(width: 6),
              Expanded(
                child: Text(
                  slot.name.isEmpty ? slot.identifier : slot.name,
                  style: theme.midTextStyle.copyWith(
                    color: slot.bypass
                        ? theme.colorTextSecond
                        : theme.colorTextMain,
                  ),
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
              ),
            ],
          ),
          const SizedBox(height: 4),
          Text(
            statusText,
            style: theme.littleTextStyle.copyWith(
              fontSize: 11,
              color: statusColor,
            ),
            maxLines: 2,
            overflow: TextOverflow.ellipsis,
          ),
          const Spacer(),
          Row(
            children: [
              _iconButton(
                theme,
                tooltip: slot.bypass ? '取消旁通' : '旁通这个插件',
                icon: slot.bypass ? Icons.toggle_off : Icons.toggle_on,
                onPressed: onBypass == null ? null : () => onBypass!(position),
              ),
              _iconButton(
                theme,
                tooltip: loaded ? '打开插件编辑器' : '应用到全局链之后才有编辑器',
                icon: Icons.tune,
                onPressed: (loaded && onOpenEditor != null)
                    ? () => onOpenEditor!(position)
                    : null,
              ),
              const Spacer(),
              _iconButton(
                theme,
                tooltip: '从链里移除',
                icon: Icons.close,
                onPressed: onRemove == null ? null : () => onRemove!(position),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _iconButton(
    AppTheme theme, {
    required String tooltip,
    required IconData icon,
    VoidCallback? onPressed,
  }) {
    return Tooltip(
      message: tooltip,
      child: InkWell(
        onTap: onPressed,
        borderRadius: BorderRadius.circular(4),
        child: Padding(
          padding: const EdgeInsets.all(4),
          child: Icon(
            icon,
            size: 17,
            color: onPressed == null ? theme.colorEdge : theme.colorTextSecond,
          ),
        ),
      ),
    );
  }
}
