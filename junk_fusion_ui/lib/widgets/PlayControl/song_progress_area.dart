/// SongProgressArea — 播放栏的进度条
///
/// ── 只做三件事 ────────────────────────────────────────────────────────
///   1. 显示当前进度 / 总时长（左右两侧的时间标签）
///   2. 点击或拖动进度条跳转
///   3. 跟随 PlaybackProvider 的 currentTimeStamp 刷新
///
/// ── 历史 ──────────────────────────────────────────────────────────────
/// 这里原来挂着一层"时域波形图"（导入时算好的 128 根柱子），鼠标悬浮时从
/// 进度条下方滑出来。这个功能已经整体删除，原因：
///   · 它是装饰性的，不提供任何操作价值；
///   · 它会盖住 1:00 / 4:05 的时间标签；
///   · 它属于主界面，但用户操作其他 UI 时会被它打断；
///   · 以后播放页要放实时频谱，再放个静态时域图会让人分不清。
/// 对应的 spectrum_painter.dart 也一起删了。
library;

import 'package:audio_video_progress_bar/audio_video_progress_bar.dart';
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:provider/provider.dart';

class SongProgressArea extends StatelessWidget {
  const SongProgressArea({super.key});

  @override
  Widget build(BuildContext context) {
    final playback = context.watch<PlaybackProvider>();
    final theme = context.watch<AppTheme>();

    final basicInfo = playback.curBasicInfo;
    if (basicInfo == null) return const SizedBox.shrink();

    return ProgressBar(
      progress: Duration(
        milliseconds: (playback.currentTimeStamp * 1000).toInt(),
      ),
      total: Duration(milliseconds: (basicInfo.duration * 1000).toInt()),
      onSeek: (value) {
        context.read<PlaybackProvider>().seekPreferPTS(
          value.inMilliseconds.toDouble() / 1000.0,
        );
      },

      // 时间标签：左边 1:00、右边 4:05（时域图删了之后这两位终于露出来了）
      timeLabelLocation: TimeLabelLocation.sides,
      timeLabelTextStyle: theme.littleTextStyle,

      barHeight: 5,
      thumbRadius: 6,
      baseBarColor: theme.colorEdge,
      bufferedBarColor: theme.colorHover,
      progressBarColor: theme.colorStress,
      thumbColor: theme.colorStress,
      thumbGlowColor: theme.colorStress.withValues(alpha: 0.3),
    );
  }
}
