import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/utils/global_key_defs.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';
import 'package:toastification/toastification.dart';

// ════════════════════════════════════════════════════════════════
// error_toast.dart — 非致命错误的「右下角卡片通知」
//
// ── 它是干什么的 ──────────────────────────────────────────────
// C++ 那边 registerErrorSendCallback 推过来的错误（现在只有 FFmpeg 解码失败，
// 以后导入失败之类也会走这里），以及前端自己发现的非致命错误，都从这里弹一条
// 卡片：几秒后自己消失、点一下就消失、鼠标压上去暂停计时。
//
// 真正的致命错误（库打不开、音频进程没了）不走这里，走
// UtilFunction.showFatalMessage() 的中央模态窗 —— 那条路必须手动关掉。
//
// ── 怎么调用 ──────────────────────────────────────────────────
//     ErrorToast.show('解码失败: xxx');       // 直接调
//     UtilFunction.showErrorMessage('xxx');  // 推荐（统一入口）
//
//     ToastificationWrapper(config: ErrorToast.config, ...)  // main.dart 里挂配置
//
// ── 为什么是右下角、为什么挂在播放栏上面 ──────────────────────
// 顶栏 50px 是窗口按钮 + 拖窗口区，内容区顶部右侧是各页工具栏（文件夹页的
// 「导入文件夹 / 单独导入歌曲」就在那儿），所以顶部放红条要么盖住按钮要么把
// 内容整体下推；右下角是桌面应用的通知区惯例，而地板被 PlayBar 占了
// （放歌时浮上来 100px，见 PlayBar），所以通知挂在它上面 = bottom 留 112。
//
// 想换位置/大小/条数/时长，只改下面这一组常量 + _marginBuilder 就行。
//
// ── 堆积与去重 ────────────────────────────────────────────────
// 1. 最多同时 maxVisible 条，第 4 条挤掉最老的（toastification 的 maxToastLimit）；
// 2. 同一段文案 mergeWindow 内再来一遍，不新建卡片，而是把标题改成「错误 ×N」
//    并重新计时 —— 一张坏专辑连切歌时，屏幕上不会出现三张一模一样的卡片。
// ════════════════════════════════════════════════════════════════

/// 非致命错误的右下角卡片通知
class ErrorToast {
  // ════════════════════════════════════════════════════════════
  // 位置 / 尺寸 / 时长（想调外观只改这里）
  // ════════════════════════════════════════════════════════════

  /// 通知栈贴在哪一角
  static const Alignment alignment = Alignment.bottomRight;

  /// 单张卡片的宽度
  static const double itemWidth = 340;

  /// 同时最多显示几张（超了挤掉最老的）
  static const int maxVisible = 3;

  /// 一张卡片停留多久（鼠标悬停时暂停计时）
  static const Duration showDuration = Duration(seconds: 6);

  /// 底部留白：PlayBar 放歌时浮上来占 100px，通知挂在它上面
  static const double bottomInset = 112;

  /// 右侧留白
  static const double sideInset = 12;

  /// 同一段文案的合并窗口：这个时间内重复出现 -> 计数 +1、重新计时
  static const Duration mergeWindow = Duration(seconds: 10);

  // ── 配色（和 AppTheme.colorError 同一套色系，浅色主题下的红卡片） ──
  static const Color _backgroundColor = Color(0xFFFDECEA);
  static const Color _foregroundColor = Color(0xFF7F1D1D);
  static const Color _accentColor = Color(0xFF991F1F);

  /// 正文最多几行（FFmpeg 的错误里带完整路径，超了就省略号）
  static const int _descriptionMaxLines = 4;

  // ════════════════════════════════════════════════════════════
  // 给 main.dart 用的全局配置
  // ════════════════════════════════════════════════════════════

  /// 挂在 MaterialApp 外面的 ToastificationWrapper 上
  ///
  /// 必须复用同一个实例：toastification 内部按 alignment 缓存 manager，
  /// 每次重新 new 一个 config 是不会生效的。
  static const ToastificationConfig config = ToastificationConfig(
    alignment: alignment,
    itemWidth: itemWidth,
    maxToastLimit: maxVisible,
    marginBuilder: _marginBuilder,
  );

  /// 通知栈整体离窗口边缘的距离
  static EdgeInsetsGeometry _marginBuilder(
    BuildContext context,
    AlignmentGeometry alignment,
  ) {
    return const EdgeInsets.only(right: sideInset, bottom: bottomInset);
  }

  // ════════════════════════════════════════════════════════════
  // 内部状态
  // ════════════════════════════════════════════════════════════

  /// 正在屏幕上显示着的卡片：key = 文案
  static final Map<String, ToastificationItem> _shown = {};

  /// 最近出现过的文案（去重计数用）
  static final Map<String, _RecentError> _recent = {};

  // ════════════════════════════════════════════════════════════
  // 对外的接口
  // ════════════════════════════════════════════════════════════

  /// 弹一条错误卡片
  ///
  /// 空文案直接忽略；界面还没挂起来的时候只打控制台（正常不会发生：
  /// 回调在 initState 里注册，那时首帧还没画完而已，等得到）。
  static void show(String message) {
    final text = message.trim();
    if (text.isEmpty) return;

    final now = DateTime.now();
    // 过期的记录清掉，别让这两张表一直长
    _recent.removeWhere((_, each) => now.difference(each.lastAt) > mergeWindow);

    final count = (_recent[text]?.count ?? 0) + 1;
    _recent[text] = _RecentError(count: count, lastAt: now);

    // 同一段文案还挂在屏幕上：先撤掉旧卡片，换成带计数的（顺便把倒计时拉满）
    final oldItem = _shown.remove(text);
    if (oldItem != null) {
      toastification.dismiss(oldItem, showRemoveAnimation: false);
    }

    final overlayState = navigatorKey.currentState?.overlay;
    if (overlayState == null) {
      print('[ErrorToast] 界面还没准备好，这条错误只留在控制台：$text');
      return;
    }

    late final ToastificationItem item;
    item = toastification.show(
      overlayState: overlayState,
      type: ToastificationType.error,
      style: ToastificationStyle.flat,
      autoCloseDuration: showDuration,
      showProgressBar: true,
      pauseOnHover: true,
      closeOnClick: true,
      onHoverMouseCursor: SystemMouseCursors.click,
      // 卡片之间留 8px；默认的左右 12 会让宽度缩水，这里只留底部的
      margin: const EdgeInsets.only(bottom: 8),
      backgroundColor: _backgroundColor,
      foregroundColor: _foregroundColor,
      primaryColor: _accentColor,
      borderSide: BorderSide(
        color: _accentColor.withValues(alpha: 0.25),
        width: 1.5,
      ),
      icon: const Icon(
        TablerIcons.alertTriangleFilled,
        size: 22,
        color: _accentColor,
      ),
      title: Text(count > 1 ? '错误 ×$count' : '错误'),
      description: Text(
        text,
        maxLines: _descriptionMaxLines,
        overflow: TextOverflow.ellipsis,
      ),
      callbacks: ToastificationCallbacks(
        onAutoCompleteCompleted: (_) => _forget(text, item),
        onDismissed: (_) => _forget(text, item),
        onCloseButtonTap: (each) {
          _forget(text, item);
          // 覆盖了这个回调，关闭的默认行为要自己写
          toastification.dismiss(each);
        },
      ),
    );
    _shown[text] = item;
  }

  /// 清掉这张卡片的记录
  ///
  /// 只有表里还是同一个 item 才删 —— 旧卡片的定时器回调可能比新卡片晚到，
  /// 不加这个判断会把同名的新卡片一起忘掉（下次重复出现就不计数了）
  static void _forget(String text, ToastificationItem item) {
    if (identical(_shown[text], item)) {
      _shown.remove(text);
    }
  }
}

/// 一段相同文案的出现记录（去重用）
class _RecentError {
  const _RecentError({required this.count, required this.lastAt});

  /// mergeWindow 内出现了几次
  final int count;

  /// 最后一次出现的时间
  final DateTime lastAt;
}
