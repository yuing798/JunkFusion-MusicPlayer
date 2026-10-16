import 'package:freezed_annotation/freezed_annotation.dart';

part 'song_basic_info.freezed.dart';
part 'song_basic_info.g.dart'; // 用于 JSON

// SongBasicInfo — 单首歌曲的基础元数据
@freezed
abstract class SongBasicInfo with _$SongBasicInfo {
  const factory SongBasicInfo({
    required int songId,
    required double duration,
    required String title,

    required List<String>? artists,
    String? album,

    required bool isMyLike,

    required int playNum,
    String? hash,
    int? trackNumber,
    int? discNumber,
  }) = _SongBasicInfo;

  factory SongBasicInfo.fromJson(Map<String, dynamic> json) =>
      _$SongBasicInfoFromJson(json);
}
