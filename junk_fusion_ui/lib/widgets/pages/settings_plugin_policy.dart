/// 设置页的「插件名单」表：黑名单 + 白名单，**单独一张表放在扫描表下面**
///
/// ── 为什么不和扫描表并排放 ────────────────────────────────────────────
/// 扫描表的列已经排到"路径"，再加"黑名单/白名单"要横向滚很远，
/// 而这一列恰恰是最需要一眼看到的。两张表分行放，各管各的。
///
/// ── 两行数据的含义（host-integration.md §6）────────────────────────────
///   黑名单：系统观测（"这个插件把进程弄崩过 N 次"），只能解除，用户改不了原因；
///   白名单：用户意图（"我知道它有问题，但我要用"），优先级最高，
///           但它只是"解除自动禁止"，下一次仍然会先探测。
///
/// 文档要求**全量显示**：黑名单、白名单、正常插件都要能看到，不隐藏
/// —— 隐藏会让人以为插件"消失了"。
///
/// 用法（只有 settings_page.dart 引用）：
///     const SettingsPluginPolicy()
library;

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/plugin_chain.dart';
import 'package:junk_fusion_ui/providers/plugin_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:provider/provider.dart';

class SettingsPluginPolicy extends StatelessWidget {
  const SettingsPluginPolicy({super.key});

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final provider = context.watch<PluginProvider>();

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Row(
          children: [
            Text('插件名单', style: theme.midTextStyle),
            const SizedBox(width: 8),
            Text(
              '黑名单 ${provider.blacklist.length} · 白名单 ${provider.whitelist.length}',
              style: theme.littleTextStyle,
            ),
            const Spacer(),
            IconButton(
              tooltip: '重新读取',
              icon: const Icon(Icons.refresh, size: 20),
              onPressed: provider.refreshPolicy,
            ),
          ],
        ),
        const SizedBox(height: 4),

        Container(
          decoration: BoxDecoration(
            border: Border.all(color: theme.colorEdge),
            borderRadius: BorderRadius.circular(6),
          ),
          clipBehavior: Clip.antiAlias,
          child: Column(
            children: [
              _buildHeader(theme),
              Container(height: 1, color: theme.colorEdge),
              if (provider.blacklist.isEmpty && provider.whitelist.isEmpty)
                Padding(
                  padding: const EdgeInsets.symmetric(vertical: 22),
                  child: Text('没有插件被拉黑或强制启用', style: theme.littleTextStyle),
                )
              else ...[
                for (final ban in provider.blacklist)
                  _buildBanRow(context, theme, provider, ban),
                for (final allow in provider.whitelist)
                  _buildAllowRow(context, theme, provider, allow),
              ],
            ],
          ),
        ),
        const SizedBox(height: 6),
        Text(
          '黑名单是"它把进程弄崩过"的系统记录；白名单是你强制允许用的插件（优先级最高）。'
          '被拉黑的插件不会自动进效果器链，解除后需要重新应用一次链。',
          style: theme.littleTextStyle.copyWith(fontSize: 12),
        ),
      ],
    );
  }

  Widget _buildHeader(AppTheme theme) {
    final style = theme.littleTextStyle.copyWith(
      fontSize: 13,
      fontWeight: FontWeight.bold,
    );

    return Container(
      height: 36,
      color: theme.colorCell,
      child: Row(
        children: [
          _cell('插件', 260, style),
          _cell('状态', 160, style),
          _cell('原因 / 备注', 220, style),
          _cell('时间', 150, style),
          const Spacer(),
          _cell('操作', 180, style),
        ],
      ),
    );
  }

  Widget _buildBanRow(
    BuildContext context,
    AppTheme theme,
    PluginProvider provider,
    PluginBanItem ban,
  ) {
    return Container(
      height: 44,
      decoration: BoxDecoration(color: theme.colorCell.withValues(alpha: 0.35)),
      child: Row(
        children: [
          _cell(
            _nameOf(provider, ban.identifier, ban.filePath),
            260,
            theme.littleTextStyle,
          ),
          _cell(
            '黑名单',
            160,
            theme.littleTextStyle.copyWith(
              color: theme.colorError,
              fontSize: 13,
            ),
          ),
          _cell(
            '${ban.reason}（${ban.failureCount} 次）',
            220,
            theme.littleTextStyle,
          ),
          _cell(ban.lastFailedAt, 150, theme.littleTextStyle),
          const Spacer(),
          Row(
            children: [
              TextButton(
                onPressed: () {
                  provider.removeBlacklist(ban.identifier);
                },
                child: const Text('解除黑名单'),
              ),
              TextButton(
                onPressed: () {
                  provider.addWhitelist(ban.identifier, ban.filePath);
                },
                child: const Text('强制启用'),
              ),
            ],
          ),
          const SizedBox(width: 6),
        ],
      ),
    );
  }

  Widget _buildAllowRow(
    BuildContext context,
    AppTheme theme,
    PluginProvider provider,
    PluginAllowItem allow,
  ) {
    return Container(
      height: 44,
      child: Row(
        children: [
          _cell(
            _nameOf(provider, allow.identifier, allow.filePath),
            260,
            theme.littleTextStyle,
          ),
          _cell(
            '白名单（强制）',
            160,
            theme.littleTextStyle.copyWith(
              color: theme.colorStress,
              fontSize: 13,
            ),
          ),
          _cell(
            allow.note.isEmpty ? '—' : allow.note,
            220,
            theme.littleTextStyle,
          ),
          _cell(allow.createdAt, 150, theme.littleTextStyle),
          const Spacer(),
          Row(
            children: [
              TextButton(
                onPressed: () => provider.removeWhitelist(allow.identifier),
                child: const Text('取消强制'),
              ),
            ],
          ),
          const SizedBox(width: 6),
        ],
      ),
    );
  }

  Widget _cell(String text, double width, TextStyle style) {
    return SizedBox(
      width: width,
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 10),
        child: Align(
          alignment: Alignment.centerLeft,
          child: Text(
            text,
            style: style,
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
          ),
        ),
      ),
    );
  }

  /// 名单里没有名字字段，从扫描结果里按 identifier 反查；查不到就用文件名
  String _nameOf(PluginProvider provider, String identifier, String filePath) {
    for (final plugin in provider.plugins) {
      if (plugin.identifier == identifier) return plugin.name;
    }
    if (filePath.isEmpty) return identifier;

    final normalized = filePath.replaceAll('\\', '/');
    final last = normalized.split('/').last;
    return last.isEmpty ? filePath : last;
  }
}
