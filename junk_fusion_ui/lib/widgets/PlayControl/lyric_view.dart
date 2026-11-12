/// 歌词滚动视图：同时间戳成组显示、整组定位、逐字扫色、点击跳转。
///
/// 用法：LyricView(lyrics: ..., positionSeconds: ..., durationSeconds: ...,
/// normalColor: ..., highlightColor: ..., onSeekToLine: ...)。颜色参数供设置页
/// 接入；未指定时使用局部主题。回调仍返回原始歌词行号。
/// 组内仅留 4px，组间留 32px；每行保留最大字号高度，切换高亮不改变滚动位置。
/// 时间计算交给 LyricTimeline，真实逐字时间优先，普通 LRC 均分相邻组的时长。
library;

import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:flutter_smooth_wheel_scroll/flutter_smooth_wheel_scroll.dart';
import 'package:junk_fusion_ui/model/lyric_info.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/lyric_timeline.dart';
import 'package:provider/provider.dart';

class LyricView extends StatefulWidget {
  const LyricView({
    super.key,
    required this.lyrics,
    required this.positionSeconds,
    this.durationSeconds,
    this.normalColor,
    this.highlightColor,
    this.onSeekToLine,
  });

  final LyricDocument? lyrics;
  final double positionSeconds;
  final double? durationSeconds;

  /// 普通歌词及当前行尚未唱到的文字颜色。
  final Color? normalColor;

  /// 当前组已经唱到的文字颜色，原文和翻译共用。
  final Color? highlightColor;

  /// 参数是原始行号，由调用方跳转到对应时间。
  final ValueChanged<int>? onSeekToLine;

  @override
  State<LyricView> createState() => _LyricViewState();
}

class _LyricViewState extends State<LyricView> {
  final ScrollController _scrollController = SmoothScrollController();
  late LyricTimeline _timeline;
  int _currentGroup = -1;
  List<double> _offsets = [];
  double _viewportHeight = 0;
  TextScaler? _textScaler;
  bool _scrollScheduled = false;

  int get _positionMs => (widget.positionSeconds * 1000).round();

  @override
  void initState() {
    super.initState();
    _updateTimeline();
  }

  void _updateTimeline() {
    _timeline = LyricTimeline(
      widget.lyrics ?? const LyricDocument(),
      durationMs: widget.durationSeconds == null
          ? null
          : (widget.durationSeconds! * 1000).round(),
    );
    _currentGroup = _timeline.groupForTime(_positionMs);
    _offsets = [];
    _scheduleScroll();
  }

  @override
  void dispose() {
    _scrollController.dispose();
    super.dispose();
  }

  @override
  void didUpdateWidget(covariant LyricView oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (!identical(widget.lyrics, oldWidget.lyrics) ||
        widget.durationSeconds != oldWidget.durationSeconds) {
      _updateTimeline();
      return;
    }
    final group = _timeline.groupForTime(_positionMs);
    if (group != _currentGroup) {
      _currentGroup = group;
      _scheduleScroll();
    }
  }

  void _scheduleScroll() {
    if (_scrollScheduled) return;
    _scrollScheduled = true;
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _scrollScheduled = false;
      if (!mounted || !_scrollController.hasClients || _offsets.isEmpty) return;
      // 列表上下各留半屏，因此只需要滚到当前组在内容中的中心。
      final target = _currentGroup < 0
          ? 0.0
          : (_offsets[_currentGroup] + _offsets[_currentGroup + 1]) / 2;
      _scrollController.animateTo(
        target.clamp(0.0, _scrollController.position.maxScrollExtent),
        duration: const Duration(milliseconds: 350),
        curve: Curves.easeOutCubic,
      );
    });
  }

  void _updateLayout(double height, TextScaler scaler) {
    if (_offsets.isNotEmpty &&
        height == _viewportHeight &&
        scaler == _textScaler) {
      return;
    }
    _viewportHeight = height;
    _textScaler = scaler;
    _offsets = [0];
    for (final group in _timeline.groups) {
      var extent = 32.0;
      for (var i = group.firstLine; i < group.endLine; i++) {
        if (i > group.firstLine) extent += 4;
        extent += scaler.scale(22) * 1.5;
        if ((_timeline.document.lines[i].translation ?? '').isNotEmpty) {
          extent += 4 + scaler.scale(15) * 1.5;
        }
      }
      _offsets.add(_offsets.last + extent);
    }
    _scheduleScroll();
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final normalColor = widget.normalColor ?? theme.colorTextSecond;
    final highlightColor = widget.highlightColor ?? theme.colorStress;

    if (_timeline.groups.isEmpty) {
      return Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.lyrics_outlined, size: 46, color: normalColor),
            const SizedBox(height: 12),
            Text('没有找到歌词', style: theme.midTextStyle),
            const SizedBox(height: 6),
            Text('把同名的 .lrc 文件放在歌曲旁边，重新导入即可', style: theme.littleTextStyle),
          ],
        ),
      );
    }

    return LayoutBuilder(
      builder: (context, constraints) {
        final scaler = MediaQuery.textScalerOf(context);
        _updateLayout(constraints.maxHeight, scaler);
        return ShaderMask(
          shaderCallback: (rect) => const LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [
              Colors.transparent,
              Colors.black,
              Colors.black,
              Colors.transparent,
            ],
            stops: [0.0, 0.12, 0.88, 1.0],
          ).createShader(rect),
          blendMode: BlendMode.dstIn,
          child: ListView.builder(
            controller: _scrollController,
            padding: EdgeInsets.symmetric(vertical: constraints.maxHeight / 2),
            itemExtentBuilder: (index, _) =>
                _offsets[index + 1] - _offsets[index],
            itemCount: _timeline.groups.length,
            itemBuilder: (context, groupIndex) {
              final group = _timeline.groups[groupIndex];
              final isCurrent = groupIndex == _currentGroup;
              return Padding(
                padding: const EdgeInsets.symmetric(
                  horizontal: 24,
                  vertical: 16,
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    for (var i = group.firstLine; i < group.endLine; i++) ...[
                      if (i > group.firstLine) const SizedBox(height: 4),
                      InkWell(
                        onTap: () => widget.onSeekToLine?.call(i),
                        child: Semantics(
                          selected: isCurrent,
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.stretch,
                            children: [
                              _buildText(
                                context,
                                groupIndex,
                                i,
                                isCurrent,
                                normalColor,
                                highlightColor,
                                scaler,
                              ),
                              if ((_timeline.document.lines[i].translation ??
                                      '')
                                  .isNotEmpty) ...[
                                const SizedBox(height: 4),
                                _buildText(
                                  context,
                                  groupIndex,
                                  i,
                                  isCurrent,
                                  normalColor,
                                  highlightColor,
                                  scaler,
                                  translation: true,
                                ),
                              ],
                            ],
                          ),
                        ),
                      ),
                    ],
                  ],
                ),
              );
            },
          ),
        );
      },
    );
  }

  Widget _buildText(
    BuildContext context,
    int groupIndex,
    int lineIndex,
    bool isCurrent,
    Color normal,
    Color highlight,
    TextScaler scaler, {
    bool translation = false,
  }) {
    final line = _timeline.document.lines[lineIndex];
    final text = translation ? line.translation! : line.text;
    final fontSize = translation
        ? (isCurrent ? 15.0 : 13.0)
        : (isCurrent ? 22.0 : 18.0);
    final style = DefaultTextStyle.of(context).style.copyWith(
      fontSize: fontSize,
      height: 1.5,
      fontWeight: isCurrent ? FontWeight.bold : FontWeight.normal,
      color: normal,
    );
    return Semantics(
      label: text,
      child: SizedBox(
        height: scaler.scale(translation ? 15 : 22) * 1.5,
        child: CustomPaint(
          painter: _LyricTextPainter(
            text: text,
            style: style,
            textScaler: scaler,
            direction: Directionality.of(context),
            highlightColor: highlight,
            progress: isCurrent
                ? _timeline.characterProgress(
                    groupIndex: groupIndex,
                    lineIndex: lineIndex,
                    positionMs: _positionMs,
                    translation: translation,
                  )
                : 0,
          ),
        ),
      ),
    );
  }
}

/// 按实际字符边界裁剪高亮，英文窄字、中文、emoji 都按字符均分时间。
/// 使用 TextPainter 的 selection boxes，避免简单按整行宽度计算而产生速度偏差。
class _LyricTextPainter extends CustomPainter {
  _LyricTextPainter({
    required this.text,
    required this.style,
    required this.textScaler,
    required this.direction,
    required this.highlightColor,
    required this.progress,
  });

  final String text;
  final TextStyle style;
  final TextScaler textScaler;
  final TextDirection direction;
  final Color highlightColor;
  final double progress;

  @override
  void paint(Canvas canvas, Size size) {
    final painter = TextPainter(
      text: TextSpan(text: text, style: style),
      textDirection: direction,
      textScaler: textScaler,
      maxLines: 1,
      ellipsis: '…',
    )..layout(maxWidth: size.width);
    final offset = Offset(0, (size.height - painter.height) / 2);
    painter.paint(canvas, offset);
    if (progress <= 0) {
      painter.dispose();
      return;
    }

    final characters = text.characters.toList();
    final complete = progress.floor().clamp(0, characters.length);
    final end = characters.take(complete).join().length;
    final clip = Path();
    for (final box in painter.getBoxesForSelection(
      TextSelection(baseOffset: 0, extentOffset: end),
    )) {
      clip.addRect(box.toRect().shift(offset));
    }
    if (complete < characters.length) {
      final fraction = progress - complete;
      for (final box in painter.getBoxesForSelection(
        TextSelection(
          baseOffset: end,
          extentOffset: end + characters[complete].length,
        ),
      )) {
        final rect = box.toRect();
        final width = rect.width * fraction;
        clip.addRect(
          Rect.fromLTWH(
            box.direction == TextDirection.rtl ? rect.right - width : rect.left,
            rect.top,
            math.max(0, width),
            rect.height,
          ).shift(offset),
        );
      }
    }
    canvas.save();
    canvas.clipPath(clip);
    painter.text = TextSpan(
      text: text,
      style: style.copyWith(color: highlightColor),
    );
    painter.layout(maxWidth: size.width);
    painter.paint(canvas, offset);
    canvas.restore();
    painter.dispose();
  }

  @override
  bool shouldRepaint(covariant _LyricTextPainter oldDelegate) =>
      text != oldDelegate.text ||
      style != oldDelegate.style ||
      textScaler != oldDelegate.textScaler ||
      direction != oldDelegate.direction ||
      highlightColor != oldDelegate.highlightColor ||
      progress != oldDelegate.progress;
}
