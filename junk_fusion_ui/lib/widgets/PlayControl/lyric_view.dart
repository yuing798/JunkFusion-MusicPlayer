/// 歌词滚动视图
///
/// ── 渲染逻辑 ──────────────────────────────────────────────────────────
///   · 每行固定高度 [lineExtent]，保证滚动位置可精确计算；
///   · 当前行高亮（主题强调色 + 加粗），前后行淡化；
///   · 有翻译时每行下面多一行小字；
///   · 点击任意一行 → 跳到那一行的时间（回调出去，页面负责 seek）。
///
/// ── 为什么自己写而不引 flutter_lyric ──────────────────────────────────
/// 我们的歌词是后端解析好的结构化数据（含逐字时间），只需要"滚动 + 高亮"，
/// 引第三方包反而要绕着它的样式体系改。真要做逐字扫光时再在 [currentLine]
/// 上叠一层 ClipRect 就行，接口已经留好了。
///
/// ── 用法 ─────────────────────────────────────────────────────────────
///     LyricView(
///       lyrics: context.watch<LyricProvider>().lyrics,
///       positionSeconds: playback.currentTimeStamp,
///       onSeekToLine: (index) => playback.seekPreferPTS(...),
///     )
library;

import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:flutter_smooth_wheel_scroll/flutter_smooth_wheel_scroll.dart';
import 'package:junk_fusion_ui/model/lyric_info.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:provider/provider.dart';

class LyricView extends StatefulWidget {
  const LyricView({
    super.key,
    required this.lyrics,
    required this.positionSeconds,
    this.onSeekToLine,
  });

  final LyricDocument? lyrics;
  final double positionSeconds;

  /// 点某一行：参数是行号（调用方自己去 seek 到那一行的时间）
  final ValueChanged<int>? onSeekToLine;

  @override
  State<LyricView> createState() => _LyricViewState();
}

class _LyricViewState extends State<LyricView> {
  final ScrollController _scrollController = SmoothScrollController();

  /// 每行占的高度（有翻译时也是一行原文 + 一行翻译，塞进同一个高度里）
  static const double _lineExtent = 72;

  int _currentIndex = -1;

  @override
  void dispose() {
    _scrollController.dispose();
    super.dispose();
  }

  @override
  void didUpdateWidget(covariant LyricView oldWidget) {
    super.didUpdateWidget(oldWidget);

    final index = _indexForTime(widget.positionSeconds);
    if (index != _currentIndex) {
      _currentIndex = index;
      _scrollToCurrent();
    }
  }

  /// 当前时间落在第几行（二分找最后一个 startMs <= time 的行）
  int _indexForTime(double seconds) {
    final lines = widget.lyrics?.lines;
    if (lines == null || lines.isEmpty) return -1;

    final timeMs = (seconds * 1000).round();
    int low = 0;
    int high = lines.length - 1;
    int result = -1;

    while (low <= high) {
      final mid = (low + high) ~/ 2;
      if (lines[mid].startMs <= timeMs) {
        result = mid;
        low = mid + 1;
      } else {
        high = mid - 1;
      }
    }
    return result;
  }

  void _scrollToCurrent() {
    if (!_scrollController.hasClients || _currentIndex < 0) return;

    final viewport = _scrollController.position.viewportDimension;
    final target = _currentIndex * _lineExtent - viewport / 2 + _lineExtent / 2;
    final clamped = target.clamp(
      _scrollController.position.minScrollExtent,
      _scrollController.position.maxScrollExtent,
    );

    _scrollController.animateTo(
      clamped,
      duration: const Duration(milliseconds: 350),
      curve: Curves.easeOutCubic,
    );
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final lyrics = widget.lyrics;

    if (lyrics == null || lyrics.lines.isEmpty) {
      return Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.lyrics_outlined, size: 46, color: theme.colorEdge),
            const SizedBox(height: 12),
            Text('没有找到歌词', style: theme.midTextStyle),
            const SizedBox(height: 6),
            Text('把同名的 .lrc 文件放在歌曲旁边，重新导入即可', style: theme.littleTextStyle),
          ],
        ),
      );
    }

    return ShaderMask(
      // 上下边缘淡出，滚动时不会出现生硬的截断
      shaderCallback: (rect) {
        return const LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            Colors.transparent,
            Colors.black,
            Colors.black,
            Colors.transparent,
          ],
          stops: [0.0, 0.12, 0.88, 1.0],
        ).createShader(rect);
      },
      blendMode: BlendMode.dstIn,
      child: ListView.builder(
        controller: _scrollController,
        padding: const EdgeInsets.symmetric(vertical: 40),
        itemExtent: _lineExtent,
        itemCount: lyrics.lines.length,
        itemBuilder: (context, index) {
          final line = lyrics.lines[index];
          final isCurrent = index == _currentIndex;

          return InkWell(
            onTap: () => widget.onSeekToLine?.call(index),
            child: Container(
              padding: const EdgeInsets.symmetric(horizontal: 24),
              alignment: Alignment.centerLeft,
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    line.text,
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                    style: TextStyle(
                      fontSize: isCurrent ? 22 : 18,
                      fontWeight: isCurrent
                          ? FontWeight.bold
                          : FontWeight.normal,
                      color: isCurrent
                          ? theme.colorStress
                          : theme.colorTextSecond.withValues(alpha: 0.75),
                    ),
                  ),
                  if ((line.translation ?? '').isNotEmpty)
                    Padding(
                      padding: const EdgeInsets.only(top: 4),
                      child: Text(
                        line.translation!,
                        maxLines: 1,
                        overflow: TextOverflow.ellipsis,
                        style: TextStyle(
                          fontSize: isCurrent ? 15 : 13,
                          color: isCurrent
                              ? theme.colorTextMain.withValues(alpha: 0.85)
                              : theme.colorTextSecond.withValues(alpha: 0.5),
                        ),
                      ),
                    ),
                ],
              ),
            ),
          );
        },
      ),
    );
  }

  /// 给外部（比如页面初始化）算当前行用
  static int indexForTime(LyricDocument document, double seconds) {
    final timeMs = (seconds * 1000).round();
    int result = -1;
    for (int i = 0; i < document.lines.length; ++i) {
      if (document.lines[i].startMs <= timeMs) {
        result = i;
      } else {
        break;
      }
    }
    return math.max(result, -1);
  }
}
