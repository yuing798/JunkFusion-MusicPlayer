/// 全屏播放页的开关状态
///
/// ── 为什么单独一个 Provider ───────────────────────────────────────────
/// 播放页是浮在整个窗口上的（见 app.dart 的 Stack），打开它的入口在
/// 底部播放栏上，关掉它的按钮在页面自己身上，两边不在一棵子树里，
/// 所以用一个很薄的 ChangeNotifier 传状态。
///
/// ── 用法 ─────────────────────────────────────────────────────────────
///     context.read<PlayPageProvider>().open();   // 播放栏点封面
///     context.read<PlayPageProvider>().close();  // 页面上的关闭按钮
///     context.watch<PlayPageProvider>().isOpen;  // 页面据此滑入/滑出
library;

import 'package:flutter/foundation.dart';

class PlayPageProvider extends ChangeNotifier {
  bool _isOpen = false;

  bool get isOpen => _isOpen;

  void open() {
    if (_isOpen) return;
    _isOpen = true;
    notifyListeners();
  }

  void close() {
    if (!_isOpen) return;
    _isOpen = false;
    notifyListeners();
  }

  void toggle() => _isOpen ? close() : open();
}
