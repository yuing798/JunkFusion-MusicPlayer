import 'dart:math';

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/Macro/sliderParam.dart';
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

// 1. 这里混入 SingleTickerProviderStateMixin
class VolumeControllerButtonState extends State<VolumeControllerButton>
    with SingleTickerProviderStateMixin {
  OverlayEntry? _overlayEntry;
  bool _isHoveringButton = false;
  bool _isHoveringSlider = false;
  Timer? _closeTimer;
  double lastVolume = 0.0;

  // 2. 将动画控制器移到这里统一管理
  late final AnimationController _fadeController;
  late final Animation<double> _fadeAnimation;

  @override
  void initState() {
    super.initState();

    // 初始化动画控制器
    _fadeController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 300),
    );
    _fadeAnimation = CurvedAnimation(
      parent: _fadeController,
      curve: Curves.easeInOut,
    );

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
    _overlayEntry?.remove();
    _overlayEntry = null;
    _fadeController.dispose(); // 销毁控制器
    super.dispose();
  }

  // ---------- Overlay 管理 ----------
  void _showSlider() {
    // 3. 如果已经存在 Overlay，只播放进入动画，不重复创建
    if (_overlayEntry != null) {
      _fadeController.forward();
      return;
    }

    final RenderBox? renderBox = context.findRenderObject() as RenderBox?;
    if (renderBox == null) return;

    final Offset position = renderBox.localToGlobal(Offset.zero);
    final Size size = renderBox.size;

    const double sliderWidth = 40.0;
    const double sliderHeight = 130.0;
    double left = position.dx + size.width / 2 - sliderWidth / 2;
    double top = position.dy - sliderHeight - 10;

    final overlay = navigatorKey.currentState!.overlay;
    _overlayEntry = OverlayEntry(
      builder: (context) => Positioned(
        left: left,
        top: top,
        // 4. 将 FadeTransition 移到这里，由父组件控制透明度
        child: FadeTransition(
          opacity: _fadeAnimation,
          child: _VolumeSlider(onEnter: _onSliderEnter, onExit: _onSliderExit),
        ),
      ),
    );

    overlay!.insert(_overlayEntry!);
    _fadeController.forward(); // 启动淡入动画
  }

  // 5. 将 remove 修改为异步，先退场后销毁
  void _removeOverlay() async {
    if (_overlayEntry == null) return;

    // 播放退出动画并等待结束
    await _fadeController.reverse();

    // 关键判断：动画结束时，如果状态是 completely dismissed (代表中途没有再次触发 forward)，再移除 overlay
    if (_fadeController.isDismissed && _overlayEntry != null) {
      _overlayEntry?.remove();
      _overlayEntry = null;
    }
  }

  // ---------- 悬停回调 ----------
  void _onButtonEnter() {
    _isHoveringButton = true;
    _closeTimer?.cancel();
    _closeTimer = null;
    _showSlider(); // 直接调用即可，_showSlider 里写了拦截判断
  }

  void _onButtonExit() {
    _isHoveringButton = false;
    _startCloseTimer();
  }

  void _onSliderEnter() {
    _isHoveringSlider = true;
    _closeTimer?.cancel();
    _closeTimer = null;
    // 如果鼠标移回滑块时，可能正在播放退出动画，立即重新正向播放
    _fadeController.forward();
  }

  void _onSliderExit() {
    _isHoveringSlider = false;
    _startCloseTimer();
  }

  void _startCloseTimer() {
    _closeTimer?.cancel();
    _closeTimer = Timer(const Duration(milliseconds: 200), () {
      if (!_isHoveringButton && !_isHoveringSlider) {
        _removeOverlay(); // 调用带动画的移除方法
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
            UtilFunction.sendSliderMsg(SliderParam.masterVolume, 0.0, false);
          } else {
            context.read<PlaybackProvider>().setVolume(lastVolume);
            UtilFunction.sendSliderMsg(
              SliderParam.masterVolume,
              lastVolume,
              false,
            );
          }
        },
        icon: Icon(iconData, size: 24, color: theme.colorTextSecond),
      ),
    );
  }
}

// ---------- 滑块组件（大幅精简） ----------
// 由于动画被父组件接管，这里可以直接改为 StatelessWidget
class _VolumeSlider extends StatelessWidget {
  final VoidCallback onEnter;
  final VoidCallback onExit;

  const _VolumeSlider({required this.onEnter, required this.onExit});

  @override
  Widget build(BuildContext context) {
    final currentVolume = context.select<PlaybackProvider, double>(
      (p) => p.volume,
    );

    return MouseRegion(
      onEnter: (_) => onEnter(),
      onExit: (_) => onExit(),
      child: Material(
        // 移除了内部的 FadeTransition
        elevation: 8.0,
        borderRadius: BorderRadius.circular(8),
        child: SizedBox(
          width: 40,
          height: 130,
          child: Column(
            verticalDirection: VerticalDirection.up,
            children: [
              Text("${(currentVolume * 100).toStringAsFixed(0)}%"),
              Expanded(
                child: RotatedBox(
                  quarterTurns: 3,
                  child: Slider(
                    padding: const EdgeInsets.symmetric(horizontal: 15),
                    value: currentVolume,
                    onChanged: (value) {
                      context.read<PlaybackProvider>().setVolume(value);
                      UtilFunction.sendSliderMsg(
                        SliderParam.masterVolume,
                        value,
                        true,
                      );
                    },
                    onChangeEnd: (value) {
                      context.read<PlaybackProvider>().setVolume(value);
                      UtilFunction.sendSliderMsg(
                        SliderParam.masterVolume,
                        value,
                        false,
                      );
                    },
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
