/// 「挑一个效果器插件」对话框
///
/// ── 它挑什么 ──────────────────────────────────────────────────────────
/// 只列能进效果器链的插件：类型 = 效果器、上次扫描成功、**有 identifier**
/// （没有 identifier 就没法定位加载，见 PluginInstanceFactory 的实现）。
///
/// ── 用在哪 ────────────────────────────────────────────────────────────
///   · 效果器页工具栏的「+ 添加」；
///   · 预设编辑对话框里的「+ 添加」。
/// 两处是同一种操作，所以抽出来，别复制两份过滤逻辑。
///
/// ── 用法 ─────────────────────────────────────────────────────────────
/// ```dart
/// final plugin = await showPluginPickerDialog(context, allPlugins);
/// if (plugin != null) provider.addSlot(plugin);
/// ```
library;

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/plugin_info.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:provider/provider.dart';

/// 弹出插件选择框；用户取消返回 null
///
/// [allPlugins] 一般传 `context.read<PluginProvider>().plugins`。
Future<PluginItem?> showPluginPickerDialog(
  BuildContext context,
  List<PluginItem> allPlugins,
) {
  final candidates =
      allPlugins
          .where(
            (plugin) =>
                plugin.type == PluginType.effect &&
                !plugin.isFailed &&
                plugin.identifier.isNotEmpty,
          )
          .toList()
        ..sort((a, b) => a.name.toLowerCase().compareTo(b.name.toLowerCase()));

  return showDialog<PluginItem>(
    context: context,
    builder: (dialogContext) {
      final theme = dialogContext.watch<AppTheme>();

      return AlertDialog(
        title: const Text('添加效果器'),
        content: SizedBox(
          width: 540,
          height: 420,
          child: candidates.isEmpty
              ? Center(
                  child: Text(
                    '没有可用的效果器插件\n'
                    '先去设置页扫描，或者把插件的分类改成「效果器」',
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
                        plugin.vendor.isEmpty ? plugin.filePath : plugin.vendor,
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
}
