import 'package:freezed_annotation/freezed_annotation.dart';

part "playlist_detail_info.freezed.dart";
part "playlist_detail_info.g.dart";

// ════════════════════════════════════════════════════════════════
// playlist_detail_info.dart — 单个歌单的完整信息
//
// 对应后端 getCompletePlaylistInfo(playlistId)，结构和
// AlbumDetailInfo 一样：自己这一层的字段 + songsLists（歌单里的 songId，
// 按歌单内的顺序）。真正的歌曲信息还是回 SongProvider 拿，不在这里存一份。
//
// TODO(后端): 接口还没有，见 PlaylistProvider 顶部的约定
// ════════════════════════════════════════════════════════════════

@freezed
abstract class PlaylistDetailInfo with _$PlaylistDetailInfo {
  const factory PlaylistDetailInfo({
    /// 歌单里的歌曲 id（按歌单内的顺序）
    required List<int> songsLists,
  }) = _PlaylistDetailInfo;

  factory PlaylistDetailInfo.fromJson(Map<String, dynamic> json) =>
      _$PlaylistDetailInfoFromJson(json);
}
