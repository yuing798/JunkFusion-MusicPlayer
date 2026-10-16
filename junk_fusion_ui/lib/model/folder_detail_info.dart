import 'package:freezed_annotation/freezed_annotation.dart';

part "folder_detail_info.freezed.dart";
part "folder_detail_info.g.dart";

// ════════════════════════════════════════════════════════════════
// folder_detail_info.dart — 单个文件夹的完整信息
//
// 对应后端 getSongsListByFolderPath(folderPath)：
// 文件夹本身没有表，路径就是它的身份，所以这里存 path + 这个文件夹里的 songId。
// 真正的歌曲信息回 SongProvider 拿。
//
// TODO(后端): 接口还没有，见 FolderProvider 顶部的约定
// ════════════════════════════════════════════════════════════════

@freezed
abstract class FolderDetailInfo with _$FolderDetailInfo {
  const factory FolderDetailInfo({
    /// 文件夹完整路径（唯一标识）
    required String path,

    /// 文件夹名
    required String name,

    /// 封面哈希：该文件夹里第一首有封面的歌，没有就是空串
    String? hash,

    /// 这个文件夹里的歌曲 id
    List<int>? songsLists,
  }) = _FolderDetailInfo;

  factory FolderDetailInfo.fromJson(Map<String, dynamic> json) =>
      _$FolderDetailInfoFromJson(json);
}
