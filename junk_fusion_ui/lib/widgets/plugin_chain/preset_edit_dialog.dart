/// 预设编辑对话框：命名 + 调整这条预设的链
///
/// ── 什么时候弹 ────────────────────────────────────────────────────────
///   · 侧边栏右上角 **+**      → 新建预设（名字空、链为空，从当前全局链拷一条也行）
///   · 侧边栏 **双击**某个预设 → 编辑那条预设（名字和链都回填）
///
/// ── 它不做什么 ────────────────────────────────────────────────────────
///   不直接写数据库、不直接下发。它只把「名字 + 一条链 + 要不要顺便应用到全局」
///   返回给调用者（效果器页），由页面调 Provider 去存 / 去应用。
///   这样对话框可以复用，也不必知道后端长什么样。
///
/// ── 用法 ─────────────────────────────────────────────────────────────
/// ```dart
/// final result = await showDialog<PresetEditResult>(
///   context: context,
///   builder: (_) => PresetEditDialog(
///     presetId: preset?.id,             // null = 新建
///     initialTitle: preset?.title ?? '',
///     initialSlots: preset?.slots ?? const [],
///   ),
/// );
/// ```
library;

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/plugin_chain.dart';
import 'package:junk_fusion_ui/model/plugin_info.dart';
import 'package:junk_fusion_ui/providers/plugin_chain_provider.dart';
import 'package:junk_fusion_ui/providers/plugin_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';
import 'package:junk_fusion_ui/widgets/plugin_chain/effect_chain_editor.dart';
import 'package:junk_fusion_ui/widgets/plugin_chain/plugin_picker_dialog.dart';
import 'package:provider/provider.dart';

/// 对话框的返回值
class PresetEditResult {
  const PresetEditResult({
    required this.title,
    required this.slots,
    required this.applyToGlobal,
  });

  final String title;
  final List<ChainSlot> slots;

  /// true = 保存后顺便贴到全局链并下发（"保存并应用"按钮）
  final bool applyToGlobal;
}

class PresetEditDialog extends StatefulWidget {
  const PresetEditDialog({
    super.key,
    this.presetId,
    this.initialTitle = '',
    this.initialSlots = const <ChainSlot>[],
  });

  /// null = 新建预设
  final int? presetId;
  final String initialTitle;
  final List<ChainSlot> initialSlots;

  @override
  State<PresetEditDialog> createState() => _PresetEditDialogState();
}

class _PresetEditDialogState extends State<PresetEditDialog> {
  late final TextEditingController _titleController;
  late List<ChainSlot> _slots;

  bool get _isEditing => widget.presetId != null;

  @override
  void initState() {
    super.initState();
    _titleController = TextEditingController(text: widget.initialTitle);
    _slots = [...widget.initialSlots];
  }

  @override
  void dispose() {
    _titleController.dispose();
    super.dispose();
  }

  // ── 链编辑 ────────────────────────────────────────────────────────
  Future<void> _addSlot() async {
    final plugins = context.read<PluginProvider>().plugins;
    final selected = await showPluginPickerDialog(context, plugins);
    if (selected == null || !mounted) return;
    _appendPlugin(selected);
  }

  void _appendPlugin(PluginItem plugin) {
    if (_slots.length >= PluginChainProvider.maxSlots) {
      UtilFunction.showWarnMessage(
        '一条链最多 ${PluginChainProvider.maxSlots} 个效果器',
      );
      return;
    }
    setState(() {
      _slots = [
        ..._slots,
        ChainSlot.create(
          index: _slots.length,
          filePath: plugin.filePath,
          identifier: plugin.identifier,
          name: plugin.name,
        ),
      ];
    });
  }

  void _removeSlot(int position) {
    if (position < 0 || position >= _slots.length) return;
    setState(() => _slots = [..._slots]..removeAt(position));
  }

  void _moveSlot(int oldIndex, int newIndex) {
    var target = newIndex;
    if (target > oldIndex) target -= 1;
    if (target < 0 || target >= _slots.length || target == oldIndex) return;

    setState(() {
      final next = [..._slots];
      final moved = next.removeAt(oldIndex);
      next.insert(target, moved);
      _slots = next;
    });
  }

  void _toggleBypass(int position) {
    if (position < 0 || position >= _slots.length) return;
    setState(() {
      final next = [..._slots];
      next[position] = next[position].copyWith(bypass: !next[position].bypass);
      _slots = next;
    });
  }

  // ── 保存 ──────────────────────────────────────────────────────────
  void _submit({required bool applyToGlobal}) {
    final title = _titleController.text.trim();
    if (title.isEmpty) {
      UtilFunction.showWarnMessage('给预设起个名字吧');
      return;
    }
    if (_slots.isEmpty) {
      UtilFunction.showWarnMessage('这条链还是空的，先加一个效果器');
      return;
    }

    Navigator.of(context).pop(
      PresetEditResult(
        title: title,
        slots: _slots,
        applyToGlobal: applyToGlobal,
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    // 820 是舒展宽度；窗口窄的时候按 80% 收，别让对话框撑出屏幕
    final dialogWidth = (MediaQuery.sizeOf(context).width * 0.8).clamp(
      520.0,
      820.0,
    );
    // 上下排布的链编辑区要给它一个有界高度：链长了在里面滚
    final dialogHeight = (MediaQuery.sizeOf(context).height * 0.72).clamp(
      420.0,
      620.0,
    );

    return AlertDialog(
      title: Text(_isEditing ? '编辑预设' : '新建预设'),
      content: SizedBox(
        width: dialogWidth,
        height: dialogHeight,
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                SizedBox(
                  width: 72,
                  child: Text('预设名称', style: theme.littleTextStyle),
                ),
                Expanded(
                  child: TextField(
                    controller: _titleController,
                    autofocus: true,
                    decoration: const InputDecoration(
                      isDense: true,
                      hintText: '例如：母带 · 温暖',
                    ),
                    onSubmitted: (_) => _submit(applyToGlobal: false),
                  ),
                ),
                const SizedBox(width: 12),
                TextButton.icon(
                  onPressed: _addSlot,
                  icon: const Icon(Icons.add, size: 18),
                  label: const Text('添加效果器'),
                ),
              ],
            ),
            const SizedBox(height: 10),
            Text(
              '拖动卡片左侧的 ⠿ 调整顺序；这条链可以贴到全局，也可以（以后）贴到某个分轨',
              style: theme.littleTextStyle.copyWith(fontSize: 12),
            ),
            const SizedBox(height: 8),
            Expanded(
              child: EffectChainEditor(
                slots: _slots,
                status: null, // 预设没有实例在跑，不显示"已加载/延迟"
                onReorder: _moveSlot,
                onRemove: _removeSlot,
                onBypass: _toggleBypass,
                onAdd: _addSlot,
              ),
            ),
          ],
        ),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.of(context).pop(),
          child: const Text('取消'),
        ),
        TextButton(
          onPressed: () => _submit(applyToGlobal: false),
          child: const Text('保存'),
        ),
        FilledButton(
          onPressed: () => _submit(applyToGlobal: true),
          child: const Text('保存并应用到全局链'),
        ),
      ],
    );
  }
}
