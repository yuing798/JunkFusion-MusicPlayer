import 'dart:io';

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/bridge/dll/dll_invoke.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/widgets/helper_widget.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';
import 'package:window_manager/window_manager.dart';

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
          RectIconButton(
            iconData: TablerIcons.minus,
            onPressed: () => windowManager.minimize(), // 最小化
            tooltip: "最小化",
          ),
          RectIconButton(
            iconData: _isMaximized
                ? TablerIcons.windowMinimize
                : TablerIcons.windowMaximize,

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
          RectIconButton(
            iconData: TablerIcons.x,
            hoverColor: Colors.red, // 鼠标悬浮关闭按钮时变红
            onPressed: () {
              // print("用户点击了右上角的关闭按钮");
              windowManager.close();
            }, // 关闭窗口
          ),
        ],
      ),
    );
  }
}
