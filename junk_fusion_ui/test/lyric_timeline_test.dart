/// 测试 LyricTimeline 的同时间戳分组及真实/模拟逐字进度。
/// 通过标准：同组使用下一不同时间戳；普通歌词按字符均分；真实时间优先；
/// 翻译、末句、未知时长、空文本、Unicode 字符及向后跳转均给出正确结果。
/// 原因：这些是歌词渲染所依赖的时间计算，零时长和多行时间戳容易导致错亮。
/// 运行：flutter test test/lyric_timeline_test.dart。
library;

import 'package:flutter_test/flutter_test.dart';
import 'package:junk_fusion_ui/model/lyric_info.dart';
import 'package:junk_fusion_ui/utils/lyric_timeline.dart';

void main() {
  const bilingual = LyricDocument(
    lines: [
      LyricLine(startMs: 11400, text: 'Young black cool independent'),
      LyricLine(startMs: 11400, text: '年轻、酷炫、独立'),
      LyricLine(startMs: 15400, text: 'Next'),
    ],
  );

  test('同时间戳全部成组，首句之前和精确边界正确，支持向后跳转', () {
    final timeline = LyricTimeline(bilingual, durationMs: 19400);
    expect(timeline.groups.length, 2);
    expect(timeline.groups.first.firstLine, 0);
    expect(timeline.groups.first.endLine, 2);
    expect(timeline.groups.first.endMs, 15400);
    expect(timeline.groupForTime(11399), -1);
    expect(timeline.groupForTime(11400), 0);
    expect(timeline.groupForTime(15400), 1);
    expect(timeline.groupForTime(12000), 0);
    expect(LyricTimeline(const LyricDocument()).groupForTime(1000), -1);
  });

  test('同组不同长度文本按下一不同时间戳同步推进', () {
    final timeline = LyricTimeline(bilingual);
    for (var line = 0; line < 2; line++) {
      expect(
        timeline.characterProgress(
          groupIndex: 0,
          lineIndex: line,
          positionMs: 13400,
        ),
        bilingual.lines[line].text.length / 2,
      );
      expect(
        timeline.characterProgress(
          groupIndex: 0,
          lineIndex: line,
          positionMs: 15400,
        ),
        bilingual.lines[line].text.length.toDouble(),
      );
    }
  });

  test('末组使用歌曲时长，未知或无效时长则整行高亮', () {
    final timeline = LyricTimeline(bilingual, durationMs: 19400);
    expect(
      timeline.characterProgress(
        groupIndex: 1,
        lineIndex: 2,
        positionMs: 17400,
      ),
      2,
    );
    expect(
      timeline.characterProgress(
        groupIndex: 1,
        lineIndex: 2,
        positionMs: 25000,
      ),
      4,
    );
    for (final duration in [null, 0, 15400]) {
      final unknown = LyricTimeline(bilingual, durationMs: duration);
      expect(
        unknown.characterProgress(
          groupIndex: 1,
          lineIndex: 2,
          positionMs: 15400,
        ),
        4,
      );
    }
  });

  test('Unicode 按用户可见字符计时，空文本不除零', () {
    final timeline = LyricTimeline(
      const LyricDocument(
        lines: [
          LyricLine(startMs: 1000, text: '👨‍👩‍👧‍👦e\u0301你'),
          LyricLine(startMs: 1000, text: ''),
          LyricLine(startMs: 4000, text: '结束'),
        ],
      ),
    );
    expect(
      timeline.characterProgress(groupIndex: 0, lineIndex: 0, positionMs: 2000),
      1,
    );
    expect(
      timeline.characterProgress(groupIndex: 0, lineIndex: 0, positionMs: 2500),
      1.5,
    );
    expect(
      timeline.characterProgress(groupIndex: 0, lineIndex: 1, positionMs: 2500),
      0,
    );
    expect(
      timeline.characterProgress(groupIndex: 0, lineIndex: 0, positionMs: 500),
      0,
    );
  });

  test('增强歌词遵循真实时间和字间停顿，翻译单独均分整段', () {
    final timeline = LyricTimeline(
      const LyricDocument(
        lines: [
          LyricLine(
            startMs: 1000,
            text: '你好世界',
            translation: 'Hello',
            words: [
              LyricWord(startMs: 1000, endMs: 1500, text: '你'),
              LyricWord(startMs: 2000, endMs: 3000, text: '好世界'),
            ],
          ),
          LyricLine(startMs: 5000, text: '下一句'),
        ],
      ),
    );
    expect(
      timeline.characterProgress(groupIndex: 0, lineIndex: 0, positionMs: 1250),
      .5,
    );
    expect(
      timeline.characterProgress(groupIndex: 0, lineIndex: 0, positionMs: 1750),
      1,
    );
    expect(
      timeline.characterProgress(groupIndex: 0, lineIndex: 0, positionMs: 2500),
      2.5,
    );
    expect(
      timeline.characterProgress(groupIndex: 0, lineIndex: 0, positionMs: 3000),
      4,
    );
    expect(
      timeline.characterProgress(
        groupIndex: 0,
        lineIndex: 0,
        positionMs: 3000,
        translation: true,
      ),
      2.5,
    );
  });

  test('原文首尾空格被后端裁掉时仍使用真实逐字时间', () {
    final timeline = LyricTimeline(
      const LyricDocument(
        lines: [
          LyricLine(
            startMs: 1000,
            text: '你好',
            words: [
              LyricWord(startMs: 0, endMs: 1000, text: ' '),
              LyricWord(startMs: 1000, endMs: 2000, text: '你'),
              LyricWord(startMs: 2000, endMs: 3000, text: '好'),
            ],
          ),
        ],
      ),
      durationMs: 5000,
    );
    expect(
      timeline.characterProgress(groupIndex: 0, lineIndex: 0, positionMs: 1500),
      .5,
    );
  });

  test('真实逐字末尾受下一组约束，零长度时间段不产生 NaN', () {
    final timeline = LyricTimeline(
      const LyricDocument(
        lines: [
          LyricLine(
            startMs: 1000,
            text: 'AB',
            words: [
              LyricWord(startMs: 1000, endMs: 1000, text: 'A'),
              LyricWord(startMs: 1000, endMs: 9000, text: 'B'),
            ],
          ),
          LyricLine(startMs: 3000, text: 'C'),
        ],
      ),
    );
    expect(
      timeline.characterProgress(groupIndex: 0, lineIndex: 0, positionMs: 2000),
      1.5,
    );
    expect(
      timeline.characterProgress(groupIndex: 0, lineIndex: 0, positionMs: 3000),
      2,
    );
  });
}
