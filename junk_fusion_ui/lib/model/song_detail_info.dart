import 'package:freezed_annotation/freezed_annotation.dart';

part 'song_detail_info.freezed.dart';
part 'song_detail_info.g.dart'; // 用于 JSON

// SongDetailInfo — 单首歌曲的完整元数据
@freezed
abstract class SongDetailInfo with _$SongDetailInfo {
  const factory SongDetailInfo({
    required int songId,
    required double duration,
    required String title,

    required List<String>? artists,
    String? album,
    int? trackNumber,
    int? discNumber,

    required int bitRate,
    required int sampleRate,
    required String channelLayout,
    required int bitDepth,

    String? codecName,
    int? bpm,
    String? key,

    required bool isMyLike,

    String? comment,
    required int playNum,
    String? hash,
  }) = _SongDetailInfo;

  factory SongDetailInfo.fromJson(Map<String, dynamic> json) =>
      _$SongDetailInfoFromJson(json);
}
