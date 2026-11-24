/// 效果器链的「上下箭头 + 卡片」编辑区（**全局链和预设对话框共用**）
///
/// ── 长什么样 ──────────────────────────────────────────────────────────
/// ```
/// ┌───────────────────────────────┐
/// │ 输入 · 交叉淡化之后            │
/// └───────────────────────────────┘
///              ↓
/// ┌───────────────────────────────┐
/// │ ⠿ ① ChowMatrix        未应用   │
/// │        ⏻   🎛   ✕             │
/// └───────────────────────────────┘
///              ↓
/// ┌───────────────────────────────┐
/// │ 输出 · → 均衡器                │
/// └───────────────────────────────┘
/// ```
/// **为什么是上下排**：横向排的时候一屏只放得下 3~4 张卡，第 5 张开始就被裁掉；
/// 而且横向的拖动和横向滚动会抢手势，实际很难拖。上下排每个条目都是通栏，
/// 左侧一整条 32px 的拖动手柄区，拖起来没有歧义。
///
/// ── 拖动 ──────────────────────────────────────────────────────────────
///   用 animated_reorderable_list，但**关掉默认拖动**，只在左侧手柄上挂
///   `ReorderableDragStartListener` —— 桌面端长按拖动很别扭，而且默认拖动
///   会和滚动打架。
///
/// ── 为什么单独抽出来 ──────────────────────────────────────────────────
///   页面主区和"预设编辑对话框"是同一个东西：卡片、箭头、拖动、增删。
///   抽成一个纯展示 + 回调的控件，两处只差 `status`（对话框里传 null，
///   因为预设没有实例在跑，不该显示"已加载/延迟"）。
///
/// ── 用法 ─────────────────────────────────────────────────────────────
/// ```dart
/// EffectChainEditor(                  // 放进 Expanded 里
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
// 这个包的文档写的是 Flutter 自带的 ReorderableDragStartListener，但实现里用的是
// 它自己的 ReorderableGridDragStartListener，而且**没有从主库导出**。
// 挂 Flutter 那个的话祖先查不到 ReorderableAnimatedBuilder，拖拽压根不会启动，
// 所以这里只能引它的 src（升级包时如果路径变了，编译期就会报错，不会静默失效）。
// ignore: implementation_imports
import 'package:animated_reorderable_list/src/component/drag_listener.dart';
import 'package:flutter/gestures.dart' show DragStartBehavior;
import 'package:flutter/material.dart';
import 'package:flutter_smooth_wheel_scroll/flutter_smooth_wheel_scroll.dart';
import 'package:junk_fusion_ui/model/plugin_chain.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:provider/provider.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

class EffectChainEditor extends StatefulWidget {
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

  @override
  State<EffectChainEditor> createState() => _EffectChainEditorState();
}

class _EffectChainEditorState extends State<EffectChainEditor> {
  /// 滚轮平滑滚动（项目规范：新写的滚动器都用 SmoothScrollController）
  final ScrollController _scrollController = SmoothScrollController();

  /// 一张卡片的高度（拖动手柄整条可拖，所以这里要够高）
  static const double _cardHeight = 66;

  /// 上下两端假卡的高度
  static const double _terminalHeight = 42;

  /// 箭头那一行的高度
  static const double _arrowHeight = 18;

  @override
  void dispose() {
    _scrollController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final showRuntime = widget.status != null;
    final hasSlots = widget.slots.isNotEmpty;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        _buildTerminal(theme, '输入', '交叉淡化之后'),
        _buildArrow(theme),

        if (hasSlots) ...[
          // Flexible + shrinkWrap：链短的时候下面的「添加」和「输出」
          // 紧跟最后一张卡，链长的时候列表自己在剩余高度里滚
          Flexible(child: _buildList(theme, showRuntime)),
          _buildArrow(theme),
        ],

        // 没满上限就一直在链尾给一个「添加效果器」按钮
        if (widget.slots.length < widget.maxSlots) _buildAddCard(theme),

        _buildArrow(theme),
        _buildTerminal(theme, '输出', '→ 均衡器'),
      ],
    );
  }

  Widget _buildList(AppTheme theme, bool showRuntime) {
    return AnimatedReorderableListView<ChainSlot>(
      scrollDirection: Axis.vertical,
      controller: _scrollController,
      physics: const ClampingScrollPhysics(),
      shrinkWrap: true,
      // 默认拖动（长按/自绘手柄）会和滚动抢手势，改成只在左侧手柄上拖
      buildDefaultDragHandles: false,
      padding: EdgeInsets.zero,
      dragStartBehavior: DragStartBehavior.start,
      items: widget.slots,
      // 配置对象每次改都会换新实例，身份靠前端分配的 uid
      isSameItem: (a, b) => a.uid == b.uid,
      onReorder: widget.onReorder ?? (_, __) {},
      // 删除动画会把这棵子树重新 activate 一次，如果子树里有 OverlayPortal
      // （比如 Tooltip）就会在 layout 中往 Overlay 插节点，触发 Flutter 的
      // "_RenderLayoutBuilder was mutated in performLayout"。
      // 删插件本来也不需要动画，直接归零最省事。
      removeDuration: Duration.zero,
      itemBuilder: (context, index) {
        final slot = widget.slots[index];
        return Column(
          key: ValueKey<int>(slot.uid),
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            _buildCard(theme, index, slot, showRuntime),
            _buildArrow(theme),
          ],
        );
      },
    );
  }

  // ── 两端的假卡 ────────────────────────────────────────────────────
  Widget _buildTerminal(AppTheme theme, String title, String subtitle) {
    return Container(
      height: _terminalHeight,
      padding: const EdgeInsets.symmetric(horizontal: 12),
      decoration: BoxDecoration(
        color: theme.colorCell.withValues(alpha: 0.4),
        border: Border.all(color: theme.colorEdge),
        borderRadius: BorderRadius.circular(6),
      ),
      child: Row(
        children: [
          Text(title, style: theme.midTextStyle),
          const SizedBox(width: 8),
          Text(subtitle, style: theme.littleTextStyle),
        ],
      ),
    );
  }

  Widget _buildArrow(AppTheme theme) {
    return SizedBox(
      height: _arrowHeight,
      child: Center(
        child: Icon(
          TablerIcons.arrowDown,
          size: 15,
          color: theme.colorTextSecond.withValues(alpha: 0.7),
        ),
      ),
    );
  }

  // ── 链尾的「添加效果器」 ──────────────────────────────────────────
  Widget _buildAddCard(AppTheme theme) {
    return InkWell(
      onTap: widget.onAdd,
      borderRadius: BorderRadius.circular(6),
      child: Container(
        height: 46,
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(6),
          border: Border.all(color: theme.colorEdge),
        ),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(Icons.add, size: 20, color: theme.colorTextSecond),
            const SizedBox(width: 8),
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
    final slotStatus = widget.status?.slots[position];
    final error = slotStatus?.error ?? '';
    final loaded = slotStatus?.loaded ?? false;

    // 状态行：错误 > 未应用 / 已加载(+延迟)
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
      height: _cardHeight,
      decoration: BoxDecoration(
        color: theme.colorCell,
        borderRadius: BorderRadius.circular(6),
        border: Border.all(
          color: slot.bypass ? theme.colorEdge : theme.colorStress,
          width: slot.bypass ? 1 : 1.5,
        ),
      ),
      clipBehavior: Clip.antiAlias,
      child: Row(
        children: [
          // ── 左侧整条拖动手柄（通栏高度，拖起来没有歧义）──
          //
          // 必须用**这个包自己的** ReorderableGridDragStartListener：
          // Flutter 自带的那个查的是 SliverReorderableList 的祖先，这里是
          // ReorderableAnimatedBuilder，挂上去不会启动拖拽（踩过）。
          ReorderableGridDragStartListener(
            index: position,
            child: MouseRegion(
              cursor: SystemMouseCursors.grab,
              child: Container(
                width: 32,
                height: double.infinity,
                color: theme.colorHover,
                child: Icon(
                  TablerIcons.gripVertical,
                  size: 16,
                  color: theme.colorTextSecond,
                ),
              ),
            ),
          ),
          const SizedBox(width: 10),

          // 序号
          Container(
            width: 20,
            height: 20,
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
          const SizedBox(width: 10),

          // 名字 + 状态
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Text(
                  slot.name.isEmpty ? slot.identifier : slot.name,
                  style: theme.midTextStyle.copyWith(
                    color: slot.bypass
                        ? theme.colorTextSecond
                        : theme.colorTextMain,
                  ),
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
                const SizedBox(height: 2),
                Text(
                  statusText,
                  style: theme.littleTextStyle.copyWith(
                    fontSize: 11,
                    color: statusColor,
                  ),
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
              ],
            ),
          ),

          // 三个操作
          _iconButton(
            theme,
            tooltip: slot.bypass ? '取消旁通（立即生效）' : '旁通这个插件（立即生效）',
            icon: slot.bypass ? Icons.toggle_off : Icons.toggle_on,
            onPressed: widget.onBypass == null
                ? null
                : () => widget.onBypass!(position),
          ),
          _iconButton(
            theme,
            tooltip: loaded ? '打开插件编辑器' : '应用到全局链之后才有编辑器',
            icon: Icons.tune,
            onPressed: (loaded && widget.onOpenEditor != null)
                ? () => widget.onOpenEditor!(position)
                : null,
          ),
          _iconButton(
            theme,
            tooltip: '从链里移除',
            icon: Icons.close,
            onPressed: widget.onRemove == null
                ? null
                : () => widget.onRemove!(position),
          ),
          const SizedBox(width: 6),
        ],
      ),
    );
  }

  /// 卡片上的小图标按钮
  ///
  /// **刻意不用 Tooltip**：Tooltip 内部是 OverlayPortal，而这个列表在删除条目时
  /// 会把即将消失的子树重新 activate 一次，OverlayPortal 在 layout 期间往
  /// Overlay 插节点会触发 Flutter 的断言
  /// （`_RenderLayoutBuilder was mutated in performLayout`，实测删插件必现）。
  /// 读屏信息用 Semantics 补，鼠标指针自己管。
  Widget _iconButton(
    AppTheme theme, {
    required String tooltip,
    required IconData icon,
    VoidCallback? onPressed,
  }) {
    return Semantics(
      label: tooltip,
      button: true,
      enabled: onPressed != null,
      child: MouseRegion(
        cursor: onPressed == null
            ? SystemMouseCursors.basic
            : SystemMouseCursors.click,
        child: InkWell(
          onTap: onPressed,
          borderRadius: BorderRadius.circular(4),
          child: Padding(
            padding: const EdgeInsets.all(5),
            child: Icon(
              icon,
              size: 18,
              color: onPressed == null
                  ? theme.colorEdge
                  : theme.colorTextSecond,
            ),
          ),
        ),
      ),
    );
  }
}
