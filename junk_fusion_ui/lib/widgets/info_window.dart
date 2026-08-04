/// ════════════════════════════════════════════════════════════════
/// info_window.dart — 全局通知弹窗（单例模式）
///
/// 对应原 Vue 项目 components/other/infoWindow.vue
///
/// 设计模式：模块级单例
/// 原 Vue 中，_message 和 _visible 是模块级 ref，所有导入者共享同一实例。
/// Flutter 中，我们用静态字段 + Overlay 实现同样的效果。
///
/// Dart 语法说明：
/// - `static` 成员属于类本身而非实例，全局只有一份
/// - `Overlay` 是 Flutter 的浮层系统，可以在所有内容上方显示 widget
/// - `OverlayEntry` 是 Overlay 中的一个条目
/// - `Timer` 是 Dart 的定时器（dart:async）
/// - `NavigatorKey` 是全局的导航键，用于在没有 context 的地方访问 Navigator
///
/// 使用方式：
/// ```dart
/// import 'widgets/info_window.dart';
///
/// // 在 try/catch 中
/// InfoWindow.show('操作成功');
/// InfoWindow.show('错误信息', holdTime: 5000);
/// InfoWindow.show(Exception('失败'));
/// InfoWindow.close(); // 手动关闭
/// ```
/// ════════════════════════════════════════════════════════════════

import 'dart:async'; // Timer 类在此库中
import 'package:flutter/material.dart';
import '../theme/app_theme.dart';

/// InfoWindow — 全局通知弹窗管理类
///
/// 这是纯静态类（所有成员都是 static），不需要实例化
///
/// Dart 语法：
/// - `abstract class` 抽象类不能被 new
/// - 所有方法都是 `static`，通过 `InfoWindow.show(...)` 调用
abstract class InfoWindow {
  // ════════════════════════════════════════════════════════════════
  // 静态状态（模块级单例）
  //
  // 对应原 Vue:
  //   const _message = ref('');
  //   const _visible = ref(false);
  //   let hideTimer: ReturnType<typeof setTimeout> | null = null;
  // ════════════════════════════════════════════════════════════════

  /// 当前显示的弹窗条目
  static OverlayEntry? _overlayEntry;

  /// 隐藏定时器
  ///
  /// `Timer?` 是可空的 Timer 引用
  static Timer? _hideTimer;

  // ════════════════════════════════════════════════════════════════
  // 公开方法
  // ════════════════════════════════════════════════════════════════

  /// 初始化 InfoWindow（需要在 MaterialApp 中设置 navigatorKey）
  ///
  /// 如果使用 Overlay 模式，需要持有一个 GlobalKey<NavigatorState>
  /// 来获取 Overlay 的 context
  static final GlobalKey<NavigatorState> navigatorKey =
      GlobalKey<NavigatorState>();

  /// 显示通知弹窗
  ///
  /// 对应原 Vue: showInfoWindow(msg, holdTime)
  ///
  /// Dart 语法：
  /// - `Object? msg` 可接受任意类型（Error, String, 等）
  /// - `int holdTime = 3000` 是默认参数值，调用时可以不传
  /// - `static void show(...)` 静态方法，通过类名调用
  static void show(Object? msg, {int holdTime = 3000}) {
    // 先关闭已有的弹窗
    _dismissOverlay();
    _hideTimer?.cancel();

    // 提取消息文本
    final message = _extractMessage(msg);

    // 获取 Overlay 的 context
    final context = navigatorKey.currentContext;
    if (context == null) return; // 如果没有 context，无法显示

    // 创建 OverlayEntry
    // OverlayEntry 是 Overlay 系统中的一个"浮层条目"
    _overlayEntry = OverlayEntry(
      builder: (context) => _InfoWindowWidget(
        message: message,
        onClose: () {
          _dismissOverlay();
          _hideTimer?.cancel();
        },
      ),
    );

    // 插入 Overlay
    // Overlay.of(context) 获取最近的 Overlay 实例
    Overlay.of(context).insert(_overlayEntry!);

    // 设置自动消失定时器
    // `Timer` 是 Dart 的定时器类
    // `Duration(milliseconds: 300 + holdTime)` 等待淡入动画(300ms) + 保持时长
    _hideTimer = Timer(Duration(milliseconds: 300 + holdTime), () {
      _dismissOverlay();
    });
  }

  /// 立即关闭弹窗
  ///
  /// 对应原 Vue: closeInfoWindow()
  static void close() {
    _hideTimer?.cancel();
    _dismissOverlay();
  }

  // ════════════════════════════════════════════════════════════════
  // 私有辅助方法
  // ════════════════════════════════════════════════════════════════

  /// 移除 Overlay 条目
  static void _dismissOverlay() {
    _overlayEntry?.remove();
    _overlayEntry = null;
  }

  /// 从任意类型的消息中提取显示字符串
  ///
  /// 对应原 Vue: extractInfo(info)
  ///
  /// Dart 语法：
  /// - `is` 是类型检查运算符，等价于 JS 的 instanceof
  /// - `toString()` 是 Object 的方法，所有类都继承它
  static String _extractMessage(Object? msg) {
    if (msg == null) return '';
    if (msg is Error) return msg.toString();
    if (msg is String) return msg;
    return msg.toString();
  }
}

/// _InfoWindowWidget — 通知弹窗的实际 UI widget
///
/// 对应原 Vue 的 template:
///   <div class="info-popup" :class="{ visible }">
///
/// 使用 AnimatedOpacity 实现淡入淡出效果
/// 对应原 CSS transition: opacity var(--ease-time) ease
class _InfoWindowWidget extends StatefulWidget {
  final String message;
  final VoidCallback onClose; // VoidCallback = void Function() 的类型别名

  const _InfoWindowWidget({required this.message, required this.onClose});

  @override
  State<_InfoWindowWidget> createState() => _InfoWindowWidgetState();
}

class _InfoWindowWidgetState extends State<_InfoWindowWidget>
    with SingleTickerProviderStateMixin {
  // `with` 关键字引入 mixin（混入）
  // `SingleTickerProviderStateMixin` 为 AnimationController 提供 ticker

  /// 动画控制器
  ///
  /// Dart 语法：
  /// - `late` 延迟初始化（在 initState 中赋值）
  /// - `AnimationController` 管理动画的时间线
  late AnimationController _controller;
  late Animation<double> _opacityAnimation;

  @override
  void initState() {
    super.initState();

    // AnimationController — 动画控制器
    // `duration` 动画持续时间（等于 CSS 的 transition-duration）
    // `vsync: this` 提供帧同步信号（来自 SingleTickerProviderStateMixin）
    _controller = AnimationController(
      duration: AppTheme.easeTime, // 150ms
      vsync: this,
    );

    // `Tween` 定义动画值的范围（从 0 到 1）
    // `animate` 把 Tween 绑定到 AnimationController
    _opacityAnimation = Tween<double>(
      begin: 0.0,
      end: 1.0,
    ).animate(CurvedAnimation(parent: _controller, curve: Curves.easeOut));

    // 启动淡入动画
    _controller.forward();
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    // `AnimatedBuilder` 或直接用 `FadeTransition`
    // 这里用 FadeTransition 实现 CSS opacity 动画
    return FadeTransition(
      opacity: _opacityAnimation,
      child: Material(
        // `Material` 用于 Overlay 中的 widget 提供 Material 设计基础
        type: MaterialType.transparency,
        child: Center(
          child: Container(
            width: 360,
            // `BoxDecoration` 定义背景、阴影、圆角
            decoration: BoxDecoration(
              // 背景色（对应 CSS background-color: color-mix(...)）
              color: AppTheme.colorHover.withAlpha(179), // ~70% 透明度
              borderRadius: BorderRadius.circular(AppTheme.borderRadius),
              boxShadow: const [
                BoxShadow(
                  color: Color(0x40000000), // 黑色 25% 透明度
                  offset: Offset(0, 4),
                  blurRadius: 16,
                ),
              ],
            ),
            child: Stack(
              // `Stack` 允许子 widget 层叠（类似 CSS position: absolute）
              children: [
                // --- 消息文本 ---
                Padding(
                  padding: const EdgeInsets.all(24),
                  child: Text(
                    widget.message,
                    style: AppTheme.midTextStyle.copyWith(
                      color: AppTheme.colorError,
                    ),
                    textAlign: TextAlign.center,
                  ),
                ),

                // --- 关闭按钮（右上角 X） ---
                Positioned(
                  top: 6,
                  right: 6,
                  child: IconButton(
                    icon: const Icon(Icons.close, size: 18),
                    onPressed: widget.onClose,
                    padding: EdgeInsets.zero,
                    constraints: const BoxConstraints(
                      minWidth: 24,
                      minHeight: 24,
                    ),
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

/// ════════════════════════════════════════════════════════════════
/// 关键概念：
///
/// Overlay 系统：
/// - Overlay 是 Flutter 的"浮层"系统，类似于 CSS 的 z-index 机制
/// - Navigator 和 MaterialApp 内部都有一个 Overlay
/// - OverlayEntry 是一个浮层条目，insert() 插入后显示，remove() 移除
/// - 这等价于 Vue 的 Teleport to body
///
/// Timer：
/// - Timer(Duration, callback) 等价于 JS 的 setTimeout(callback, ms)
/// - Timer.periodic(Duration, callback) 等价于 JS 的 setInterval
/// - timer.cancel() 等价于 clearTimeout / clearInterval
///
/// Mixins：
/// - `with` 关键字混入 mixin 的功能
/// - `SingleTickerProviderStateMixin` 提供动画帧同步
/// - mixin 类似 C++ 的 CRTP 或 Scala 的 trait
/// ════════════════════════════════════════════════════════════════
