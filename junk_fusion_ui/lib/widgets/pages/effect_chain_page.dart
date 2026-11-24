/// 效果器页的主内容区：**全局效果器链**
///
/// ── 它在本段里的位置 ──────────────────────────────────────────────────
///   效果器这一段的中间栏是**链预设**（widgets/nav/effect_chain_preset_sidebar.dart），
///   主内容区才是链本体。两个板块的分工：
///     · 中间栏：存下来的链（预设），可以贴到全局链上，以后也可以贴到某个分轨；
///     · 内容区：正在生效的全局链 —— 它真的插在交叉淡化之后、EQ 之前出声，
///       所以有"应用 / 整链旁通 / 打开编辑器 / 已加载延迟"这些运行时概念。
///   和 EQ 页一个分工（预设住中间栏，工作区住内容区）。
///
/// ── 交互 ──────────────────────────────────────────────────────────────
///   · + 添加：从扫描到的效果器里挑一个追加到链尾
///   · 拖动 ⠿：调整顺序（**只改前端**，点应用才重建）
///   · ⏻：单卡旁通，立即生效
///   · 🎛：打开插件自己的界面（窗口在音频进程里）
///   · ✕：从链里移除
///   · 整链旁通：一键回到干声
///
/// ── 为什么"改"了还要点应用 ────────────────────────────────────────────
///   换插件/换顺序要重新实例化（实测 CHOWTapeModel 1.86 秒）。重建期间旧链照常出声，
///   所以这个按钮不是"省 CPU"，而是"让用户决定什么时候切声音"。
///
/// 用法（只有 pages_route.dart 引用）：
///     const EffectChainPage()
library;

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/plugin_info.dart';
import 'package:junk_fusion_ui/providers/plugin_chain_provider.dart';
import 'package:junk_fusion_ui/providers/plugin_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';
import 'package:junk_fusion_ui/widgets/plugin_chain/effect_chain_editor.dart';
import 'package:junk_fusion_ui/widgets/plugin_chain/plugin_picker_dialog.dart';
import 'package:provider/provider.dart';

class EffectChainPage extends StatefulWidget {
  const EffectChainPage({super.key});

  @override
  State<EffectChainPage> createState() => _EffectChainPageState();
}

class _EffectChainPageState extends State<EffectChainPage> {
  @override
  void initState() {
    super.initState();
    // 中间栏也会调，重复调用无副作用
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
        _buildToolbar(theme, chain, plugins),
        Expanded(
          child: Padding(
            padding: const EdgeInsets.fromLTRB(12, 8, 12, 8),
            // 上下排布：编辑区自己撑满高度，中间那段在链长时自己滚
            child: EffectChainEditor(
              slots: chain.slots,
              status: chain.status,
              onReorder: chain.moveSlot,
              onRemove: chain.removeSlot,
              onBypass: chain.toggleSlotBypass,
              onOpenEditor: chain.openEditor,
              onAdd: () => _addPlugin(chain, plugins),
              maxSlots: PluginChainProvider.maxSlots,
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

  Widget _buildToolbar(
    AppTheme theme,
    PluginChainProvider chain,
    List<PluginItem> plugins,
  ) {
    return SizedBox(
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
    );
  }

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
}
