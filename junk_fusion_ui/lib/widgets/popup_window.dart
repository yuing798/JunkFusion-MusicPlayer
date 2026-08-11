// ════════════════════════════════════════════════════════════════
// info_window.dart — 全局通知弹窗（单例模式）
// 使用方式：
// ```dart
// import 'widgets/info_window.dart';
//
// // 在 try/catch 中
// InfoWindow.show('操作成功');
// InfoWindow.show('错误信息', holdTime: 5000);
// InfoWindow.show(Exception('失败'));
// InfoWindow.close(); // 手动关闭
// ```
// ════════════════════════════════════════════════════════════════

import 'dart:async'; // Timer 类在此库中
import 'package:flutter/material.dart';
import 'package:flutter_improved_scrolling/flutter_improved_scrolling.dart';
import 'package:junk_fusion_ui/utils/global_key_defs.dart';
import 'package:junk_fusion_ui/widgets/helper_widget.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';
import '../theme/app_theme.dart';

class DialogUtil {
  // 使用原生的 showDialog 弹出自定义内容
  static Future<void> showInfoDialog(String message) async {
    // 关键：利用 navigatorKey 获取 Context，无需手动传 context
    return showDialog(
      context: navigatorKey.currentContext!,
      barrierDismissible: true, // 点击灰色蒙版是否自动关闭（带默认退场动画）
      builder: (BuildContext context) {
        //当前弹窗组件自带的上下文
        // 👇 这里直接返回你的弹窗内容 Widget
        return _InfoWindowWidget(
          message: message,
          // 关闭时调用 Navigator.pop，系统会自动执行淡出+缩放动画
          onClose: () => Navigator.of(context).pop(),
        );
      },
    );
  }
}

class _InfoWindowWidget extends StatefulWidget {
  final String message;
  final VoidCallback onClose; // VoidCallback = void Function() 的类型别名

  const _InfoWindowWidget({
    // super.key,
    required this.message,
    required this.onClose,
  });

  @override
  State<StatefulWidget> createState() {
    return _InfoWindowWidgetState();
  }
}

// _InfoWindowWidget — 通知弹窗的实际 UI widget
class _InfoWindowWidgetState extends State<_InfoWindowWidget> {
  final _scrollController = ScrollController();

  @override
  void dispose() {
    _scrollController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Material(
        type: MaterialType.transparency,
        child: Container(
          width: 600,
          // 高度约束：最小 400，最大 800
          constraints: const BoxConstraints(minHeight: 400, maxHeight: 800),
          decoration: BoxDecoration(
            color: AppTheme.colorHover,
            borderRadius: BorderRadius.circular(AppTheme.borderRadius),
            boxShadow: const [
              BoxShadow(
                color: Colors.black26,
                offset: Offset(0, 4),
                blurRadius: 16,
              ),
            ],
          ),
          child: Padding(
            padding: const EdgeInsets.symmetric(vertical: 5, horizontal: 10),
            child: Stack(
              children: [
                // 1. 可滚动的内容区域（填充整个Stack）
                Positioned.fill(
                  child: Padding(
                    // 右边距留出空间给关闭按钮（按钮宽约24px，再加些间距）
                    padding: const EdgeInsets.only(
                      top: 10,
                      bottom: 10,
                      right: 40,
                      left: 0,
                    ),
                    child: ImprovedScrolling(
                      // 2. 使用 ImprovedScrollView 增强桌面滚动体验
                      scrollController: _scrollController,
                      enableMMBScrolling: true, // 开启鼠标中键（按下滚轮）拖拽滚动
                      enableCustomMouseWheelScrolling: true, // 开启自定义鼠标滚轮平滑滚动
                      enableKeyboardScrolling: true, //开启键盘键位滚动
                      child: Text(
                        widget.message,
                        style: AppTheme.midTextStyle,
                        textAlign: TextAlign.center,
                      ),
                    ),
                  ),
                ),
                // 3. 关闭按钮（固定在右上角，不随内容滚动）
                Positioned(
                  top: 6,
                  right: 6,
                  child: RectIconButton(
                    iconData: TablerIcons.x,
                    onPressed: widget.onClose,
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
