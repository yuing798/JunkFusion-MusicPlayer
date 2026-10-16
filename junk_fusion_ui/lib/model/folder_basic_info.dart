import 'package:freezed_annotation/freezed_annotation.dart';

part "folder_basic_info.freezed.dart";
part "folder_basic_info.g.dart";

// ════════════════════════════════════════════════════════════════
// folder_basic_info.dart — 文件夹列表项（文件夹网格里的一张卡片）
//
// 文件夹不像专辑那样有 id：后端是拿 songs.filePath 的父目录算出来的，
// 所以**路径就是它的唯一标识**（path 当 key 用）。
//
// TODO(后端): 文件夹目前完全没有接口（FileDirManage 是空壳），
//   这个模型先按约定写好，接口做完之后 FolderProvider 里填上解析即可。
//   服务端约定见 FolderProvider 顶部的注释。
// ════════════════════════════════════════════════════════════════

@freezed
abstract class FolderBasicInfo with _$FolderBasicInfo {
  const factory FolderBasicInfo({
    /// 文件夹完整路径（唯一标识，也是 ValueKey / 路由参数）
    required String path,

    /// 文件夹名（只显示最后一级）
    required String name,

    /// 这个文件夹下（含子文件夹）有多少首歌
    required int songCount,

    /// 封面哈希：取该文件夹里第一首有封面的歌，没有就是空串
    String? hash,
  }) = _FolderBasicInfo;

  factory FolderBasicInfo.fromJson(Map<String, dynamic> json) =>
      _$FolderBasicInfoFromJson(json);
}
