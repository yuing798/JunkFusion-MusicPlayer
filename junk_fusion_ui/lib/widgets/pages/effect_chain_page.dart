/// 效果器页（左侧导航 id = 8）：全局效果器链
///
/// ── 这一页是什么 ──────────────────────────────────────────────────────
/// 8 个插槽，串在**交叉淡化之后、EQ 之前**（单条路径、全局共享、不需要 PDC）。
/// 路由理由见 docs/audio-routing.md §4.1。
///
/// ── 交互 ──────────────────────────────────────────────────────────────
///   · 每个槽：选插件 / 清除 / 单槽旁通（旁通**立即生效**，不用点应用）
///   · 整链旁通：一键回到干声（插件行为异常时的逃生口）
///   · 「应用」：把 8 槽配置下发给音频进程（会实例化插件，几百毫秒）
///   · 预设：把当前 8 槽（含每个插件的 state）存下来 / 一键切回
///
/// ── 为什么要有"应用"这个按钮而不是改了就生效 ──────────────────────────
/// 换插件要实例化（LoadLibrary + prepare），每次改动都重建链会又慢又吵；
/// 而且用户常常一次改好几个槽。旁通这种纯开关才做成即时。
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
import 'package:provider/provider.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

class EffectChainPage extends StatefulWidget {
  const EffectChainPage({super.key});

  @override
  State<EffectChainPage> createState() => _EffectChainPageState();
}

class _EffectChainPageState extends State<EffectChainPage> {
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

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        _buildHeader(theme, chain),
        const SizedBox(height: 6),
        Expanded(
          child: ListView(
            padding: const EdgeInsets.only(bottom: 24),
            children: [
              for (int i = 0; i < PluginChainProvider.slotCount; i++)
                _buildSlotRow(theme, chain, plugins, i),
              const SizedBox(height: 18),
              _buildPresetArea(theme, chain),
            ],
          ),
        ),
      ],
    );
  }

  // ── 顶部：标题 + 整链旁通 + 应用 ────────────────────────────────────
  Widget _buildHeader(AppTheme theme, PluginChainProvider chain) {
    final latencyMs = chain.totalLatency / 44.1; // 只是给个量级，采样率未知
    return SizedBox(
      height: 56,
      child: Row(
        children: [
          Text('效果器链', style: theme.bigTextStyle),
          const SizedBox(width: 10),
          Text('全局 · 交叉淡化之后', style: theme.littleTextStyle),
          if (chain.totalLatency > 0) ...[
            const SizedBox(width: 10),
            Tooltip(
              message: '整条链的插件延迟（采样）。单路径不需要补偿，只影响"听到"比"显示"晚一点',
              child: Text(
                '延迟 ${chain.totalLatency} 采样（≈${latencyMs.toStringAsFixed(0)}ms）',
                style: theme.littleTextStyle,
              ),
            ),
          ],
          const Spacer(),

          // 整链旁通：逃生口，立即生效
          Text('整链旁通', style: theme.littleTextStyle),
          Switch(
            value: chain.bypassAll,
            onChanged: (_) => chain.toggleBypassAll(),
          ),
          const SizedBox(width: 8),

          FilledButton.tonal(
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
    );
  }

  // ── 一个插槽 ──────────────────────────────────────────────────────
  Widget _buildSlotRow(
    AppTheme theme,
    PluginChainProvider chain,
    List<PluginItem> plugins,
    int index,
  ) {
    final slot = chain.slots[index];
    final status = chain.statusOf(index);

    // 运行状态优先级：加载报错 > 已加载 > 未应用 / 空槽
    String statusText;
    Color statusColor = theme.colorTextSecond;
    if (status.error.isNotEmpty) {
      statusText = status.error;
      statusColor = theme.colorError;
    } else if (status.loaded) {
      statusText = status.latency > 0 ? '已加载 · 延迟 ${status.latency}' : '已加载';
    } else if (slot.isEmpty) {
      statusText = '空插槽';
    } else {
      statusText = '未应用';
    }

    return Container(
      height: 62,
      margin: const EdgeInsets.only(bottom: 6),
      padding: const EdgeInsets.symmetric(horizontal: 10),
      decoration: BoxDecoration(
        color: theme.colorCell,
        border: Border.all(color: theme.colorEdge),
        borderRadius: BorderRadius.circular(6),
      ),
      child: Row(
        children: [
          SizedBox(
            width: 26,
            child: Text('${index + 1}', style: theme.midTextStyle),
          ),
          const SizedBox(width: 4),
          Icon(
            slot.isEmpty ? TablerIcons.plugOff : TablerIcons.plugConnected,
            size: 18,
            color: slot.isEmpty ? theme.colorEdge : theme.colorStress,
          ),
          const SizedBox(width: 8),

          // 名字 + 状态
          Expanded(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  slot.isEmpty ? '（空插槽）' : slot.name,
                  style: theme.midTextStyle.copyWith(
                    color: slot.isEmpty
                        ? theme.colorTextSecond
                        : theme.colorTextMain,
                  ),
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
                Text(
                  statusText,
                  style: theme.littleTextStyle.copyWith(
                    fontSize: 12,
                    color: statusColor,
                  ),
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
              ],
            ),
          ),

          Text(slot.bypass ? '已旁通' : '', style: theme.littleTextStyle),
          Switch(
            value: slot.bypass,
            onChanged: slot.isEmpty
                ? null
                : (_) => chain.toggleSlotBypass(index),
          ),
          TextButton(
            onPressed: () => _pickPlugin(context, index, plugins),
            child: Text(slot.isEmpty ? '选择' : '更换'),
          ),
          // 打开插件自己的界面：**没有这个就没法调音**，
          // 只能听插件默认参数（而且很多插件默认是干的）
          IconButton(
            tooltip: status.loaded ? '打开插件编辑器' : '先点「应用」，插件加载后才有编辑器',
            icon: const Icon(Icons.tune, size: 18),
            onPressed: status.loaded
                ? () {
                    chain.openEditor(index);
                    UtilFunction.showInfoMessage('正在打开「${slot.name}」的编辑器…');
                  }
                : null,
          ),
          IconButton(
            tooltip: '清空这个插槽',
            icon: const Icon(Icons.close, size: 16),
            onPressed: slot.isEmpty ? null : () => chain.clearSlot(index),
          ),
        ],
      ),
    );
  }

  // ── 预设区 ────────────────────────────────────────────────────────
  Widget _buildPresetArea(AppTheme theme, PluginChainProvider chain) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            Text('链预设', style: theme.midTextStyle),
            const SizedBox(width: 8),
            Text('存的是"用了哪些插件 + 每个插件的状态"', style: theme.littleTextStyle),
            const Spacer(),
            TextButton.icon(
              onPressed: () => _savePreset(context, chain),
              icon: const Icon(Icons.add, size: 18),
              label: const Text('把当前链存为预设'),
            ),
          ],
        ),
        const SizedBox(height: 6),
        if (chain.presets.isEmpty)
          Padding(
            padding: const EdgeInsets.symmetric(vertical: 8),
            child: Text('还没有预设', style: theme.littleTextStyle),
          )
        else
          Wrap(
            spacing: 8,
            runSpacing: 8,
            children: [
              for (final preset in chain.presets)
                _buildPresetChip(theme, chain, preset),
            ],
          ),
      ],
    );
  }

  Widget _buildPresetChip(
    AppTheme theme,
    PluginChainProvider chain,
    ChainPresetItem preset,
  ) {
    return GestureDetector(
      onTap: () => _applyPreset(chain, preset),
      // 右键删除（项目规范：右键菜单统一走 showRootContextMenu）
      onSecondaryTapDown: (details) {
        showRootContextMenu(
          context: context,
          position: details.globalPosition,
          useBarrier: true,
          items: [
            ContextMenuItem(
              label: '删除预设',
              onTap: () => chain.deletePreset(preset.id),
            ),
          ],
        );
      },
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
        decoration: BoxDecoration(
          color: theme.colorHover,
          borderRadius: BorderRadius.circular(6),
          border: Border.all(color: theme.colorEdge),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(TablerIcons.playlist, size: 16, color: theme.colorTextSecond),
            const SizedBox(width: 6),
            Text(preset.title, style: theme.littleTextStyle),
          ],
        ),
      ),
    );
  }

  // ── 交互 ──────────────────────────────────────────────────────────
  Future<void> _pickPlugin(
    BuildContext context,
    int slot,
    List<PluginItem> allPlugins,
  ) async {
    // 只列"能当效果器用"的：类型是 effect、扫描成功、有 identifier（没有它没法定位加载）
    final candidates =
        allPlugins
            .where(
              (plugin) =>
                  plugin.type == PluginType.effect &&
                  !plugin.isFailed &&
                  plugin.identifier.isNotEmpty,
            )
            .toList()
          ..sort(
            (a, b) => a.name.toLowerCase().compareTo(b.name.toLowerCase()),
          );

    final selected = await showDialog<PluginItem>(
      context: context,
      builder: (dialogContext) {
        final theme = dialogContext.watch<AppTheme>();
        return AlertDialog(
          title: Text('选择插件（插槽 ${slot + 1}）'),
          content: SizedBox(
            width: 520,
            height: 420,
            child: candidates.isEmpty
                ? Center(
                    child: Text(
                      '没有可用的效果器插件\n先去设置页扫描，或把分类改成「效果器」',
                      textAlign: TextAlign.center,
                      style: theme.littleTextStyle,
                    ),
                  )
                : ListView.builder(
                    itemCount: candidates.length,
                    itemBuilder: (context, index) {
                      final plugin = candidates[index];
                      return ListTile(
                        dense: true,
                        title: Text(plugin.name, style: theme.midTextStyle),
                        subtitle: Text(
                          plugin.vendor.isEmpty
                              ? plugin.filePath
                              : plugin.vendor,
                          style: theme.littleTextStyle,
                          maxLines: 1,
                          overflow: TextOverflow.ellipsis,
                        ),
                        onTap: () => Navigator.of(dialogContext).pop(plugin),
                      );
                    },
                  ),
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(dialogContext).pop(),
              child: const Text('取消'),
            ),
          ],
        );
      },
    );

    if (selected == null || !context.mounted) return;
    context.read<PluginChainProvider>().loadSlot(slot, selected);
  }

  void _applyChain(PluginChainProvider chain) {
    if (chain.apply()) {
      UtilFunction.showInfoMessage('已下发效果器链，正在加载插件…');
      return;
    }
    final error = chain.lastError;
    chain.clearError();
    UtilFunction.showErrorMessage(error ?? '下发效果器链失败');
  }

  void _applyPreset(PluginChainProvider chain, ChainPresetItem preset) {
    if (chain.applyPreset(preset.id)) {
      UtilFunction.showInfoMessage('已切换到「${preset.title}」');
      return;
    }
    final error = chain.lastError;
    chain.clearError();
    UtilFunction.showErrorMessage(error ?? '切换预设失败');
  }

  Future<void> _savePreset(
    BuildContext context,
    PluginChainProvider chain,
  ) async {
    final controller = TextEditingController();
    final title = await showDialog<String>(
      context: context,
      builder: (dialogContext) => AlertDialog(
        title: const Text('保存为预设'),
        content: TextField(
          controller: controller,
          autofocus: true,
          decoration: const InputDecoration(hintText: '例如：母带 · 温暖'),
          onSubmitted: (value) => Navigator.of(dialogContext).pop(value.trim()),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(dialogContext).pop(),
            child: const Text('取消'),
          ),
          TextButton(
            onPressed: () =>
                Navigator.of(dialogContext).pop(controller.text.trim()),
            child: const Text('保存'),
          ),
        ],
      ),
    ).whenComplete(controller.dispose);

    if (title == null || title.isEmpty || !context.mounted) return;

    final id = chain.savePreset(title);
    if (id > 0) {
      UtilFunction.showInfoMessage('已保存预设「$title」');
    } else {
      UtilFunction.showErrorMessage('保存预设失败');
    }
  }
}
