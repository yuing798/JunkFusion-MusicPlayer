import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/utils/layout_controller.dart';
import 'package:junk_fusion_ui/widgets/layout/icon_rail.dart';
import 'package:junk_fusion_ui/widgets/layout/sidebar_host.dart';
import 'package:junk_fusion_ui/widgets/layout/sidebar_resize_handle.dart';

// ════════════════════════════════════════════════════════════════
// three_column_layout.dart — 三段式骨架
//
//         ┌──────┬──────────────┬─┬───────────────────┐
//         │ 图标 │ 中间栏        │↔│ 主内容             │
//         │ 56px │ 180 ~ 480    │6│ 占满剩下的宽度      │
//         └──────┴──────────────┴─┴───────────────────┘
//
// 图标栏（IconRail）固定宽；中间栏（SidebarHost）宽度由 LayoutController
// 管，可以拖；分隔线（SidebarResizeHandle）只管把位移交出去；主内容原封不动
// 通过 child 传进来，三段式本身不关心里面是什么页面。
//
// 用法（在 app.dart 里，顶栏下面那块）：
//     ThreeColumnLayout(
//       child: Padding(
//         padding: EdgeInsets.symmetric(horizontal: 10),
//         child: PagesRouteView(),
//       ),
//     )
//
// 性能：拖拽时**不能**在这一层 setState。中间栏以后可能是几千个节点的
// 文件夹树，每帧重建整棵三个栏会掉帧。所以宽度变化只通过 AnimatedBuilder
// 重画外面那一层 SizedBox，SidebarHost 作为 child 传进去，不重建。
// ════════════════════════════════════════════════════════════════

/// 三段式：图标栏 | 中间栏 | 主内容
class ThreeColumnLayout extends StatefulWidget {
  /// 右侧主内容（页面栈 + 返回按钮那一坨）
  final Widget child;

  const ThreeColumnLayout({super.key, required this.child});

  @override
  State<ThreeColumnLayout> createState() => _ThreeColumnLayoutState();
}

class _ThreeColumnLayoutState extends State<ThreeColumnLayout> {
  /// 中间栏宽度状态（自己从 SharedPreferences 里读上次的值）
  late final LayoutController _layout = LayoutController();

  @override
  void dispose() {
    _layout.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return LayoutBuilder(
      builder: (context, constraints) {
        // 这一行拿到的总宽度（图标栏 + 中间栏 + 分隔线 + 主内容）
        final availableWidth = constraints.maxWidth;

        return Row(
          // stretch：三栏都撑满这一行的高度
          // （默认的 center 会让图标栏 / 中间栏缩成内容高度，底色只画一截）
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            // ── 第 1 段：图标栏（固定宽） ──
            const IconRail(),

            // ── 第 2 段：中间栏（可拖宽） ──
            AnimatedBuilder(
              animation: _layout,
              // child 传进来：拖拽时只重建 SizedBox，中间栏里面不重建
              child: const SidebarHost(),
              builder: (context, child) => SizedBox(
                width: _layout.effectiveWidth(availableWidth),
                child: child,
              ),
            ),

            // ── 第 3 段：分隔线 ──
            SidebarResizeHandle(
              onDrag: (delta) => _layout.resizeBy(delta, availableWidth),
              onDragEnd: _layout.persist,
            ),

            // ── 第 4 段：主内容（占满剩下的） ──
            Expanded(child: widget.child),
          ],
        );
      },
    );
  }
}
