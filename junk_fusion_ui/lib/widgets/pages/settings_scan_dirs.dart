/// 插件扫描目录（设置页「插件管理」里的第一块）
///
/// ── 它解决什么 ────────────────────────────────────────────────────────
/// 扫描范围 = `pluginDirs` 表里的所有目录。系统默认目录
/// （`C:\Program Files\Common Files\VST3`）由后端启动时自己插，用户只能
/// 增删**自己的**目录；删默认目录会被后端拒绝，这里把那个按钮直接隐藏。
///
/// ── 交互 ──────────────────────────────────────────────────────────────
///   添加：FilePicker 选一个文件夹 → `addPluginDir` → 刷新列表
///         （加完要点旁边的「开始扫描」才会真的扫进去）
///   删除：只删"扫描范围"，**不会**动已经入库的插件行
///         （后端规格：目录移除后 plugins 行保留，加回来就还在）
///   失败：后端会给一句中文原因（"系统默认目录不能删除" / "目录不存在"），
///         用 SnackBar 显示
///
/// ── 用法 ──────────────────────────────────────────────────────────────
///     const SettingsScanDirs()      // 只有 settings_page.dart 用
library;

import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/plugin_info.dart';
import 'package:junk_fusion_ui/providers/plugin_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:provider/provider.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

class SettingsScanDirs extends StatelessWidget {
  const SettingsScanDirs({super.key});

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<PluginProvider>();
    final theme = context.watch<AppTheme>();
    final dirs = provider.dirs;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Row(
          children: [
            Text('扫描目录', style: theme.midTextStyle),
            const SizedBox(width: 8),
            Text('共 ${dirs.length} 个', style: theme.littleTextStyle),
            const Spacer(),
            TextButton.icon(
              onPressed: () => _addDir(context, provider),
              icon: const Icon(Icons.create_new_folder_outlined, size: 18),
              label: const Text('添加文件夹'),
            ),
          ],
        ),
        const SizedBox(height: 4),

        if (dirs.isEmpty)
          Padding(
            padding: const EdgeInsets.symmetric(vertical: 12),
            child: Text('没有可用的扫描目录', style: theme.littleTextStyle),
          )
        else
          Container(
            decoration: BoxDecoration(
              border: Border.all(color: theme.colorEdge),
              borderRadius: BorderRadius.circular(6),
            ),
            clipBehavior: Clip.antiAlias,
            child: Column(
              children: [
                for (var i = 0; i < dirs.length; i++) ...[
                  if (i > 0) Divider(height: 1, color: theme.colorEdge),
                  _buildRow(context, theme, provider, dirs[i]),
                ],
              ],
            ),
          ),
      ],
    );
  }

  Widget _buildRow(
    BuildContext context,
    AppTheme theme,
    PluginProvider provider,
    PluginDirItem dir,
  ) {
    return SizedBox(
      height: 38,
      child: Row(
        children: [
          const SizedBox(width: 10),
          Icon(TablerIcons.folder, size: 16, color: theme.colorTextSecond),
          const SizedBox(width: 8),

          // 路径：单行省略，悬停看全
          Expanded(
            child: Tooltip(
              message: dir.path,
              waitDuration: const Duration(milliseconds: 600),
              child: Text(
                dir.path,
                style: theme.littleTextStyle.copyWith(fontSize: 13),
                maxLines: 1,
                overflow: TextOverflow.ellipsis,
              ),
            ),
          ),
          const SizedBox(width: 8),

          if (dir.isBuiltin)
            // 系统目录：不给删除按钮（后端也会拒绝），只标一个标签
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
              margin: const EdgeInsets.only(right: 12),
              decoration: BoxDecoration(
                color: theme.colorHover,
                borderRadius: BorderRadius.circular(4),
              ),
              child: Text(
                '系统',
                style: theme.littleTextStyle.copyWith(fontSize: 11),
              ),
            )
          else
            IconButton(
              tooltip: '从扫描范围移除（已入库的插件不会删）',
              icon: const Icon(Icons.close, size: 16),
              visualDensity: VisualDensity.compact,
              onPressed: () => _removeDir(context, provider, dir.path),
            ),
        ],
      ),
    );
  }

  /// 选一个文件夹加进扫描范围
  Future<void> _addDir(BuildContext context, PluginProvider provider) async {
    final path = await FilePicker.getDirectoryPath(
      dialogTitle: '选择 VST3 插件目录',
      lockParentWindow: true,
    );
    if (path == null || path.isEmpty) return;
    if (!context.mounted) return;

    if (provider.addDir(path)) return;
    _showError(context, provider, '添加目录失败');
  }

  Future<void> _removeDir(
    BuildContext context,
    PluginProvider provider,
    String path,
  ) async {
    if (provider.removeDir(path)) return;
    _showError(context, provider, '删除目录失败');
  }

  void _showError(
    BuildContext context,
    PluginProvider provider,
    String fallback,
  ) {
    final error = provider.lastError;
    provider.clearError();
    ScaffoldMessenger.maybeOf(
      context,
    )?.showSnackBar(SnackBar(content: Text(error ?? fallback)));
  }
}
