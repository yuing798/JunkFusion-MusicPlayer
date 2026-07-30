/// ════════════════════════════════════════════════════════════════
/// popup_window.dart — 通用弹窗外壳
///
/// 对应原 Vue 项目 components/other/popupWindow.vue
///
/// 功能：
/// - 触发按钮被点击时，弹窗从触发按钮中心缩放出现
/// - 半透明遮罩层，点击遮罩关闭弹窗
/// - 标题栏（居中标题 + X 关闭按钮）
/// - 内容区域由调用方提供（通过 builder 参数）
///
/// Dart 语法说明：
/// - `GlobalKey` 是 Flutter 中获取 Widget 引用的一种方式
///   用于获取触发按钮的位置和尺寸
/// - `showDialog` 是 Flutter 内置的弹窗方法
/// - `showGeneralDialog` 提供更底层的弹窗控制（支持自定义动画）
/// - 这里使用 `Overlay` 实现，以匹配原 Vue 的 Teleport + CSS 动画行为
/// ════════════════════════════════════════════════════════════════

import 'package:flutter/material.dart';
import '../theme/app_theme.dart';

/// PopupWindow — 通用弹窗组件
///
/// 使用方式（对应原 Vue 的具名插槽模式）：
/// ```dart
/// PopupWindow(
///   title: '歌曲详情',
///   triggerBuilder: (open) => IconButton(
///     icon: Icon(Icons.info),
///     onPressed: open,
///   ),
///   contentBuilder: () => YourContentWidget(),
/// )
/// ```
///
/// Dart 语法说明：
/// - `triggerBuilder` 参数类型是 `Widget Function(VoidCallback open)`
///   它是一个函数，接收 open 回调，返回一个 Widget
/// - `contentBuilder` 参数类型是 `Widget Function()`
///   它是一个函数，无参数，返回弹窗内容的 Widget
class PopupWindow extends StatefulWidget {
  /// 弹窗标题（显示在标题栏居中位置）
  final String title;

  /// 触发按钮构建器
  /// 接收 `open` 回调函数，调用方把它绑定到按钮的 onPressed
  final Widget Function(VoidCallback open) triggerBuilder;

  /// 弹窗内容构建器
  final Widget Function() contentBuilder;

  const PopupWindow({
    super.key,
    required this.title,
    required this.triggerBuilder,
    required this.contentBuilder,
  });

  @override
  State<PopupWindow> createState() => _PopupWindowState();
}

class _PopupWindowState extends State<PopupWindow> {
  /// 触发按钮的 GlobalKey，用于获取其在屏幕上的位置
  ///
  /// GlobalKey 是 Flutter 中唯一可以跨 Widget 树层级获取 Widget
  /// 位置和尺寸的方式
  final GlobalKey _triggerKey = GlobalKey();

  /// 弹窗是否可见
  bool _visible = false;

  /// 弹窗是否正在关闭（用于动画）
  bool _closing = false;

  /// 打开弹窗
  ///
  /// 对应原 Vue: function open()
  void _open() {
    setState(() {
      _visible = true;
      _closing = false;
    });
  }

  /// 关闭弹窗（带动画）
  ///
  /// 对应原 Vue: function close()
  void _close() {
    setState(() {
      _closing = true;
    });

    // 等待动画结束后彻底移除
    Future.delayed(const Duration(milliseconds: 200), () {
      if (mounted) {
        // `mounted` 检查 State 是否还在 Widget Tree 中
        setState(() {
          _visible = false;
          _closing = false;
        });
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        // --- 触发按钮 ---
        // `key: _triggerKey` 把 GlobalKey 绑定到触发按钮上
        GestureDetector(
          key: _triggerKey,
          onTap: _open,
          child: widget.triggerBuilder(_open),
        ),

        // --- 弹窗（条件渲染） ---
        if (_visible) _buildPopup(),
      ],
    );
  }

  /// 构建弹窗（遮罩 + 内容窗口）
  Widget _buildPopup() {
    return Stack(
      children: [
        // --- 半透明遮罩 ---
        // 对应原 Vue: <div class="popup-backdrop">
        GestureDetector(
          onTap: _close, // 点击遮罩关闭弹窗
          child: AnimatedOpacity(
            opacity: _closing ? 0.0 : 1.0,
            duration: const Duration(milliseconds: 200),
            child: Container(
              color: const Color(0x59000000), // 黑色 35% 透明度
            ),
          ),
        ),

        // --- 弹窗窗口 ---
        // 使用 AnimatedBuilder 或直接用 AnimatedScale
        Center(
          child: AnimatedScale(
            scale: _closing ? 0.0 : 1.0,
            duration: const Duration(milliseconds: 200),
            curve: _closing ? Curves.easeIn : Curves.easeOut,
            child: AnimatedOpacity(
              opacity: _closing ? 0.0 : 1.0,
              duration: const Duration(milliseconds: 200),
              child: Material(
                type: MaterialType.transparency,
                child: Container(
                  // 宽高由内容决定（不设固定值）
                  constraints: const BoxConstraints(maxWidth: 960),
                  decoration: BoxDecoration(
                    color: AppTheme.colorHover,
                    borderRadius:
                        BorderRadius.circular(AppTheme.borderRadius),
                    boxShadow: const [
                      BoxShadow(
                        color: Color(0x40000000),
                        offset: Offset(0, 4),
                        blurRadius: 24,
                      ),
                    ],
                  ),
                  child: Column(
                    mainAxisSize: MainAxisSize.min, // 高度跟随内容
                    children: [
                      // --- 标题栏（32px 高）---
                      _buildTitleBar(),

                      // --- 内容区域 ---
                      // 对应原 Vue: <div class="popup-body">
                      //              <slot name="popup-window-component" />
                      Flexible(
                        child: widget.contentBuilder(),
                      ),
                    ],
                  ),
                ),
              ),
            ),
          ),
        ),
      ],
    );
  }

  /// 构建标题栏：居中标题 + 右侧 X 关闭按钮
  ///
  /// 对应原 Vue: <div class="popup-titlebar">
  Widget _buildTitleBar() {
    return SizedBox(
      height: 32,
      child: Stack(
        children: [
          // 居中标题
          Center(
            child: Text(
              widget.title,
              style: AppTheme.midTextStyle.copyWith(
                fontWeight: FontWeight.bold,
              ),
              overflow: TextOverflow.ellipsis,
            ),
          ),

          // 右侧关闭按钮
          Positioned(
            right: 4,
            top: 0,
            bottom: 0,
            child: Center(
              child: IconButton(
                icon: const Icon(Icons.close, size: 18),
                onPressed: _close,
                padding: EdgeInsets.zero,
                constraints: const BoxConstraints(
                  minWidth: 24,
                  minHeight: 24,
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }
}

/// ════════════════════════════════════════════════════════════════
/// 对比 Vue 具名插槽 vs Flutter builder 模式：
///
/// Vue:
///   <PopupWindow title="歌曲详情">
///     <template #trigger-button>
///       <button>打开</button>
///     </template>
///     <template #popup-window-component>
///       <YourContent />
///     </template>
///   </PopupWindow>
///
/// Flutter:
///   PopupWindow(
///     title: '歌曲详情',
///     triggerBuilder: (open) => ElevatedButton(
///       onPressed: open,
///       child: Text('打开'),
///     ),
///     contentBuilder: () => YourContent(),
///   )
///
/// Flutter 没有"插槽"的概念，用 builder 回调函数实现同样的效果。
/// builder 模式是 Flutter 中最常见的组件组合方式。
/// ════════════════════════════════════════════════════════════════
