import 'package:freezed_annotation/freezed_annotation.dart';

part "album_detail_info.freezed.dart";
part "album_detail_info.g.dart";

@freezed
abstract class AlbumDetailInfo with _$AlbumDetailInfo {
  const factory AlbumDetailInfo({required List<int> songsLists}) =
      _AlbumDetailInfo;

  factory AlbumDetailInfo.fromJson(Map<String, dynamic> json) =>
      _$AlbumDetailInfoFromJson(json);
}
