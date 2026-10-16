import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:junk_fusion_ui/Macro/coordinatorMacro.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/model/folder_basic_info.dart';
import 'package:junk_fusion_ui/model/folder_detail_info.dart';
import 'package:junk_fusion_ui/model/song_basic_info.dart';
import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';

// ════════════════════════════════════════════════════════════════
// folder_provider.dart — 文件夹视图的数据源
//
// TODO(后端): 文件夹现在也**一个接口都没有**
//   （Coordinator/CatagoryViewManage/FileDirManage 是个空壳），
//   所以这里同样只写清楚「要什么数据」，查询一律返回空列表。
//
// ── 需要的后端接口（详细约定见我给的那份清单）──────────────────
//   getAllFolderView()
//     → { "foldersList": [ {path, name, songCount, hash}, ... ] }
//   getSongsListByFolderPath(folderPath)
//     → { "songIdList": [songId, ...] }        // 复用 CoordinatorMacro.songIdList
//
// 说明：文件夹不需要新表 —— 后端直接拿 songs.filePath 的父目录算出来就行，
//       前端拿 path 当唯一标识（FolderBasicInfo.path）。
//
// 歌曲元数据只有一份：这里只存「文件夹里有哪些 songId」，
// 取歌的时候回 SongProvider 拿最新的 SongBasicInfo。
// ════════════════════════════════════════════════════════════════
class FolderProvider extends ChangeNotifier {
  final SongProvider songProvider;

  FolderProvider({required this.songProvider});

  List<FolderBasicInfo> _folders = [];
  List<FolderBasicInfo> get folders => _folders;
  int get folderCount => _folders.length;

  /// 当前打开的文件夹（列表项那份）
  FolderBasicInfo? _curBasicInfo;
  FolderBasicInfo? get curBasicInfo => _curBasicInfo;

  /// 当前打开的文件夹（完整信息）
  FolderDetailInfo? _curDetailInfo;
  FolderDetailInfo? get curDetailInfo => _curDetailInfo;

  /// 当前文件夹里的歌（SongProvider 里的实时数据）
  List<SongBasicInfo> _songsInFolder = [];
  List<SongBasicInfo> get songsInFolder => _songsInFolder;
  int get songCount => _songsInFolder.length;

  /// 拉一遍文件夹列表（进文件夹页的时候调一次）
  void getAllFolders() {
    // TODO(后端): getAllFolderView() 注册好之后，这里换成：
    //   _folders = (jsonDecode(
    //               UtilFunction.cPtr2String(bindings.getAllFolderView()),
    //             )?[CoordinatorMacro.foldersList] as List<dynamic>)
    //       .map((e) => FolderBasicInfo.fromJson(e))
    //       .toList();
    _folders = [];

    print('[FolderProvider] 文件夹接口还没做，getAllFolders() 先返回空列表');
    notifyListeners();
  }

  /// 进某个文件夹：把这个文件夹里的歌准备好
  /// （path 就是文件夹的唯一标识；SingleFolderPage 从这里读）
  void setNewFolder(String path) {
    _curBasicInfo = _folders.where((each) => each.path == path).firstOrNull;

    // TODO(后端): getSongsListByFolderPath(folderPath) 注册好之后，这里换成：
    //   final songIds = (jsonDecode(
    //             UtilFunction.cPtr2String(
    //               bindings.getSongsListByFolderPath(path),
    //             ),
    //           )?[CoordinatorMacro.songIdList] as List<dynamic>)
    //       .map((e) => (e as num).toInt())
    //       .toList();
    //   final songIdMap = {for (final e in songProvider.songs) e.songId: e};
    //   _songsInFolder = songIds
    //       .map((each) => songIdMap[each])
    //       .whereType<SongBasicInfo>()
    //       .toList();
    _curDetailInfo = null;
    _songsInFolder = [];

    notifyListeners();
  }
}
