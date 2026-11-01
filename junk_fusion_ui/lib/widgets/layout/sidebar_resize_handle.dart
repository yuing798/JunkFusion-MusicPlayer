import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:provider/provider.dart';

// ════════════════════════════════════════════════════════════════
// sidebar_resize_handle.dart — 中间栏和主内容之间那条可以拖的间隔线
//
// 视觉上是一条 1px 的竖线，但命中区有 6px（1px 的线根本点不中），
// 鼠标移上去变成 resize 光标、线变粗变强调色，按住左右拖改中间栏宽度。
//
// 自己不做宽度计算，只把横向位移交给外面：
//     SidebarResizeHandle(
//       onDrag: (dx) => layout.resizeBy(dx, 可用总宽度),
//       onDragEnd: layout.persist,      // 拖完才写 SharedPreferences
//     )
//
// 拖拽中不落盘：SharedPreferences 是磁盘写，每帧写一次会卡。
// ════════════════════════════════════════════════════════════════

/// 可拖拽的分隔线
class SidebarResizeHandle extends StatefulWidget {
  /// 横向拖拽增量（像素），正数 = 往右拖
  final ValueChanged<double> onDrag;

  /// 拖拽结束（松手 / 被取消）时调一次，用来落盘
  final VoidCallback onDragEnd;

  const SidebarResizeHandle({
    super.key,
    required this.onDrag,
    required this.onDragEnd,
  });

  @override
  State<SidebarResizeHandle> createState() => _SidebarResizeHandleState();
}

class _SidebarResizeHandleState extends State<SidebarResizeHandle> {
  /// 鼠标是否悬浮在分隔线上
  bool _isHovered = false;

  /// 是否正在拖拽（拖的时候鼠标早就离开这 6px 了，靠它保持高亮）
  bool _isDragging = false;

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final isActive = _isHovered || _isDragging;

    return MouseRegion(
      cursor: SystemMouseCursors.resizeColumn,
      onEnter: (_) => setState(() => _isHovered = true),
      onExit: (_) => setState(() => _isHovered = false),
      child: GestureDetector(
        // opaque：6px 的空白命中区在 Row 里也要能接到鼠标
        behavior: HitTestBehavior.opaque,
        onHorizontalDragStart: (_) => setState(() => _isDragging = true),
        onHorizontalDragUpdate: (details) => widget.onDrag(details.delta.dx),
        onHorizontalDragEnd: (_) {
          setState(() => _isDragging = false);
          widget.onDragEnd();
        },
        onHorizontalDragCancel: () {
          setState(() => _isDragging = false);
          widget.onDragEnd();
        },
        child: SizedBox(
          width: 6,
          child: Center(
            // 只改线的粗细，不改 6px 命中区 —— 否则鼠标一靠近布局就抖
            child: AnimatedContainer(
              duration: const Duration(milliseconds: 120),
              width: isActive ? 2 : 1,
              color: isActive ? theme.colorStress : theme.colorEdge,
            ),
          ),
        ),
      ),
    );
  }
}
