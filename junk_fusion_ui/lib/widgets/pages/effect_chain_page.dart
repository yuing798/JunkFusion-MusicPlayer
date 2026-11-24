/// 效果器页（左侧导航 id = 8）：全局效果器链 + 链预设
///
/// ── 两个板块怎么分 ────────────────────────────────────────────────────
///   · **右（主内容区）**：全局效果器链。它真的插在交叉淡化之后、EQ 之前出声，
///     所以有"应用 / 整链旁通 / 打开插件编辑器 / 已加载延迟"这些运行时概念。
///   · **左（sidebar）**：链预设。预设 = 一条完整的链（0~8 个插件），
///     可以贴到全局链上（现在），以后也可以贴到某个分轨上。
///     和 EQ 页一个分工：预设列表住 sidebar，工作区住主区。
///
/// ── 交互 ──────────────────────────────────────────────────────────────
///   · 主区：+ 添加 / 拖动 ⠿ 排序（只改前端，点应用才重建）/ 单卡旁通（立即生效）
///           / 🎛 打开插件编辑器 / ✕ 移除 / 整链旁通（立即生效）
///   · 侧边栏：单击预设 = 载入并立即下发；双击 = 打开对话框改名 + 改链；
///            + = 新建预设（默认拷一份当前全局链，省得从零搭）；
///            右键 = 编辑 / 应用到全局链 / 删除
///
/// ── 为什么"改"了还要点应用 ────────────────────────────────────────────
///   换插件/换顺序要重新实例化（实测 CHOWTapeModel 1.86 秒）。重建期间旧链照常出声，
///   所以这个按钮不是"省 CPU"，而是"让用户决定什么时候切声音"。
///
/// 用法（只有 pages_route.dart 引用）：
///     const EffectChainPage()
library;

import 'package:flutter/material.dart';
import 'package:flutter_root_context_menu/flutter_root_context_menu.dart';
import 'package:junk_fusion_ui/model/plugin_chain.dart';
import 'package:junk_fusion_ui/model/plugin_info.dart';
import 'package:junk_fusion_ui/providers/plugin_chain_provider.dart';
import 'package:junk_fusion_ui/providers/plugin_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';
import 'package:junk_fusion_ui/widgets/plugin_chain/effect_chain_editor.dart';
import 'package:junk_fusion_ui/widgets/plugin_chain/plugin_picker_dialog.dart';
import 'package:junk_fusion_ui/widgets/plugin_chain/preset_edit_dialog.dart';
import 'package:provider/provider.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

class EffectChainPage extends StatefulWidget {
  const EffectChainPage({super.key});

  @override
  State<EffectChainPage> createState() => _EffectChainPageState();
}

class _EffectChainPageState extends State<EffectChainPage> {
  /// sidebar 宽度（和 EQ 页的预设列一个量级）
  static const double _sidebarWidth = 236;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted) return;
      context.read<PluginChainProvider>().ensureInitialized();
      context.read<PluginProvider>().ensureInitialized();
    });
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final chain = context.watch<PluginChainProvider>();
    final plugins = context.watch<PluginProvider>().plugins;

    return Row(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        SizedBox(width: _sidebarWidth, child: _buildSidebar(theme, chain)),
        const VerticalDivider(width: 1),
        Expanded(child: _buildMainArea(theme, chain, plugins)),
      ],
    );
  }

  // ════════════════════════════════════════════════════════════════
  // sidebar：链预设
  // ════════════════════════════════════════════════════════════════
  Widget _buildSidebar(AppTheme theme, PluginChainProvider chain) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        SizedBox(
          height: 52,
          child: Padding(
            padding: const EdgeInsets.only(left: 12, right: 4),
            child: Row(
              children: [
                Text('链预设', style: theme.midTextStyle),
                const Spacer(),
                IconButton(
                  tooltip: '新建预设',
                  icon: const Icon(Icons.add, size: 20),
                  onPressed: () => _openPresetDialog(chain, preset: null),
                ),
              ],
            ),
          ),
        ),
        Container(height: 1, color: theme.colorEdge),
        Expanded(
          child: chain.presets.isEmpty
              ? Center(
                  child: Padding(
                    padding: const EdgeInsets.all(16),
                    child: Text(
                      '还没有预设\n\n调好全局链之后，\n点右上角的 + 存一条',
                      textAlign: TextAlign.center,
                      style: theme.littleTextStyle,
                    ),
                  ),
                )
              : ListView.builder(
                  padding: const EdgeInsets.symmetric(vertical: 6),
                  itemCount: chain.presets.length,
                  itemBuilder: (context, index) {
                    return _buildPresetRow(theme, chain, chain.presets[index]);
                  },
                ),
        ),
        Container(height: 1, color: theme.colorEdge),
        Padding(
          padding: const EdgeInsets.fromLTRB(12, 8, 12, 10),
          child: Text(
            '当前全局链：${chain.length} 个 · 延迟 ${chain.totalLatency} 采样',
            style: theme.littleTextStyle.copyWith(fontSize: 12),
          ),
        ),
      ],
    );
  }

  Widget _buildPresetRow(
    AppTheme theme,
    PluginChainProvider chain,
    ChainPresetItem preset,
  ) {
    return InkWell(
      // 单击 = 载入并立即下发
      onTap: () => _applyPreset(chain, preset),
      // 双击 = 改名 + 改链
      onDoubleTap: () => _openPresetDialog(chain, preset: preset),
      onSecondaryTapDown: (details) =>
          _showPresetMenu(context, chain, preset, details.globalPosition),
      child: Padding(
        padding: const EdgeInsets.fromLTRB(12, 8, 12, 8),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              preset.title,
              style: theme.midTextStyle,
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
            ),
            const SizedBox(height: 2),
            Text(
              preset.preview,
              style: theme.littleTextStyle.copyWith(fontSize: 11),
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
            ),
          ],
        ),
      ),
    );
  }

  void _showPresetMenu(
    BuildContext context,
    PluginChainProvider chain,
    ChainPresetItem preset,
    Offset position,
  ) {
    showRootContextMenu(
      context: context,
      position: position,
      useBarrier: true,
      items: [
        ContextMenuItem(
          label: '编辑（改名 / 改链）',
          onTap: () => _openPresetDialog(chain, preset: preset),
        ),
        ContextMenuItem(
          label: '应用到全局链',
          onTap: () => _applyPreset(chain, preset),
        ),
        ContextMenuItem(
          label: '删除',
          onTap: () {
            chain.deletePreset(preset.id);
            UtilFunction.showInfoMessage('已删除预设「${preset.title}」');
          },
        ),
      ],
    );
  }

  // ════════════════════════════════════════════════════════════════
  // 主区：全局效果器链
  // ════════════════════════════════════════════════════════════════
  Widget _buildMainArea(
    AppTheme theme,
    PluginChainProvider chain,
    List<PluginItem> plugins,
  ) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        SizedBox(
          height: 56,
          child: Padding(
            padding: const EdgeInsets.symmetric(horizontal: 12),
            child: Row(
              children: [
                Text('全局效果器链', style: theme.bigTextStyle),
                const SizedBox(width: 10),
                Text('交叉淡化之后 · 全局共享', style: theme.littleTextStyle),
                const Spacer(),
                Text('整链旁通', style: theme.littleTextStyle),
                Switch(
                  value: chain.bypassAll,
                  onChanged: (_) => chain.toggleBypassAll(),
                ),
                const SizedBox(width: 8),
                TextButton.icon(
                  onPressed: chain.isFull
                      ? () => UtilFunction.showWarnMessage(
                          '一条链最多 ${PluginChainProvider.maxSlots} 个效果器',
                        )
                      : () => _addPlugin(chain, plugins),
                  icon: const Icon(Icons.add, size: 18),
                  label: const Text('添加'),
                ),
                const SizedBox(width: 8),
                FilledButton(
                  onPressed: () => _applyChain(chain),
                  style: chain.isDirty
                      ? FilledButton.styleFrom(
                          backgroundColor: theme.colorStress,
                          foregroundColor: theme.colorOnStress,
                        )
                      : null,
                  child: Text(chain.isDirty ? '应用 *' : '应用'),
                ),
              ],
            ),
          ),
        ),
        Expanded(
          child: Padding(
            padding: const EdgeInsets.fromLTRB(12, 8, 12, 8),
            child: Align(
              alignment: Alignment.topLeft,
              child: EffectChainEditor(
                slots: chain.slots,
                status: chain.status,
                onReorder: chain.moveSlot,
                onRemove: chain.removeSlot,
                onBypass: chain.toggleSlotBypass,
                onOpenEditor: chain.openEditor,
                onAdd: () => _addPlugin(chain, plugins),
              ),
            ),
          ),
        ),
        Padding(
          padding: const EdgeInsets.fromLTRB(12, 0, 12, 10),
          child: Text(
            '拖动卡片左侧的 ⠿ 调整顺序；改完点「应用」才重建（重建期间还在放旧链）。'
            '旁通是立即生效的。',
            style: theme.littleTextStyle.copyWith(fontSize: 12),
          ),
        ),
      ],
    );
  }

  // ════════════════════════════════════════════════════════════════
  // 交互
  // ════════════════════════════════════════════════════════════════
  Future<void> _addPlugin(
    PluginChainProvider chain,
    List<PluginItem> plugins,
  ) async {
    final selected = await showPluginPickerDialog(context, plugins);
    if (selected == null || !mounted) return;

    if (!chain.addSlot(selected)) {
      final error = chain.lastError;
      chain.clearError();
      UtilFunction.showWarnMessage(error ?? '添加失败');
    }
  }

  void _applyChain(PluginChainProvider chain) {
    if (!chain.apply()) {
      final error = chain.lastError;
      chain.clearError();
      UtilFunction.showErrorMessage(error ?? '下发效果器链失败');
      return;
    }

    // apply() 可能留下"被黑名单挡住"的提示
    final warning = chain.lastError;
    chain.clearError();
    if (warning != null) {
      UtilFunction.showWarnMessage(warning);
    } else {
      UtilFunction.showInfoMessage('已下发，正在加载插件…');
    }
  }

  void _applyPreset(PluginChainProvider chain, ChainPresetItem preset) {
    if (!chain.applyPreset(preset.id)) {
      final error = chain.lastError;
      chain.clearError();
      UtilFunction.showErrorMessage(error ?? '载入预设失败');
      return;
    }
    UtilFunction.showInfoMessage('已载入预设「${preset.title}」');
  }

  /// 打开预设对话框：preset == null 表示新建（默认拷一份当前全局链）
  Future<void> _openPresetDialog(
    PluginChainProvider chain, {
    required ChainPresetItem? preset,
  }) async {
    var initialSlots = chain.slots;
    if (preset != null) {
      final loaded = chain.readPresetChain(preset.id);
      if (loaded == null) {
        UtilFunction.showErrorMessage('预设读不出来，可能数据损坏');
        return;
      }
      initialSlots = loaded.slots;
    } else {
      // 新建：拷当前链，省得从零搭
      initialSlots = _cloneChain(chain.slots);
    }

    final result = await showDialog<PresetEditResult>(
      context: context,
      builder: (_) => PresetEditDialog(
        presetId: preset?.id,
        initialTitle: preset?.title ?? '',
        initialSlots: initialSlots,
      ),
    );
    if (result == null || !mounted) return;

    if (preset == null) {
      final id = chain.saveChainAsPreset(result.title, result.slots);
      if (id <= 0) {
        UtilFunction.showErrorMessage('保存预设失败');
        return;
      }
      UtilFunction.showInfoMessage('已保存预设「${result.title}」');
    } else {
      final ok = chain.updatePresetChain(preset.id, result.title, result.slots);
      if (!ok) {
        UtilFunction.showErrorMessage('更新预设失败');
        return;
      }
      UtilFunction.showInfoMessage('已更新预设「${result.title}」');
    }

    if (!result.applyToGlobal) return;

    chain.loadChain(result.slots);
    _applyChain(chain);
  }

  /// 拷一条链（生成新的 uid，避免和全局链的条目撞身份）
  List<ChainSlot> _cloneChain(List<ChainSlot> source) {
    return [
      for (var index = 0; index < source.length; index++)
        ChainSlot.create(
          index: index,
          filePath: source[index].filePath,
          identifier: source[index].identifier,
          name: source[index].name,
          bypass: source[index].bypass,
          state: source[index].state,
        ),
    ];
  }
}
