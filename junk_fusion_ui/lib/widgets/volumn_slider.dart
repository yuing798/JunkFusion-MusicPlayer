import 'dart:math';

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/bridge/dllAndFlutterBridgeDefs.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/global_key_defs.dart';
import 'package:junk_fusion_ui/utils/utils.dart';
import 'package:provider/provider.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';
import 'dart:async';

class VolumeControllerButton extends StatefulWidget {
  const VolumeControllerButton({super.key});
  @override
  State<StatefulWidget> createState() {
    return VolumeControllerButtonState();
  }
}

class VolumeControllerButtonState extends State<VolumeControllerButton> {
  OverlayEntry? _overlayEntry;
  bool _isHoveringButton = false;
  bool _isHoveringSlider = false;
  Timer? _closeTimer;
  double lastVolume = 0.0;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      double tempVolume = context.read<PlaybackProvider>().volume;
      if (tempVolume != 0.0) {
        lastVolume = tempVolume;
      }
    });
  }

  @override
  void dispose() {
    _closeTimer?.cancel();
    _removeOverlay();
    super.dispose();
  }

  // ---------- Overlay 管理 ----------
  void _showSlider() {
    _removeOverlay();
    final RenderBox? renderBox = context.findRenderObject() as RenderBox?;
    //RenderBox 是渲染树（Render Tree）上负责“真实物理尺寸和位置”的底层对象
    //获取当前音量按钮在屏幕上的“真实坐标和尺寸”，以便把 Overlay 浮层精准地定位在它上方。
    if (renderBox == null) return;

    final Offset position = renderBox.localToGlobal(Offset.zero); //按钮的左上角
    //将“本地坐标”转换为“全局屏幕坐标”
    final Size size = renderBox.size;

    // 计算滑块位置：按钮上方居中（间距 10px）
    const double sliderWidth = 40.0;
    const double sliderHeight = 130.0;
    double left = position.dx + size.width / 2 - sliderWidth / 2;
    double top = position.dy - sliderHeight - 10;

    final overlay = navigatorKey.currentState!.overlay;
    _overlayEntry = OverlayEntry(
      builder: (context) => Positioned(
        left: left,
        top: top,
        child: _VolumeSlider(onEnter: _onSliderEnter, onExit: _onSliderExit),
      ),
    );
    //这个组件只有在鼠标悬浮到按钮上才会执行构造函数，所以直接在initstate中放forward是对的
    overlay!.insert(_overlayEntry!);
  }

  void _removeOverlay() {
    _overlayEntry?.remove();
    _overlayEntry = null;
  }

  // ---------- 悬停回调 ----------
  void _onButtonEnter() {
    _isHoveringButton = true;
    _closeTimer?.cancel();
    _closeTimer = null;
    if (_overlayEntry == null) {
      _showSlider();
    }
  }

  void _onButtonExit() {
    _isHoveringButton = false;
    _startCloseTimer();
  }

  void _onSliderEnter() {
    _isHoveringSlider = true;
    _closeTimer?.cancel();
    _closeTimer = null;
  }

  void _onSliderExit() {
    _isHoveringSlider = false;
    _startCloseTimer();
  }

  void _startCloseTimer() {
    _closeTimer?.cancel();
    _closeTimer = Timer(const Duration(milliseconds: 200), () {
      if (!_isHoveringButton && !_isHoveringSlider) {
        _removeOverlay();
      }
      _closeTimer = null;
    });
  }

  // ---------- 构建 UI ----------
  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final currentVolume = context.select<PlaybackProvider, double>(
      (p) => p.volume,
    );
    IconData iconData;
    if (currentVolume == 0.0) {
      iconData = TablerIcons.volume3;
    } else if (currentVolume <= 0.5) {
      iconData = TablerIcons.volume2;
    } else {
      iconData = TablerIcons.volume;
    }

    return MouseRegion(
      onEnter: (_) => _onButtonEnter(),
      onExit: (_) => _onButtonExit(),
      child: IconButton(
        onPressed: () {
          if (currentVolume != 0.0) {
            context.read<PlaybackProvider>().setVolume(0.0);
            lastVolume = currentVolume;
          } else {
            context.read<PlaybackProvider>().setVolume(lastVolume);
          }
        },
        icon: Icon(iconData, size: 24, color: theme.colorTextSecond),
      ),
    );
  }
}

// ---------- 滑块组件（带悬停检测） ----------
class _VolumeSlider extends StatefulWidget {
  final VoidCallback onEnter;
  final VoidCallback onExit;
  const _VolumeSlider({required this.onEnter, required this.onExit});

  @override
  State<_VolumeSlider> createState() => _VloumeSliderState();
}

class _VloumeSliderState extends State<_VolumeSlider>
    with SingleTickerProviderStateMixin {
  late final AnimationController _controller;
  late final Animation<double> _opaqueAnimation;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 300),
    );
    _opaqueAnimation = Tween<double>(
      begin: 0.0,
      end: 1.0,
    ).animate(CurvedAnimation(parent: _controller, curve: Curves.easeInOut));
    _controller.forward(); // 启动淡入动画
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final currentVolume = context.select<PlaybackProvider, double>(
      (p) => p.volume,
    );

    return MouseRegion(
      onEnter: (_) => widget.onEnter(),
      onExit: (_) => widget.onExit(),
      child: FadeTransition(
        opacity: _opaqueAnimation,
        child: Material(
          elevation: 8.0,
          borderRadius: BorderRadius.circular(8),
          child: Column(
            children: [
              RotatedBox(
                quarterTurns: 3,
                child: SizedBox(
                  width: 120,
                  height: 40,
                  child: Slider(
                    padding: EdgeInsets.symmetric(horizontal: 10),
                    value: currentVolume,
                    onChanged: (value) {
                      context.read<PlaybackProvider>().setVolume(value);
                      UtilFunction.sendSliderMsg(
                        B_sliderParam.masterVolume,
                        value,
                        true,
                      );
                    },
                    onChangeEnd: (value) {
                      context.read<PlaybackProvider>().setVolume(value);
                      UtilFunction.sendSliderMsg(
                        B_sliderParam.masterVolume,
                        value,
                        false,
                      );
                    },
                  ),
                ),
              ),
              Text("${(currentVolume * 100).toStringAsFixed(0)}%"),
            ],
          ),
        ),
      ),
    );
  }
}
