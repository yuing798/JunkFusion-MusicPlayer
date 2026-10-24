import 'package:freezed_annotation/freezed_annotation.dart';

part "playlist_basic_info.freezed.dart";
part "playlist_basic_info.g.dart";

// ════════════════════════════════════════════════════════════════
// playlist_basic_info.dart — 歌单列表项（歌单网格里的一张卡片）
//
// 字段名必须和 BridgeMacro/PlaylistInfo.jsonc 里的键一致，
// freezed 生成的 fromJson 是按名字取键的。
//
// TODO(后端): 现在后端还没有歌单相关的表和接口
//   （Coordinator/CatagoryViewManage/PlayListManage 还是个空壳，
//     dbTable.hpp 里也没有 playlists 表），所以这个模型先按约定写好，
//   接口做完之后 PlaylistProvider 里填上解析就能直接用。
//   服务端约定见 PlaylistProvider 顶部的注释。
// ════════════════════════════════════════════════════════════════

@freezed
abstract class PlaylistBasicInfo with _$PlaylistBasicInfo {
  const factory PlaylistBasicInfo({
    required int playlistId,
    required String title,
    required String createTime,

    /// 封面哈希：取歌单里第一首有封面的歌（和专辑一个做法），没有就是空串
    String? hash,
  }) = _PlaylistBasicInfo;

  factory PlaylistBasicInfo.fromJson(Map<String, dynamic> json) =>
      _$PlaylistBasicInfoFromJson(json);
}
