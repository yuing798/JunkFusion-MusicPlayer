/// 均衡器的中间栏：预设列表
///
/// ── 为什么中间栏放预设而不是 band 参数 ────────────────────────────────
/// band 的参数在主内容区底部 inspector 里改（选中曲线上的点就出来），
/// 因为 EQ 是"直接操作"的 UI，参数应该贴着曲线。
/// 而这个位置留给预设：对播放器来说"一键换风格"的使用频率高得多，
/// 而且预设天生就是列表，正是中间栏的形状。
///
/// ── 数据 ──────────────────────────────────────────────────────────────
/// 全走 EqProvider → FFI → Coordinator 的 eqPresets 表。
/// 只存 id/title/createTime，参数体按需再拉（getPresetParams）。
library;

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/eq_param.dart';
import 'package:junk_fusion_ui/providers/eq_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:provider/provider.dart';

class EqPresetSidebar extends StatefulWidget {
  const EqPresetSidebar({super.key});

  @override
  State<EqPresetSidebar> createState() => _EqPresetSidebarState();
}

class _EqPresetSidebarState extends State<EqPresetSidebar> {
  @override
  void initState() {
    super.initState();

    // 先去后端拉一次（同一个 provider，主内容区那边也会调，重复调用无副作用）
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted) return;
      context.read<EqProvider>().ensureInitialized();
    });
  }

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<EqProvider>();
    final theme = context.watch<AppTheme>();

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        _buildHeader(provider, theme),
        Divider(height: 1, color: theme.colorEdge),
        Expanded(child: _buildList(provider, theme)),
        Divider(height: 1, color: theme.colorEdge),
        _buildFooter(provider, theme),
      ],
    );
  }

  Widget _buildHeader(EqProvider provider, AppTheme theme) {
    return SizedBox(
      height: 36,
      child: Row(
        children: [
          const SizedBox(width: 10),
          Text('EQ 预设', style: theme.midTextStyle),
          const Spacer(),
          IconButton(
            tooltip: '把当前 EQ 存成新预设',
            icon: const Icon(Icons.add, size: 20),
            onPressed: () => _saveCurrent(provider),
          ),
        ],
      ),
    );
  }

  Widget _buildList(EqProvider provider, AppTheme theme) {
    if (provider.presets.isEmpty) {
      return Center(
        child: Padding(
          padding: const EdgeInsets.all(12),
          child: Text(
            '还没有预设\n点右上角 + 把当前 EQ 存下来',
            style: theme.littleTextStyle,
            textAlign: TextAlign.center,
          ),
        ),
      );
    }

    return ListView.builder(
      padding: const EdgeInsets.symmetric(vertical: 4),
      itemCount: provider.presets.length,
      itemBuilder: (context, index) {
        final preset = provider.presets[index];
        return _buildItem(provider, theme, preset);
      },
    );
  }

  Widget _buildItem(EqProvider provider, AppTheme theme, EqPresetItem preset) {
    final isActive = provider.activePresetId == preset.id;

    return InkWell(
      // 单击直接应用（播放器场景，不需要二次确认）
      onTap: () {
        if (!provider.applyPreset(preset.id)) {
          _showMessage('这个预设读不出来，可能数据损坏');
        }
      },
      child: Container(
        margin: const EdgeInsets.symmetric(horizontal: 6, vertical: 1),
        padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 6),
        decoration: BoxDecoration(
          color: isActive ? theme.colorStress.withValues(alpha: 0.25) : null,
          borderRadius: BorderRadius.circular(4),
        ),
        child: Row(
          children: [
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    preset.title,
                    style: theme.midTextStyle,
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                  Text(preset.createTime, style: theme.littleTextStyle),
                ],
              ),
            ),
            _buildItemMenu(provider, preset),
          ],
        ),
      ),
    );
  }

  Widget _buildItemMenu(EqProvider provider, EqPresetItem preset) {
    return PopupMenuButton<String>(
      tooltip: '更多',
      icon: const Icon(Icons.more_horiz, size: 18),
      padding: EdgeInsets.zero,
      onSelected: (value) {
        if (value == 'rename') {
          _rename(provider, preset);
        } else if (value == 'delete') {
          _delete(provider, preset);
        }
      },
      itemBuilder: (context) => const [
        PopupMenuItem(value: 'rename', child: Text('重命名')),
        PopupMenuItem(value: 'delete', child: Text('删除')),
      ],
    );
  }

  Widget _buildFooter(EqProvider provider, AppTheme theme) {
    return Padding(
      padding: const EdgeInsets.all(6),
      child: TextButton.icon(
        onPressed: provider.resetAll,
        icon: const Icon(Icons.restart_alt, size: 18),
        label: const Text('恢复出厂 EQ'),
      ),
    );
  }

  // ── 对话框 ────────────────────────────────────────────────────────
  Future<void> _saveCurrent(EqProvider provider) async {
    final title = await _askTitle(
      title: '保存预设',
      hint: '给这个 EQ 起个名字',
      confirmText: '保存',
    );
    if (title == null || !mounted) return;

    final id = provider.saveCurrentAsPreset(title);
    if (id <= 0) _showMessage('保存失败，看看后端日志');
  }

  Future<void> _rename(EqProvider provider, EqPresetItem preset) async {
    final title = await _askTitle(
      title: '重命名预设',
      hint: preset.title,
      initialValue: preset.title,
      confirmText: '确定',
    );
    if (title == null || !mounted) return;

    provider.renamePreset(preset.id, title);
  }

  Future<void> _delete(EqProvider provider, EqPresetItem preset) async {
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('删除预设'),
        content: Text('确定删除「${preset.title}」吗？'),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(false),
            child: const Text('取消'),
          ),
          TextButton(
            onPressed: () => Navigator.of(context).pop(true),
            child: const Text('删除'),
          ),
        ],
      ),
    );
    if (confirmed != true || !mounted) return;

    provider.deletePreset(preset.id);
  }

  Future<String?> _askTitle({
    required String title,
    required String hint,
    required String confirmText,
    String? initialValue,
  }) {
    final controller = TextEditingController(text: initialValue ?? '');

    return showDialog<String>(
      context: context,
      builder: (context) => AlertDialog(
        title: Text(title),
        content: TextField(
          controller: controller,
          autofocus: true,
          decoration: InputDecoration(hintText: hint),
          onSubmitted: (value) => Navigator.of(context).pop(value.trim()),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('取消'),
          ),
          TextButton(
            onPressed: () =>
                Navigator.of(context).pop(controller.text.trim()),
            child: Text(confirmText),
          ),
        ],
      ),
    ).whenComplete(controller.dispose);
  }

  void _showMessage(String message) {
    if (!mounted) return;
    ScaffoldMessenger.maybeOf(
      context,
    )?.showSnackBar(SnackBar(content: Text(message)));
  }
}
