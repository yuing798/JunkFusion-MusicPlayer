/// 效果器段的中间栏：**链预设**列表
///
/// ── 两个板块怎么分（三段式里的分工）────────────────────────────────────
///   · 中间栏（本文件）：链预设。预设 = 一条完整的链（0~8 个插件），
///     可以贴到全局链（现在），以后也可以贴到某个分轨。
///   · 主内容区（widgets/pages/effect_chain_page.dart）：全局效果器链本体，
///     它有"应用 / 整链旁通 / 打开编辑器 / 已加载延迟"这些运行时概念。
///   和 EQ 页完全一个分工：预设住中间栏，工作区住内容区。
///
/// ── 交互 ──────────────────────────────────────────────────────────────
///   · 右上角 + ：新建预设（弹对话框，名字 + 链；默认拷一份当前全局链）
///   · 单击：载入并立即下发到全局链
///   · 双击：打开对话框改名 / 改链
///   · 右键：编辑 / 应用到全局链 / 删除
///
/// ── 数据 ──────────────────────────────────────────────────────────────
///   全走 PluginChainProvider → FFI → Coordinator 的 pluginChainPresets 表。
///   列表项自带 `preview`（后端拼的 "A → B → C"），不需要为每个预设再发 FFI。
library;

import 'package:flutter/material.dart';
import 'package:flutter_smooth_wheel_scroll/flutter_smooth_wheel_scroll.dart';
import 'package:junk_fusion_ui/model/plugin_chain.dart';
import 'package:junk_fusion_ui/providers/plugin_chain_provider.dart';
import 'package:junk_fusion_ui/providers/plugin_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';
import 'package:junk_fusion_ui/widgets/plugin_chain/preset_edit_dialog.dart';
import 'package:provider/provider.dart';
import 'package:flutter_root_context_menu/flutter_root_context_menu.dart';

class EffectChainPresetSidebar extends StatefulWidget {
  const EffectChainPresetSidebar({super.key});

  @override
  State<EffectChainPresetSidebar> createState() =>
      _EffectChainPresetSidebarState();
}

class _EffectChainPresetSidebarState extends State<EffectChainPresetSidebar> {
  /// 滚轮平滑滚动（项目规范：新写的滚动器都用 SmoothScrollController）
  final ScrollController _scrollController = SmoothScrollController();

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted) return;
      // 主内容区那边也会调，重复调用无副作用
      context.read<PluginChainProvider>().ensureInitialized();
      context.read<PluginProvider>().ensureInitialized();
    });
  }

  @override
  void dispose() {
    _scrollController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final chain = context.watch<PluginChainProvider>();
    final theme = context.watch<AppTheme>();

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        _buildHeader(chain, theme),
        Divider(height: 1, color: theme.colorEdge),
        Expanded(child: _buildList(chain, theme)),
        Divider(height: 1, color: theme.colorEdge),
        _buildFooter(chain, theme),
      ],
    );
  }

  Widget _buildHeader(PluginChainProvider chain, AppTheme theme) {
    return SizedBox(
      height: 36,
      child: Row(
        children: [
          const SizedBox(width: 10),
          Text('链预设', style: theme.midTextStyle),
          const Spacer(),
          IconButton(
            tooltip: '新建预设（默认拷一份当前全局链）',
            icon: const Icon(Icons.add, size: 20),
            onPressed: () => _openPresetDialog(chain, preset: null),
          ),
        ],
      ),
    );
  }

  Widget _buildList(PluginChainProvider chain, AppTheme theme) {
    if (chain.presets.isEmpty) {
      return Center(
        child: Padding(
          padding: const EdgeInsets.all(12),
          child: Text(
            '还没有预设\n调好全局链之后点右上角 + 存一条',
            style: theme.littleTextStyle,
            textAlign: TextAlign.center,
          ),
        ),
      );
    }

    return ListView.builder(
      controller: _scrollController,
      padding: const EdgeInsets.symmetric(vertical: 4),
      itemCount: chain.presets.length,
      itemBuilder: (context, index) =>
          _buildItem(chain, theme, chain.presets[index]),
    );
  }

  Widget _buildItem(
    PluginChainProvider chain,
    AppTheme theme,
    ChainPresetItem preset,
  ) {
    return InkWell(
      // 单击直接载入并下发（播放器场景不需要二次确认）
      onTap: () => _applyPreset(chain, preset),
      // 双击改名 / 改链
      onDoubleTap: () => _openPresetDialog(chain, preset: preset),
      onSecondaryTapDown: (details) =>
          _showMenu(chain, preset, details.globalPosition),
      child: Container(
        margin: const EdgeInsets.symmetric(horizontal: 6, vertical: 1),
        padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 6),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              preset.title,
              style: theme.midTextStyle,
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
            ),
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

  Widget _buildFooter(PluginChainProvider chain, AppTheme theme) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(10, 6, 10, 8),
      child: Text(
        '当前全局链：${chain.length} 个 · 延迟 ${chain.totalLatency} 采样',
        style: theme.littleTextStyle.copyWith(fontSize: 11),
      ),
    );
  }

  // ── 交互 ──────────────────────────────────────────────────────────
  void _showMenu(
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

  void _applyPreset(PluginChainProvider chain, ChainPresetItem preset) {
    if (!chain.applyPreset(preset.id)) {
      final error = chain.lastError;
      chain.clearError();
      UtilFunction.showErrorMessage(error ?? '载入预设失败');
      return;
    }
    UtilFunction.showInfoMessage('已载入预设「${preset.title}」');
  }

  /// preset == null：新建（默认拷一份当前全局链）
  Future<void> _openPresetDialog(
    PluginChainProvider chain, {
    required ChainPresetItem? preset,
  }) async {
    var initialSlots = _cloneChain(chain.slots);

    if (preset != null) {
      final loaded = chain.readPresetChain(preset.id);
      if (loaded == null) {
        UtilFunction.showErrorMessage('预设读不出来，可能数据损坏');
        return;
      }
      initialSlots = loaded.slots;
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
      if (!chain.updatePresetChain(preset.id, result.title, result.slots)) {
        UtilFunction.showErrorMessage('更新预设失败');
        return;
      }
      UtilFunction.showInfoMessage('已更新预设「${result.title}」');
    }

    if (!result.applyToGlobal) return;

    chain.loadChain(result.slots);
    if (chain.apply()) {
      UtilFunction.showInfoMessage('已应用到全局链，正在加载插件…');
    } else {
      final error = chain.lastError;
      chain.clearError();
      UtilFunction.showErrorMessage(error ?? '下发效果器链失败');
    }
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
