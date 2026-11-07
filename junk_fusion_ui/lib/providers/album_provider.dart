import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:junk_fusion_ui/Macro/coordinatorMacro.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/model/album_basic_info.dart';
import 'package:junk_fusion_ui/model/album_detail_info.dart';
import 'package:junk_fusion_ui/model/song_basic_info.dart';

import 'package:junk_fusion_ui/providers/song_provider.dart';
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
// 什么时候重新拉：启动时整体拉一次；之后靠后端回调保持同步，不用整表重拉 ——
//   新建了一张专辑          → addOrUpdateAlbum()（registerOnAddNewAlbum）
//   专辑里一首歌都不剩被删掉 → deleteAlbum()      （registerOnDeleteAlbum）
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
    //     UtilFunction.cPtr2String(bindings.getDetailAlbumInfo(albumId)),
    //   ),
    // );

    _curDetailInfo = AlbumDetailInfo.fromJson(
      jsonDecode(
        UtilFunction.cPtr2String(bindings.getDetailAlbumInfo(albumId)),
      ),
    );

    if (_curDetailInfo == null || _curBasicInfo == null) return;

    // print(1);
    // SongProvider 现在是 id -> 基本信息 的缓存：缺的先补一次，再按专辑里的顺序取
    final songIds = _curDetailInfo!.songsLists;
    songProvider.ensureSongsLoaded(songIds);
    _songsInAlbums = songIds
        .map((each) => songProvider.idToInfoMap[each])
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

  /// 后端新建了一张专辑（registerOnAddNewAlbum）
  ///
  /// 参数就是 getAllAlbumView 里的一项，所以直接塞进列表即可。
  /// 列表本身不排序（排序在专辑页里按用户选的方式做），所以追加到末尾没问题。
  void addOrUpdateAlbum(AlbumBasicInfo album) {
    final index = _albums.indexWhere((each) => each.albumId == album.albumId);
    if (index >= 0) {
      // 理论上不会重复，兜底：同一个 albumId 就原地换掉
      _albums[index] = album;
    } else {
      _albums = [..._albums, album];
    }

    // 正在看的就是这张：标题/艺术家也跟着换成新的
    if (_curBasicInfo?.albumId == album.albumId) {
      _curBasicInfo = album;
    }

    notifyListeners();
  }

  /// 后端把一张专辑删掉了（registerOnDeleteAlbum，专辑里已经一首歌都不剩）
  void deleteAlbum(int albumId) {
    final before = _albums.length;
    _albums = _albums.where((each) => each.albumId != albumId).toList();

    final isCurrent = _curBasicInfo?.albumId == albumId;
    if (isCurrent) {
      // 正在看这张：详情一起清掉（单张专辑页发现 curBasicInfo 为空会自己让位）
      _curBasicInfo = null;
      _curDetailInfo = null;
      _songsInAlbums = [];
    }

    if (_albums.length != before || isCurrent) notifyListeners();
  }
}
