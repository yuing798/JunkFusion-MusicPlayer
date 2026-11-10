/// 歌词模型（对应后端 getSongLyrics 返回的 lyric 对象）
///
/// ── 结构 ─────────────────────────────────────────────────────────────
///     LyricDocument
///       ├─ hasTranslation  有没有翻译（决定渲染时要不要多留一行）
///       └─ lines[]
///            ├─ startMs    这一行开始的时间（毫秒）
///            ├─ text       原文
///            ├─ translation 翻译（可能为空）
///            └─ words[]    逐字时间（只有增强 LRC 才有，普通 LRC 是空数组）
///
/// ── 为什么用 freezed ─────────────────────────────────────────────────
/// 项目里 model/ 统一 freezed；歌词是只读数据，用 freezed 直接拿到
/// fromJson/copyWith/==，省得手写。
library;

import 'package:freezed_annotation/freezed_annotation.dart';

part 'lyric_info.freezed.dart';
part 'lyric_info.g.dart';

@freezed
abstract class LyricWord with _$LyricWord {
  const factory LyricWord({
    required int startMs,
    required int endMs,
    required String text,
  }) = _LyricWord;

  factory LyricWord.fromJson(Map<String, dynamic> json) =>
      _$LyricWordFromJson(json);
}

@freezed
abstract class LyricLine with _$LyricLine {
  const factory LyricLine({
    required int startMs,
    required String text,
    String? translation,
    @Default(<LyricWord>[]) List<LyricWord> words,
  }) = _LyricLine;

  factory LyricLine.fromJson(Map<String, dynamic> json) =>
      _$LyricLineFromJson(json);
}

@freezed
abstract class LyricDocument with _$LyricDocument {
  const factory LyricDocument({
    @Default(false) bool hasTranslation,
    @Default(<LyricLine>[]) List<LyricLine> lines,
  }) = _LyricDocument;

  factory LyricDocument.fromJson(Map<String, dynamic> json) =>
      _$LyricDocumentFromJson(json);
}
