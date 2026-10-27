import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/widgets/nav/pages_nav_table.dart';
import 'package:junk_fusion_ui/widgets/pages/pages_route.dart';
import 'package:provider/provider.dart';

// ════════════════════════════════════════════════════════════════
// sidebar_host.dart — 三段式中间那一栏（子导航）
//
// 只做一件事：看当前在哪一段（PagesRouteController.currentNavId），
// 去 widgets/nav/pages_nav_table.dart 里取这一段的 sidebar 画出来。
//
// 中间栏跟随的是「段」，不是「具体哪一页」：
// 比如从专辑网格点进单张专辑，currentNavId 还是 3（专辑），
// 中间栏就保持原样，不会跟着内容区一起换。
//
// 各段的中间栏内容还没设计，sidebar 现在都是 null → 画 _SidebarPlaceholder。
// 以后在导航表里给某一段补上 sidebar，这里自动换掉，不用改这个文件。
//
// 用法（只有 ThreeColumnLayout 用到）：
//     const SidebarHost()
//
// TODO: 等各段真有了 sidebar，如果用户在段之间来回切时不想丢滚动位置/展开
//       状态，就把这里改成 IndexedStack（和 PagesRouteView 的做法一样），
//       只把「访问过的段」放进去，别一启动就把所有段都 build 一遍。
// ════════════════════════════════════════════════════════════════

/// 中间栏：按当前导航段切内容
class SidebarHost extends StatelessWidget {
  const SidebarHost({super.key});

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final selectedId = context.watch<PagesRouteController>().currentNavId;
    final section = PagesNavTable.byId(selectedId);

    return Container(
      color: theme.colorNav,
      child: section == null
          ? const SizedBox.expand()
          : section.sidebar?.call(context) ??
                _SidebarPlaceholder(title: section.title),
    );
  }
}

// ════════════════════════════════════════════════════════════════
// _SidebarPlaceholder — 还没有设计内容的中间栏
//
// 布局：标题行（50px，和内容页顶部那行标题对齐）→ 一条细线 → 居中提示。
// 三段式在所有页面都常驻，所以哪怕内容没定，也得有个像样的占位，
// 不能空一块白。
// ════════════════════════════════════════════════════════════════
class _SidebarPlaceholder extends StatelessWidget {
  /// 这一段的名字（「文件夹」「专辑」……）
  final String title;

  const _SidebarPlaceholder({required this.title});

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        // ── 标题行 ──
        SizedBox(
          height: 50,
          child: Padding(
            padding: const EdgeInsets.symmetric(horizontal: 12),
            child: Align(
              alignment: Alignment.centerLeft,
              child: Text(
                title,
                style: theme.midTextStyle.copyWith(fontWeight: FontWeight.bold),
                maxLines: 1,
                overflow: TextOverflow.ellipsis,
              ),
            ),
          ),
        ),

        // ── 标题和内容之间的细线 ──
        Container(height: 1, color: theme.colorEdge.withAlpha(120)),

        // ── 占位提示 ──
        // TODO(子导航): 具体内容还没定。想好了就去 pages_nav_table.dart
        //              给对应的段补 sidebar，这里就不会再画占位了
        Expanded(
          child: Center(
            child: Padding(
              padding: const EdgeInsets.symmetric(horizontal: 16),
              child: Text(
                '「$title」的子导航待设计',
                textAlign: TextAlign.center,
                style: theme.littleTextStyle.copyWith(color: theme.colorEdge),
              ),
            ),
          ),
        ),
      ],
    );
  }
}
