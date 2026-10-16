import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:junk_fusion_ui/Macro/coordinatorMacro.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/model/album_basic_info.dart';
import 'package:junk_fusion_ui/model/album_detail_info.dart';
import 'package:junk_fusion_ui/model/song_basic_info.dart';

import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/utils/app_cache.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';

// ════════════════════════════════════════════════════════════════
// album_provider.dart — 专辑视图的数据源
//
// 后端提供了两个专辑维度的接口（见 bridge/native_bindings_generated.dart）：
//   getAllAlbumView()          → { "albumsList": [ AlbumBasicInfo, ... ] }
//   getSongsListByAlbumId(id)  → { "songIdList": [ songId, ... ] }
//
// 做法：
//   1. 专辑列表整体拉一次，缓存到 _albums；
//   2. 「专辑 → songId」按专辑缓存（后端返回的是数据库顺序，不带排序）；
//   3. 取歌的时候再回 SongProvider 拿最新的 SongInfo —— 歌曲元数据、
//      「我喜欢」这些状态只有一份，不会因为这里缓存了一份就变旧。
//
// 什么时候重新拉：SongProvider.revision 变了（导入新歌、后端改完元数据）。
// 仅仅切换「我喜欢」不会改 revision，所以不会白跑查库。
// ════════════════════════════════════════════════════════════════
class AlbumProvider extends ChangeNotifier {
  final SongProvider songProvider;

  AlbumProvider({required this.songProvider});

  List<AlbumBasicInfo> _albums = [];

  List<AlbumBasicInfo> get albums => _albums;
  int get albumCount => _albums.length;

  AlbumDetailInfo? _curDetailInfo;
  AlbumDetailInfo? get curDetailInfo => _curDetailInfo;

  AlbumBasicInfo? _curBasicInfo;
  AlbumBasicInfo? get curBasicInfo => _curBasicInfo;

  List<SongBasicInfo> _songsInAlbums = [];
  List<SongBasicInfo> get songsInAlbums => _songsInAlbums;
  int get songCount => _songsInAlbums.length;

  void setNewAlbum(int albumId) {
    _curBasicInfo = _albums
        .where((each) => each.albumId == albumId)
        .firstOrNull;

    // print(
    //   jsonDecode(
    //     UtilFunction.cPtr2String(bindings.getCompleteAlbumInfo(albumId)),
    //   ),
    // );

    _curDetailInfo = AlbumDetailInfo.fromJson(
      jsonDecode(
        UtilFunction.cPtr2String(bindings.getCompleteAlbumInfo(albumId)),
      ),
    );

    if (_curDetailInfo == null || _curBasicInfo == null) return;

    // print(1);
    //用map而非set是为了保证顺序正确
    final songIdMap = {for (final e in songProvider.songs) e.songId: e};
    _songsInAlbums = _curDetailInfo!.songsLists
        .map((each) => songIdMap[each])
        .whereType<SongBasicInfo>()
        .toList();

    // print(7);

    notifyListeners();
  }

  /// 拉一遍专辑列表(初始化的时候调用)
  void getAllAlbums() {
    _albums =
        (jsonDecode(
                  UtilFunction.cPtr2String(bindings.getAllAlbumView()),
                )?[CoordinatorMacro.albumsList]
                as List<dynamic>)
            .map((e) => AlbumBasicInfo.fromJson(e))
            .toList();

    notifyListeners();
  }
}
