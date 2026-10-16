import 'package:freezed_annotation/freezed_annotation.dart';

part "album_basic_info.freezed.dart";
part "album_basic_info.g.dart";

@freezed
abstract class AlbumBasicInfo with _$AlbumBasicInfo {
  const factory AlbumBasicInfo({
    required int albumId,
    required String title,
    List<String>? albumArtists,
    String? hash,
  }) = _AlbumBasicInfo;

  factory AlbumBasicInfo.fromJson(Map<String, dynamic> json) =>
      _$AlbumBasicInfoFromJson(json);
}
