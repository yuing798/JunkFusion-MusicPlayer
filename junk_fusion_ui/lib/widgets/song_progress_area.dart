import 'package:audio_video_progress_bar/audio_video_progress_bar.dart';
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/widgets/spectrum_painter.dart';
import 'package:provider/provider.dart';

class SongProgressArea extends StatefulWidget {
  final void Function(double) onHeightChange;

  const SongProgressArea({super.key, required this.onHeightChange});
  @override
  State<StatefulWidget> createState() {
    return SongProgressAreaState();
  }
}

class SongProgressAreaState extends State<SongProgressArea>
    with SingleTickerProviderStateMixin {
  bool _isProgressBarHovered = false;
  late final AnimationController _controller;
  late final Animation<Offset> _slideAnimation;

  // 用于处理波形图拖拽的临时视觉状态
  bool _isWaveformDragging = false;
  double _waveformDragTime = 0.0;

  // 【新增】：使用 ValueNotifier 高效追踪鼠标 X 坐标，避免全局频繁 setState
  final ValueNotifier<double?> _hoverXNotifier = ValueNotifier(null);
  //包裹一个值，当这个值发生变化时，能主动通知依赖它的组件进行更新。

  //TODO: 进度标签还没写，我现在还没有思路

  @override
  void initState() {
    super.initState();

    _controller = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 350),
    );

    _slideAnimation = Tween<Offset>(
      begin: const Offset(0, 1),
      end: const Offset(0, 0),
    ).animate(CurvedAnimation(parent: _controller, curve: Curves.easeInOut));

    _controller.addStatusListener((status) {
      if (status == AnimationStatus.dismissed) {
        widget.onHeightChange(20);
        setState(() {});
      }
    });
  }

  @override
  void dispose() {
    _controller.dispose();
    _hoverXNotifier.dispose();
    super.dispose();
  }

  double _calculateTime(double localDx, double maxWidth, double duration) {
    if (maxWidth <= 0) return 0.0;
    double percentage = (localDx / maxWidth).clamp(0.0, 1.0);
    return percentage * duration;
  }

  @override
  Widget build(BuildContext context) {
    final playback = context.watch<PlaybackProvider>();
    final theme = context.watch<AppTheme>();

    return MouseRegion(
      // 【新增】：监听鼠标在整个区域内的滑动，实时更新 X 坐标
      onHover: (event) {
        _hoverXNotifier.value = event.localPosition.dx;
      },
      onEnter: (event) => setState(() {
        _isProgressBarHovered = true;
        widget.onHeightChange(20 + 80);
        _controller.forward();
      }),
      onExit: (event) {
        setState(() {
          _isProgressBarHovered = false;
        });
        _hoverXNotifier.value = null; // 【新增】：鼠标移出时隐藏线
        _controller.reverse();
      },
      cursor: SystemMouseCursors.click,
      child: Column(
        verticalDirection: VerticalDirection.up,
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          ProgressBar(
            progress: Duration(
              milliseconds: _isWaveformDragging
                  ? (_waveformDragTime * 1000).toInt()
                  : (playback.currentTimeStamp * 1000).toInt(),
            ),
            total: Duration(
              milliseconds: (playback.currentSong!.duration * 1000).toInt(),
            ),
            onSeek: (value) {
              context.read<PlaybackProvider>().seekPreferPTS(
                (value.inMilliseconds).toDouble() / 1000.0,
              );
            },
            timeLabelLocation: TimeLabelLocation.none,
            barHeight: _isProgressBarHovered ? 9.0 : 5.0,
          ),

          if (!_controller.isDismissed)
            ClipRect(
              child: SlideTransition(
                position: _slideAnimation,
                child: LayoutBuilder(
                  builder: (context, constraints) {
                    return GestureDetector(
                      behavior: HitTestBehavior.opaque,
                      onTapUp: (details) {
                        double targetTime = _calculateTime(
                          details.localPosition.dx,
                          constraints.maxWidth,
                          playback.currentSong!.duration,
                        );
                        playback.seekPreferPTS(targetTime);
                      },
                      onHorizontalDragStart: (details) {
                        // 【新增】：拖拽开始时同步线的位置
                        _hoverXNotifier.value = details.localPosition.dx;
                        setState(() {
                          _isWaveformDragging = true;
                          _waveformDragTime = _calculateTime(
                            details.localPosition.dx,
                            constraints.maxWidth,
                            playback.currentSong!.duration,
                          );
                        });
                      },
                      onHorizontalDragUpdate: (details) {
                        // 【新增】：拖拽过程中同步线的位置
                        _hoverXNotifier.value = details.localPosition.dx;
                        setState(() {
                          _waveformDragTime = _calculateTime(
                            details.localPosition.dx,
                            constraints.maxWidth,
                            playback.currentSong!.duration,
                          );
                        });
                      },
                      onHorizontalDragEnd: (details) {
                        playback.seekPreferPTS(_waveformDragTime);
                        setState(() {
                          _isWaveformDragging = false;
                        });
                      },
                      onHorizontalDragCancel: () {
                        setState(() {
                          _isWaveformDragging = false;
                        });
                      },
                      // 【核心改动】：使用 Stack 将线盖在波形图上方
                      child: SizedBox(
                        height: 80,
                        child: Stack(
                          clipBehavior: Clip.none,
                          children: [
                            // 1. 底部的波形图
                            Positioned.fill(
                              child: CustomPaint(
                                painter: SpectrumPainter(
                                  data: playback.timeDomainSpec,
                                  waveColor: theme.colorShadow,
                                ),
                              ),
                            ),

                            // 2. 悬浮/拖拽时的垂直指示线（使用 ValueListenableBuilder 局部刷新优化性能）
                            ValueListenableBuilder<double?>(
                              valueListenable: _hoverXNotifier,
                              builder: (context, hoverX, child) {
                                // 如果没有悬浮，或者鼠标超出了边界，则不显示线
                                if (hoverX == null ||
                                    hoverX < 0 ||
                                    hoverX > constraints.maxWidth) {
                                  return const SizedBox.shrink();
                                }
                                return Positioned(
                                  left: hoverX,
                                  top: 0,
                                  bottom: 0,
                                  child: Container(
                                    width: 3, // 线的粗细
                                    color: Colors.black87, // 线的颜色
                                  ),
                                );
                              },
                            ),
                          ],
                        ),
                      ),
                    );
                  },
                ),
              ),
            ),
        ],
      ),
    );
  }
}
