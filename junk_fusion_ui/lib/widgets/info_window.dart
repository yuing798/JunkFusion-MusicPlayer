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
import '../theme/app_theme.dart';

// InfoWindow — 全局通知弹窗管理类
// Dart 中的抽象类，只有“抽象方法（没有函数体的方法）”才强制要求子类重写；
// 而“具体方法（有函数体的方法）”子类可以直接继承使用，也可以按需重写（可选）。

abstract class InfoWindow {
  // ════════════════════════════════════════════════════════════════
  // 静态状态（模块级单例）
  //
  // 对应原 Vue:
  //   const _message = ref('');
  //   const _visible = ref(false);
  //   let hideTimer: ReturnType<typeof setTimeout> | null = null;
  // ════════════════════════════════════════════════════════════════

  // 当前显示的弹窗条目
  static OverlayEntry? _overlayEntry;
  //允许你在 Flutter 应用层级的“最顶层”或“指定层级”插入一个独立的 Widget，从而覆盖在所有其他界面之上

  // 隐藏定时器
  //
  // `Timer?` 是可空的 Timer 引用
  static Timer? _hideTimer;

  // ════════════════════════════════════════════════════════════════
  // 公开方法
  // ════════════════════════════════════════════════════════════════

  // 初始化 InfoWindow（需要在 MaterialApp 中设置 navigatorKey）
  //
  // 如果使用 Overlay 模式，需要持有一个 GlobalKey<NavigatorState>
  // 来获取 Overlay 的 context
  static final GlobalKey<NavigatorState> navigatorKey =
      GlobalKey<NavigatorState>();

  // 显示通知弹窗
  //
  // 对应原 Vue: showInfoWindow(msg, holdTime)
  //
  // Dart 语法：
  // - `Object? msg` 可接受任意类型（Error, String, 等）
  // - `int holdTime = 3000` 是默认参数值，调用时可以不传
  // - `static void show(...)` 静态方法，通过类名调用
  //[]:可选位置参数；{}:可选命名参数
  static void show(Object? msg, [int holdTime = 3000]) {
    // 先关闭已有的弹窗
    _dismissOverlay();
    _hideTimer?.cancel();
    assert(msg != null);

    final message = msg.toString();

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
    //通过 context 向上找到屏幕根部的 Overlay（悬浮层画布），然后把你的 _overlayEntry 挂载到画布上，让它立刻显示在屏幕最前面。
    //Overlay 是 MaterialApp（或 WidgetsApp）在启动时自动创建的顶层组件，你完全不需要、也不应该自己去手动创建它。
    Overlay.of(context).insert(_overlayEntry!);

    // 设置自动消失定时器
    // `Timer` 是 Dart 的定时器类
    // `Duration(milliseconds: 300 + holdTime)` 等待淡入动画(300ms) + 保持时长
    _hideTimer = Timer(Duration(milliseconds: 300 + holdTime), () {
      _dismissOverlay();
    });
  }

  // 立即关闭弹窗
  static void close() {
    _hideTimer?.cancel();
    _dismissOverlay();
  }

  // 移除 Overlay 条目
  static void _dismissOverlay() {
    _overlayEntry?.remove();
    _overlayEntry = null;
  }
}

// _InfoWindowWidget — 通知弹窗的实际 UI widget
//StatelessWidget 是“静态不可变”的，一旦构建完毕就不再主动变化；
//而 StatefulWidget 是“动态可变”的，拥有独立的 State 对象，可以主动触发界面刷新。
class _InfoWindowWidget extends StatefulWidget {
  final String message;
  final VoidCallback onClose; // VoidCallback = void Function() 的类型别名

  const _InfoWindowWidget({required this.message, required this.onClose});

  @override
  State<_InfoWindowWidget> createState() => _InfoWindowWidgetState();
}

//State<T> 是 StatefulWidget 的“灵魂”和“数据保险箱”。
class _InfoWindowWidgetState extends State<_InfoWindowWidget>
    with SingleTickerProviderStateMixin {
  late AnimationController _controller; //控制动画的启停、速度、方向
  //late是Dart 中一个用于“延迟初始化”和“非空承诺”的关键字。 它告诉编译器：“这个变量现在我不初始化，但我承诺在第一次使用它之前，一定会给它赋值。”
  late Animation<double> _opacityAnimation; //只负责显示当前跑到了什么数值

  @override
  void initState() {
    super.initState();

    // AnimationController — 动画控制器
    // `duration` 动画持续时间（等于 CSS 的 transition-duration）
    // `vsync: this` 提供帧同步信号（来自 SingleTickerProviderStateMixin）
    _controller = AnimationController(
      duration: Duration(milliseconds: 300), // 150ms
      vsync: this,
      //vsync:同步屏幕刷新率（防止视觉撕裂）2. 自动暂停后台动画（节省 CPU / 电量）
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

// ════════════════════════════════════════════════════════════════
// 关键概念：
//
// Overlay 系统：
// - Overlay 是 Flutter 的"浮层"系统，类似于 CSS 的 z-index 机制
// - Navigator 和 MaterialApp 内部都有一个 Overlay
// - OverlayEntry 是一个浮层条目，insert() 插入后显示，remove() 移除
// - 这等价于 Vue 的 Teleport to body
//
// Timer：
// - Timer(Duration, callback) 等价于 JS 的 setTimeout(callback, ms)
// - Timer.periodic(Duration, callback) 等价于 JS 的 setInterval
// - timer.cancel() 等价于 clearTimeout / clearInterval
//
// Mixins：
// - `with` 关键字混入 mixin 的功能
// - `SingleTickerProviderStateMixin` 提供动画帧同步
// - mixin 类似 C++ 的 CRTP 或 Scala 的 trait
// ════════════════════════════════════════════════════════════════
