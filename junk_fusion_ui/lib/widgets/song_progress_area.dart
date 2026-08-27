import 'package:audio_video_progress_bar/audio_video_progress_bar.dart';
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
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

  // 【新增状态】：用于处理波形图拖拽的临时视觉状态
  bool _isWaveformDragging = false;
  double _waveformDragTime = 0.0;

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
    super.dispose();
  }

  // 【修改方法】：只负责计算时间，不再直接触发 seek
  double _calculateTime(double localDx, double maxWidth, double duration) {
    if (maxWidth <= 0) return 0.0;
    // 计算百分比并限制在 0.0 ~ 1.0 之间
    double percentage = (localDx / maxWidth).clamp(0.0, 1.0);
    return percentage * duration;
  }

  @override
  Widget build(BuildContext context) {
    final playback = context.read<PlaybackProvider>();

    return MouseRegion(
      onEnter: (event) => setState(() {
        _isProgressBarHovered = true;
        widget.onHeightChange(20 + 120);
        _controller.forward();
      }),
      onExit: (event) => setState(() {
        _isProgressBarHovered = false;
        _controller.reverse();
      }),
      cursor: SystemMouseCursors.click,
      child: Column(
        verticalDirection: VerticalDirection.up,
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          ProgressBar(
            // 【核心改动 1】：拖拽波形时，显示临时时间；平时显示歌曲真实播放时间
            progress: Duration(
              milliseconds: _isWaveformDragging
                  ? (_waveformDragTime * 1000).toInt()
                  : (playback.currentTimeStamp * 1000).toInt(),
            ),
            total: Duration(
              milliseconds: (playback.songDuration * 1000).toInt(),
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
                      // 【核心改动 2】：单纯的单击，抬起鼠标时直接触发跳转
                      onTapUp: (details) {
                        double targetTime = _calculateTime(
                          details.localPosition.dx,
                          constraints.maxWidth,
                          playback.songDuration,
                        );
                        playback.seekPreferPTS(targetTime);
                      },
                      // 【核心改动 3】：开始拖拽，记录初始时间和拖拽状态
                      onHorizontalDragStart: (details) {
                        setState(() {
                          _isWaveformDragging = true;
                          _waveformDragTime = _calculateTime(
                            details.localPosition.dx,
                            constraints.maxWidth,
                            playback.songDuration,
                          );
                        });
                      },
                      // 【核心改动 4】：拖拽中，只更新UI临时时间，不调取后台 seek
                      onHorizontalDragUpdate: (details) {
                        setState(() {
                          _waveformDragTime = _calculateTime(
                            details.localPosition.dx,
                            constraints.maxWidth,
                            playback.songDuration,
                          );
                        });
                      },
                      // 【核心改动 5】：拖拽结束松开鼠标，触发后台真正的 seek！
                      onHorizontalDragEnd: (details) {
                        playback.seekPreferPTS(_waveformDragTime);
                        setState(() {
                          _isWaveformDragging = false;
                        });
                      },
                      // 意外中断拖拽（例如鼠标切屏等），恢复正常状态
                      onHorizontalDragCancel: () {
                        setState(() {
                          _isWaveformDragging = false;
                        });
                      },
                      child: SizedBox(
                        height: 120,
                        child: CustomPaint(
                          painter: SpectrumPainter(playback.timeDomainSpec),
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
