/// 右下角卡片通知：Info / Warn / Error 三种
///
/// ── 它是什么 ──────────────────────────────────────────────────────────
/// 非致命信息统一走这里（C++ 推上来的状态、前端自己的操作结果）。
/// 真正的致命错误不走这里，走 `UtilFunction.showFatalMessage()` 的中央模态窗
/// —— 那条路必须用户手动关掉。
///
/// │ 类型            │ 用在哪                                             │
/// │-----------------│----------------------------------------------------│
/// │ InfoToast       │ 操作成功、状态变化（"已应用效果器链"）             │
/// │ WarnToast       │ 部分成功、可疑但能继续（"有 2 个插件在黑名单里"）  │
/// │ ErrorToast      │ 明确的失败（解码失败、插件加载失败）               │
///
/// 三种**共用一份 ToastificationConfig**：toastification 内部按 alignment 缓存
/// manager，alignment 相同就等于同一个栈，所以三类通知会自然排在同一个角落，
/// 不会出现"info 在左下、error 在右下"这种分裂。
///
/// ── 怎么调用 ──────────────────────────────────────────────────────────
/// ```dart
/// InfoToast.show('已应用效果器链');
/// WarnToast.show('3 个插件被黑名单挡住');
/// ErrorToast.show('解码失败: xxx');
///
/// // 推荐走统一入口（方便以后换实现）
/// UtilFunction.showInfoMessage('已应用');
/// UtilFunction.showWarnMessage('...');
/// UtilFunction.showErrorMessage('...');
/// ```
/// main.dart 里挂一次 `ToastificationWrapper(config: AppToast.config, ...)`。
///
/// ── 堆积与去重 ─────────────────────────────────────────────────────────
/// 1. 最多同时 maxVisible 条，第 4 条挤掉最老的；
/// 2. 同一段文案 mergeWindow 内再来一遍，不新建卡片，而是把标题改成「错误 ×N」
///    并重新计时 —— 一张坏专辑连切歌时，屏幕上不会出现三张一模一样的卡片；
/// 3. 去重按 (类型 + 文案)：同样的文字但一个是 warn 一个是 error，会各弹一条。
library;

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/utils/global_key_defs.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';
import 'package:toastification/toastification.dart';

/// 通知类型（只影响配色、图标、标题）
enum ToastKind {
  /// 操作成功 / 状态变化
  info,

  /// 部分成功 / 可疑但能继续
  warn,

  /// 明确的失败
  error,
}

/// 三种通知的公共实现
class AppToast {
  // ════════════════════════════════════════════════════════════
  // 位置 / 尺寸 / 时长（想调外观只改这里，三种通知一起变）
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

  /// 同一段文案的合并窗口
  static const Duration mergeWindow = Duration(seconds: 10);

  /// 正文最多几行（FFmpeg 的错误里带完整路径，超了就省略号）
  static const int _descriptionMaxLines = 4;

  // ════════════════════════════════════════════════════════════
  // 给 main.dart 用的全局配置（**三种通知共用这一份**）
  // ════════════════════════════════════════════════════════════
  static const ToastificationConfig config = ToastificationConfig(
    alignment: alignment,
    itemWidth: itemWidth,
    maxToastLimit: maxVisible,
    marginBuilder: _marginBuilder,
  );

  static EdgeInsetsGeometry _marginBuilder(
    BuildContext context,
    AlignmentGeometry alignment,
  ) {
    return const EdgeInsets.only(right: sideInset, bottom: bottomInset);
  }

  // ════════════════════════════════════════════════════════════
  // 内部状态：key = "类型|文案"
  // ════════════════════════════════════════════════════════════
  static final Map<String, ToastificationItem> _shown = {};
  static final Map<String, _RecentToast> _recent = {};

  // ════════════════════════════════════════════════════════════
  // 对外
  // ════════════════════════════════════════════════════════════

  /// 弹一条卡片通知；空文案直接忽略
  static void show(ToastKind kind, String message) {
    final text = message.trim();
    if (text.isEmpty) return;

    final style = _Style.of(kind);
    final key = '${kind.name}|$text';

    final now = DateTime.now();
    _recent.removeWhere((_, each) => now.difference(each.lastAt) > mergeWindow);

    final count = (_recent[key]?.count ?? 0) + 1;
    _recent[key] = _RecentToast(count: count, lastAt: now);

    // 同一段文案还挂在屏幕上：先撤掉旧卡片，换成带计数的
    final oldItem = _shown.remove(key);
    if (oldItem != null) {
      toastification.dismiss(oldItem, showRemoveAnimation: false);
    }

    final overlayState = navigatorKey.currentState?.overlay;
    if (overlayState == null) {
      print('[${style.label}] 界面还没准备好，这条通知只在控制台：$text');
      return;
    }

    late final ToastificationItem item;
    item = toastification.show(
      overlayState: overlayState,
      type: style.type,
      style: ToastificationStyle.flat,
      autoCloseDuration: showDuration,
      showProgressBar: true,
      pauseOnHover: true,
      closeOnClick: true,
      onHoverMouseCursor: SystemMouseCursors.click,
      margin: const EdgeInsets.only(bottom: 8),
      backgroundColor: style.background,
      foregroundColor: style.foreground,
      primaryColor: style.accent,
      borderSide: BorderSide(
        color: style.accent.withValues(alpha: 0.25),
        width: 1.5,
      ),
      icon: Icon(style.icon, size: 22, color: style.accent),
      title: Text(count > 1 ? '${style.label} ×$count' : style.label),
      description: Text(
        text,
        maxLines: _descriptionMaxLines,
        overflow: TextOverflow.ellipsis,
      ),
      callbacks: ToastificationCallbacks(
        onAutoCompleteCompleted: (_) => _forget(key, item),
        onDismissed: (_) => _forget(key, item),
        onCloseButtonTap: (each) {
          _forget(key, item);
          toastification.dismiss(each);
        },
      ),
    );
    _shown[key] = item;
  }

  /// 只有表里还是同一个 item 才删 —— 旧卡片的定时器可能比新卡片晚到
  static void _forget(String key, ToastificationItem item) {
    if (identical(_shown[key], item)) _shown.remove(key);
  }
}

/// 信息：操作成功 / 状态变化
class InfoToast {
  static void show(String message) => AppToast.show(ToastKind.info, message);
}

/// 警告：部分成功 / 可疑但能继续
class WarnToast {
  static void show(String message) => AppToast.show(ToastKind.warn, message);
}

/// 错误：明确的失败
class ErrorToast {
  static void show(String message) => AppToast.show(ToastKind.error, message);
}

/// 一段相同文案的出现记录（去重用）
class _RecentToast {
  const _RecentToast({required this.count, required this.lastAt});
  final int count;
  final DateTime lastAt;
}

/// 每种通知的配色 / 图标 / 标题
class _Style {
  const _Style({
    required this.label,
    required this.type,
    required this.icon,
    required this.background,
    required this.foreground,
    required this.accent,
  });

  final String label;
  final ToastificationType type;
  final IconData icon;
  final Color background;
  final Color foreground;
  final Color accent;

  static _Style of(ToastKind kind) {
    switch (kind) {
      case ToastKind.info:
        return const _Style(
          label: '提示',
          type: ToastificationType.info,
          icon: TablerIcons.infoCircleFilled,
          background: Color(0xFFEAF2FD),
          foreground: Color(0xFF12365E),
          accent: Color(0xFF1D6FD1),
        );
      case ToastKind.warn:
        return const _Style(
          label: '注意',
          type: ToastificationType.warning,
          icon: TablerIcons.alertCircleFilled,
          background: Color(0xFFFFF6E5),
          foreground: Color(0xFF6B4A05),
          accent: Color(0xFFB7791F),
        );
      case ToastKind.error:
        return const _Style(
          label: '错误',
          type: ToastificationType.error,
          icon: TablerIcons.alertTriangleFilled,
          background: Color(0xFFFDECEA),
          foreground: Color(0xFF7F1D1D),
          accent: Color(0xFF991F1F),
        );
    }
  }
}
