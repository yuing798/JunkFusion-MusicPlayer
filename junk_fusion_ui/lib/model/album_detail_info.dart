import 'package:freezed_annotation/freezed_annotation.dart';

part "album_detail_info.freezed.dart";
part "album_detail_info.g.dart";

@freezed
abstract class AlbumDetailInfo with _$AlbumDetailInfo {
  const factory AlbumDetailInfo({
    required int albumId,
    required String title,
    List<String>? albumArtists,
    String? hash,
    List<int>? songsList,
  }) = _AlbumDetailInfo;

  factory AlbumDetailInfo.fromJson(Map<String, dynamic> json) =>
      _$AlbumDetailInfoFromJson(json);
}
