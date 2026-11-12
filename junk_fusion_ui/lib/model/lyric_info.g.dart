// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'lyric_info.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

_LyricWord _$LyricWordFromJson(Map<String, dynamic> json) => _LyricWord(
  startMs: (json['startMs'] as num).toInt(),
  endMs: (json['endMs'] as num).toInt(),
  text: json['text'] as String,
);

Map<String, dynamic> _$LyricWordToJson(_LyricWord instance) =>
    <String, dynamic>{
      'startMs': instance.startMs,
      'endMs': instance.endMs,
      'text': instance.text,
    };

_LyricLine _$LyricLineFromJson(Map<String, dynamic> json) => _LyricLine(
  startMs: (json['startMs'] as num).toInt(),
  text: json['text'] as String,
  translation: json['translation'] as String?,
  words:
      (json['words'] as List<dynamic>?)
          ?.map((e) => LyricWord.fromJson(e as Map<String, dynamic>))
          .toList() ??
      const <LyricWord>[],
);

Map<String, dynamic> _$LyricLineToJson(_LyricLine instance) =>
    <String, dynamic>{
      'startMs': instance.startMs,
      'text': instance.text,
      'translation': instance.translation,
      'words': instance.words,
    };

_LyricDocument _$LyricDocumentFromJson(Map<String, dynamic> json) =>
    _LyricDocument(
      hasTranslation: json['hasTranslation'] as bool? ?? false,
      lines:
          (json['lines'] as List<dynamic>?)
              ?.map((e) => LyricLine.fromJson(e as Map<String, dynamic>))
              .toList() ??
          const <LyricLine>[],
    );

Map<String, dynamic> _$LyricDocumentToJson(_LyricDocument instance) =>
    <String, dynamic>{
      'hasTranslation': instance.hasTranslation,
      'lines': instance.lines,
    };
