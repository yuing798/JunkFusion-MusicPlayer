//播放列表
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/utils/global_key_defs.dart';

class PlayList {
  late final AnimationController _controller;
  late final Animation<Offset> _slideAnimation;
  OverlayEntry? _overlayEntry; // 用来持有覆盖层

  Playlist(TickerProvider vsync) {
    _controller = AnimationController(
      vsync: vsync,
      duration: const Duration(milliseconds: 350),
    );
    _slideAnimation = Tween<Offset>(
      begin: const Offset(1, 0),
      end: const Offset(0, 0),
    ).animate(CurvedAnimation(parent: _controller, curve: Curves.easeInOut));
  }

  void dispose() {
    _removeOverlay();
    _controller.dispose();
  }

  void _removeOverlay() {
    _overlayEntry?.remove();
    _overlayEntry = null;
  }

  void _showOverlay() {
    _removeOverlay(); //先移除旧的
    final overlay = navigatorKey.currentState!.overlay;
    _overlayEntry = OverlayEntry(
      builder: (context) {
        return Stack(
          children: [
            GestureDetector(
              onTap: () {
                _controller.reverse().then((_) => _removeOverlay());
                //_controller.reverse()：触发一个 Flutter 动画控制器反向执行（比如让播放栏滑出屏幕）。
                //它返回一个 Future<void>，这个 Future 会在动画彻底结束（到达 0.0）时完成（resolve）。
                //.then(...)：当 Future 完成时，自动调用括号里的函数。这是 Dart 中处理异步回调的经典写法，等价于 await 的效果。
                //(_) => _removeOverlay()：这是一个箭头函数。_ 是一个占位符参数，
                //代表 .then 传过来的返回值（因为 reverse 返回的是 void，所以用 _ 忽略它）。
                //执行体是调用 _removeOverlay()（即你自定义的移除 OverlayEntry 的方法）。
              },
              child: Align(
                alignment: Alignment.centerRight,
                child: SlideTransition(
                  position: _slideAnimation,
                  child: Container(width: 500, height: 800),
                ),
              ),
            ),
          ],
        );
      },
    );
  }
}
