/// 设置页（左侧导航 id = 11）
///
/// ── 结构 ──────────────────────────────────────────────────────────────
///   中间栏（widgets/nav/settings_sidebar.dart）：分节目录，点哪跳哪；
///   主内容区（本文件）：所有分节竖着排成一列，共用一个 [SmoothScrollController]。
///
/// ── 目录怎么让主内容区滚动 ────────────────────────────────────────────
/// 两栏是两棵子树，碰不到对方的 ScrollController。所以走
/// `SettingsNavProvider` 的"请求-消费"：目录只写一个待处理请求，
/// 本页在 initState 里挂监听，收到后把请求取走（takePending）、
/// 再用锚点 key 算目标偏移并 animateTo。详见那个类的头注释。
///
/// ── 为什么用 getOffsetToReveal 而不是自己算 ───────────────────────────
/// 自己算要把每一节的高度加起来（标题、间距、表格的 clamp 高度都会变），
/// 一改布局就偏。`RenderAbstractViewport.getOffsetToReveal` 是 Flutter 给
/// "把这个子树滚到视口顶部"准备的官方 API，布局怎么变都对。
///
/// ── 以后加分节 ────────────────────────────────────────────────────────
/// 1. 在 `SettingsSection` 枚举里加一项；
/// 2. 本文件底部 `_buildContent` 的 switch 里加一行（switch 是穷尽的，
///    漏了会编译不过，不会出现"点了目录滚到空白"）。
///
/// 用法（只有 widgets/pages/pages_route.dart 引用）：
///     const SettingsPage()
library;

import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart' show RenderAbstractViewport;
import 'package:flutter_smooth_wheel_scroll/flutter_smooth_wheel_scroll.dart';
import 'package:junk_fusion_ui/model/plugin_info.dart';
import 'package:junk_fusion_ui/providers/plugin_provider.dart';
import 'package:junk_fusion_ui/providers/settings_nav_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/widgets/pages/settings_plugin_table.dart';
import 'package:provider/provider.dart';

class SettingsPage extends StatefulWidget {
  const SettingsPage({super.key});

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  /// 页面级平滑滚动器（目录跳转、滚轮都走它）
  final ScrollController _scroll = SmoothScrollController();

  /// 每一节的锚点：跳转时靠它找到那一节的 RenderObject
  final Map<SettingsSection, GlobalKey> _sectionKeys = {
    for (final section in SettingsSection.values)
      section: GlobalKey(debugLabel: section.debugName),
  };

  SettingsNavProvider? _nav;

  @override
  void initState() {
    super.initState();

    // 不能直接 context.read：initState 里还没进 build，provider 取不到是安全的，
    // 但 addPostFrameCallback 里更稳（也顺便等第一帧布局好再拉数据）
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted) return;

      // 插件库数据：只拉一次，重复进页面走 provider 的 ensureInitialized
      context.read<PluginProvider>().ensureInitialized();

      _nav = context.read<SettingsNavProvider>();
      _nav!.addListener(_onNavSignal);
      // 清掉进本页之前（比如上次离开时）残留的跳转请求
      _nav!.takePending();
    });
  }

  @override
  void dispose() {
    _nav?.removeListener(_onNavSignal);
    _scroll.dispose();
    super.dispose();
  }

  /// 目录被点了
  void _onNavSignal() {
    final pending = _nav?.takePending();
    if (pending == null) return;

    // 等这一帧布局完再量位置：目录点击可能刚好碰上列表刷新
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _scrollToSection(pending);
    });
  }

  void _scrollToSection(SettingsSection section) {
    if (!mounted || !_scroll.hasClients) return;

    final sectionContext = _sectionKeys[section]?.currentContext;
    final box = sectionContext?.findRenderObject() as RenderBox?;
    if (box == null) return;

    final viewport = RenderAbstractViewport.of(box);
    final targetOffset = viewport.getOffsetToReveal(box, 0.0).offset;

    _scroll.animateTo(
      targetOffset.clamp(0.0, _scroll.position.maxScrollExtent),
      duration: const Duration(milliseconds: 420),
      curve: Curves.easeOutCubic,
    );
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();

    return SingleChildScrollView(
      controller: _scroll,
      padding: const EdgeInsets.fromLTRB(4, 4, 12, 60),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          for (final section in SettingsSection.values)
            _buildSection(theme, section),
        ],
      ),
    );
  }

  // ── 一个分节 ──────────────────────────────────────────────────────
  Widget _buildSection(AppTheme theme, SettingsSection section) {
    return Container(
      // 锚点：目录跳转靠它定位（Container 只用来挂 key + 下边距）
      key: _sectionKeys[section],
      margin: const EdgeInsets.only(bottom: 28),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          _buildSectionHeader(theme, section),
          const SizedBox(height: 10),
          _buildContent(section),
        ],
      ),
    );
  }

  Widget _buildSectionHeader(AppTheme theme, SettingsSection section) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            Icon(section.icon, size: 20, color: theme.colorStress),
            const SizedBox(width: 8),
            Text(
              section.title,
              style: theme.bigTextStyle.copyWith(fontSize: 20),
            ),
          ],
        ),
        const SizedBox(height: 4),
        Text(section.subtitle, style: theme.littleTextStyle),
      ],
    );
  }

  /// 分节的内容。switch 是穷尽的：加了枚举没加内容会直接编译失败
  Widget _buildContent(SettingsSection section) {
    return switch (section) {
      SettingsSection.pluginLibrary => const _PluginLibrarySection(),
    };
  }
}

// ════════════════════════════════════════════════════════════════
// 插件库这一节的正文：汇总 + 操作栏 + 表格
//
// 操作栏（刷新 / 开始扫描）是做表格时顺手加的最小触发入口：
// 没有它，全新安装的库里一行插件都没有，表格永远是空的。
// 不需要的话整块删掉即可，表格本身不依赖它。
// ════════════════════════════════════════════════════════════════
class _PluginLibrarySection extends StatelessWidget {
  const _PluginLibrarySection();

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<PluginProvider>();
    final theme = context.watch<AppTheme>();

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        _buildToolbar(context, theme, provider),
        const SizedBox(height: 8),
        const SettingsPluginTable(),
      ],
    );
  }

  Widget _buildToolbar(
    BuildContext context,
    AppTheme theme,
    PluginProvider provider,
  ) {
    final summary =
        '共 ${provider.plugins.length} 个'
        '（音源 ${provider.countOf(PluginType.instrument)} · '
        '效果器 ${provider.countOf(PluginType.effect)} · '
        '其他 ${provider.countOf(PluginType.other)}'
        '${provider.failedCount > 0 ? ' · 失败 ${provider.failedCount}' : ''}）';

    // idle / running / 刚扫完的总结都走这一行
    final progress = provider.scan.progressText;

    return Row(
      children: [
        Text(summary, style: theme.littleTextStyle),

        if (provider.isScanning) ...[
          const SizedBox(width: 10),
          const SizedBox(
            width: 12,
            height: 12,
            child: CircularProgressIndicator(strokeWidth: 2),
          ),
        ],
        if (progress.isNotEmpty) ...[
          const SizedBox(width: 10),
          Flexible(
            child: Text(
              progress,
              style: theme.littleTextStyle.copyWith(color: theme.colorStress),
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
            ),
          ),
        ],

        const Spacer(),

        IconButton(
          tooltip: '重新读取插件库',
          icon: const Icon(Icons.refresh, size: 20),
          onPressed: provider.refresh,
        ),
        const SizedBox(width: 4),
        FilledButton.tonalIcon(
          // 扫描期间禁用，避免重复触发（后端也会拒绝第二次）
          onPressed: provider.isScanning
              ? null
              : () => _startScan(context, provider),
          icon: const Icon(Icons.manage_search, size: 18),
          label: Text(provider.isScanning ? '扫描中…' : '开始扫描'),
        ),
      ],
    );
  }

  void _startScan(BuildContext context, PluginProvider provider) {
    if (provider.startScan()) return;

    final error = provider.lastError;
    provider.clearError();
    ScaffoldMessenger.maybeOf(
      context,
    )?.showSnackBar(SnackBar(content: Text(error ?? '扫描没能启动')));
  }
}
