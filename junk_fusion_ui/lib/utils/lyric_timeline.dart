/// 歌词显示时间轴：合并相同起始时间，并计算逐字高亮进度。
///
/// 用法：LyricTimeline(document, durationMs: ...)，通过 groupForTime 查当前组，
/// 通过 characterProgress 查询某行已经唱过的字符数（小数表示正在唱的字符）。
/// 普通 LRC 在当前组和下一组之间均分字符时长；末组使用歌曲总时长，未知时
/// 保持整组高亮。增强 LRC 优先使用原有 words 时间；翻译独立均分同一段时长。
/// 字符按 grapheme cluster 计数，避免拆开 emoji、代理对和组合音标。
library;

import 'dart:math' as math;

import 'package:flutter/widgets.dart' show StringCharacters;
import 'package:junk_fusion_ui/model/lyric_info.dart';

class LyricTimeline {
  LyricTimeline(this.document, {this.durationMs}) {
    final ranges = <({int firstLine, int endLine, int startMs, int? endMs})>[];
    final lines = document.lines;
    for (var first = 0; first < lines.length;) {
      var end = first + 1;
      while (end < lines.length && lines[end].startMs == lines[first].startMs) {
        end++;
      }
      final nextMs = end < lines.length ? lines[end].startMs : durationMs;
      ranges.add((
        firstLine: first,
        endLine: end,
        startMs: lines[first].startMs,
        endMs: nextMs != null && nextMs > lines[first].startMs ? nextMs : null,
      ));
      first = end;
    }
    groups = List.unmodifiable(ranges);
  }

  final LyricDocument document;
  final int? durationMs;
  late final List<({int firstLine, int endLine, int startMs, int? endMs})>
  groups;

  /// 首句之前返回 -1；相同时间戳始终属于同一组。
  int groupForTime(int positionMs) {
    var low = 0;
    var high = groups.length - 1;
    var result = -1;
    while (low <= high) {
      final mid = (low + high) ~/ 2;
      if (groups[mid].startMs <= positionMs) {
        result = mid;
        low = mid + 1;
      } else {
        high = mid - 1;
      }
    }
    return result;
  }

  double characterProgress({
    required int groupIndex,
    required int lineIndex,
    required int positionMs,
    bool translation = false,
  }) {
    final group = groups[groupIndex];
    final line = document.lines[lineIndex];
    final text = translation ? line.translation ?? '' : line.text;
    final count = text.characters.length;
    if (count == 0 || positionMs < group.startMs) return 0;

    // 后端会 trim 原文；比较时也 trim，避免空格让真实逐字时间失效。
    if (!translation &&
        line.words.isNotEmpty &&
        line.words.map((word) => word.text).join().trim() == text) {
      final wordText = line.words.map((word) => word.text).join();
      final leading =
          wordText.characters.length - wordText.trimLeft().characters.length;
      var progress = 0.0;
      for (final word in line.words) {
        final length = word.text.characters.length;
        final start = math.max(group.startMs, word.startMs);
        final end = math.min(word.endMs, group.endMs ?? word.endMs);
        if (positionMs < start) break;
        if (end <= start || positionMs >= end) {
          progress += length;
        } else {
          progress += length * (positionMs - start) / (end - start);
          break;
        }
      }
      return (progress - leading).clamp(0.0, count.toDouble());
    }

    final endMs = group.endMs;
    // 没有下一句也没有有效总时长时，不编造演唱时长。
    if (endMs == null) return count.toDouble();
    return count *
        ((positionMs - group.startMs) / (endMs - group.startMs)).clamp(
          0.0,
          1.0,
        );
  }
}
