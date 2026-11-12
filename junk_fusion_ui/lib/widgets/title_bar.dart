import 'package:flutter/material.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';
import 'package:window_manager/window_manager.dart';

// ════════════════════════════════════════════════════════════════
// title_bar.dart — 顶栏（通栏）
//
// 三段式改造后这条顶栏横跨整个窗口，压在图标栏 / 中间栏 / 内容区上面：
//   左半边：DragToMoveArea（拖动窗口）
//   右半边：最小化 / 最大化(还原) / 关闭
//
// TODO: 标题栏以后还要扩展（Logo、搜索框之类的位置到时候再定），
//       原来的 Logo 已经从左侧栏里拿掉了，先不在这里补。
// ════════════════════════════════════════════════════════════════

class TitleBar extends StatefulWidget {
  const TitleBar({super.key});

  @override
  State<TitleBar> createState() => _TitleBarState();
}

class _TitleBarState extends State<TitleBar> with WindowListener {
  bool _isMaximized = false;

  @override
  void initState() {
    super.initState();
    windowManager.addListener(this); // 监听窗口状态变化
    // 播放页的顶栏可能在窗口已最大化时才创建，先读取当前状态。
    windowManager.isMaximized().then((isMaximized) {
      if (mounted) setState(() => _isMaximized = isMaximized);
    });
  }

  @override
  void dispose() {
    windowManager.removeListener(this);
    super.dispose();
  }

  // 监听窗口最大化状态，用于切换“还原”和“最大化”图标
  @override
  void onWindowMaximize() => setState(() => _isMaximized = true);

  @override
  void onWindowUnmaximize() => setState(() => _isMaximized = false);

  @override
  Widget build(BuildContext context) {
    return Container(
      height: 50,
      color: Colors.transparent,
      child: Row(
        mainAxisAlignment: MainAxisAlignment.end, //优先靠右对齐
        crossAxisAlignment: CrossAxisAlignment.center,
        children: [
          // ── 中间空白区域：支持拖拽 ──
          // DragToMoveArea 是 拖动窗口的组件
          Expanded(
            child: DragToMoveArea(
              child: Container(
                color: Colors.transparent, // 必须有颜色才能响应手势
              ),
            ),
          ),

          // ──右侧控制按钮区 ──
          IconButton(
            icon: Icon(TablerIcons.minus, size: 32),
            style: IconButton.styleFrom(
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(0), // 0 → 矩形，可设其他值获得圆角矩形
              ),
            ),

            onPressed: () => windowManager.minimize(), // 最小化
            tooltip: "最小化",
          ),
          IconButton(
            icon: Icon(
              _isMaximized
                  ? TablerIcons.windowMinimize
                  : TablerIcons.windowMaximize,
              size: 32,
            ),
            style: IconButton.styleFrom(
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(0), // 0 → 矩形，可设其他值获得圆角矩形
              ),
            ),

            tooltip: _isMaximized ? "向下还原" : "最大化",
            onPressed: () async {
              // 切换最大化/向下还原
              if (await windowManager.isMaximized()) {
                windowManager.unmaximize();
              } else {
                windowManager.maximize();
              }
            },
          ),
          IconButton(
            icon: Icon(TablerIcons.x, size: 32),
            hoverColor: Theme.of(context).colorScheme.error, // 鼠标悬浮关闭按钮时变红
            onPressed: () {
              // print("用户点击了右上角的关闭按钮");
              windowManager.close();
            }, // 关闭窗口
            style: IconButton.styleFrom(
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(0), // 0 → 矩形，可设其他值获得圆角矩形
              ),
            ),
          ),
        ],
      ),
    );
  }
}
