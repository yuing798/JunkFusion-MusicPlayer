/// 设置页的中间栏：分节目录
///
/// ── 它是什么 ──────────────────────────────────────────────────────────
/// 就是设置页的"标题目录"：一列分节名字，点哪个，右侧主内容区就平滑滚到
/// 对应那一节。分节的定义（标题 / 图标 / 副标题）全在
/// `providers/settings_nav_provider.dart` 的 [SettingsSection] 里，
/// 这里只负责画，不加新数据。
///
/// ── 为什么这里不存滚动控制器 ──────────────────────────────────────────
/// 滚动发生在**主内容区**（settings_page.dart 持有 SmoothScrollController），
/// 中间栏和它是两棵子树。所以点击只能写一个"跳转请求"进 SettingsNavProvider，
/// 由页面消费并执行（见那个类顶部的"请求-消费模型"）。
///
/// 用法（只有 widgets/nav/pages_nav_table.dart 引用）：
///     const SettingsSidebar()
library;

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/providers/settings_nav_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:provider/provider.dart';

class SettingsSidebar extends StatelessWidget {
  const SettingsSidebar({super.key});

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final nav = context.watch<SettingsNavProvider>();

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        // 标题行：高度和 SidebarHost 的占位标题对齐（50px），换段时不会跳
        SizedBox(
          height: 50,
          child: Padding(
            padding: const EdgeInsets.symmetric(horizontal: 12),
            child: Align(
              alignment: Alignment.centerLeft,
              child: Text(
                '设置',
                style: theme.midTextStyle.copyWith(fontWeight: FontWeight.bold),
                maxLines: 1,
                overflow: TextOverflow.ellipsis,
              ),
            ),
          ),
        ),
        Container(height: 1, color: theme.colorEdge.withAlpha(120)),

        Expanded(
          child: ListView(
            padding: const EdgeInsets.symmetric(vertical: 6),
            children: [
              for (final section in SettingsSection.values)
                _buildItem(context, theme, nav, section),
            ],
          ),
        ),
      ],
    );
  }

  Widget _buildItem(
    BuildContext context,
    AppTheme theme,
    SettingsNavProvider nav,
    SettingsSection section,
  ) {
    final isActive = nav.active == section;

    return InkWell(
      // 点目录 = 请求页面滚到那一节；真正的滚动在 settings_page.dart 里做
      onTap: () => context.read<SettingsNavProvider>().requestJump(section),
      child: Container(
        margin: const EdgeInsets.symmetric(horizontal: 6, vertical: 1),
        padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 6),
        decoration: BoxDecoration(
          color: isActive ? theme.colorStress.withValues(alpha: 0.22) : null,
          borderRadius: BorderRadius.circular(4),
        ),
        child: Row(
          children: [
            Icon(
              section.icon,
              size: 18,
              color: isActive ? theme.colorStress : theme.colorTextSecond,
            ),
            const SizedBox(width: 8),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    section.title,
                    style: theme.midTextStyle.copyWith(
                      fontWeight: isActive
                          ? FontWeight.bold
                          : FontWeight.normal,
                    ),
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                  Text(
                    section.subtitle,
                    style: theme.littleTextStyle.copyWith(fontSize: 12),
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}
