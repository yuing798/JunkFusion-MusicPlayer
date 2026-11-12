import 'dart:convert';
import 'dart:ffi';

import 'package:collection/collection.dart';
import 'package:ffi/ffi.dart';
import 'package:flutter/foundation.dart';
import 'package:junk_fusion_ui/Macro/coordinatorMacro.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/model/playlist_basic_info.dart';
import 'package:junk_fusion_ui/model/playlist_detail_info.dart';
import 'package:junk_fusion_ui/model/song_basic_info.dart';
import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';
import 'package:junk_fusion_ui/widgets/popup_window.dart';

// ════════════════════════════════════════════════════════════════
// playlist_provider.dart — 歌单视图的数据源
//
//
// ── 需要的后端接口（详细约定见我给的那份清单）──────────────────
//   getAllPlaylistView()
//     → { "playlistsList": [ {playlistId, title, songCount, hash}, ... ] }
//   getDetailPlaylistInfo(playlistId)
//     → { playlistId, title, hash, songsLists: [songId, ...] }
//   createPlaylist(title) -> 新 playlistId（失败 -1）
//   renamePlaylist(playlistId, newTitle)
//   deletePlaylist(playlistId)
//   addSongsToPlaylist(playlistId, songIdListJson)
//   removeSongsFromPlaylist(playlistId, songIdListJson)
//
// 歌曲元数据只有一份：这里只存「歌单里有哪些 songId」，
// 取歌的时候回 SongProvider 拿最新的 SongBasicInfo
// （和 AlbumProvider 完全一个套路）。
// ════════════════════════════════════════════════════════════════
class PlaylistProvider extends ChangeNotifier {
  final SongProvider songProvider;

  PlaylistProvider({required this.songProvider});

  List<PlaylistBasicInfo> _playlists = [];
  List<PlaylistBasicInfo> get playlists => _playlists;
  int get playlistCount => _playlists.length;

  /// 当前打开的歌单（列表项那份）
  PlaylistBasicInfo? _curBasicInfo;
  PlaylistBasicInfo? get curBasicInfo => _curBasicInfo;

  /// 当前打开的歌单（完整信息）
  PlaylistDetailInfo? _curDetailInfo;
  PlaylistDetailInfo? get curDetailInfo => _curDetailInfo;

  /// 当前歌单里的歌（SongProvider 里的实时数据）
  List<SongBasicInfo> _songsInPlaylist = [];
  List<SongBasicInfo> get songsInPlaylist => _songsInPlaylist;
  int get songCount => _songsInPlaylist.length;

  /// 拉一遍歌单列表（进歌单页的时候调一次）
  void getAllPlaylists() {
    try {
      final decoded = jsonDecode(
        UtilFunction.cPtr2String(bindings.getAllPlaylistView()),
      );
      // 后端返回 {"playlistsList": [...]}；库异常时会退化成空数组，
      // 这里再做一层保护，避免 null 把启动流程炸掉
      final list = decoded is Map ? decoded[CoordinatorMacro.playlistsList] : null;

      _playlists = (list is List)
          ? list.map((e) => PlaylistBasicInfo.fromJson(e)).toList()
          : <PlaylistBasicInfo>[];
    } catch (e) {
      print('PlaylistProvider: 读取歌单失败 $e');
      _playlists = <PlaylistBasicInfo>[];
    }
    notifyListeners();
  }

  /// 进某个歌单：把歌单详情和里面的歌准备好
  /// （和 AlbumProvider.setNewAlbum() 一个套路，SinglePlaylistPage 从这里读）
  void setNewPlaylist(int playlistId) {
    _curBasicInfo = _playlists
        .where((each) => each.id == playlistId)
        .firstOrNull;

    _curDetailInfo = PlaylistDetailInfo.fromJson(
      jsonDecode(
        UtilFunction.cPtr2String(bindings.getDetailPlaylistInfo(playlistId)),
      ),
    );
    // SongProvider 现在是 id -> 基本信息 的缓存：缺的先补一次，再按歌单里的顺序取
    final songIds = _curDetailInfo?.songsLists;
    if (songIds != null) {
      songProvider.ensureSongsLoaded(songIds);
      _songsInPlaylist = songIds
          .map((each) => songProvider.idToInfoMap[each])
          .whereType<SongBasicInfo>()
          .toList();
    }
    notifyListeners();
  }

  /// 打开「新建歌单」弹窗（标题输入 + 点选/拖拽封面图片）
  Future<int?> _createEmptyPlaylist() async {
    final result = await DialogUtil.showPlaylistCreateDialog();
    // 用户点了取消 / 关掉了弹窗
    if (result == null) return null;
    final ptrTitle = UtilFunction.string2cPtr(result.title);
    final ptrCover = result.imagePath != null
        ? UtilFunction.string2cPtr(result.imagePath!)
        : UtilFunction.string2cPtr("");

    print("准备执行后端函数:createNewPlaylist");
    final id = bindings.createNewPlaylist(ptrTitle, ptrCover);
    print("返回结果$id");
    malloc.free(ptrTitle);
    malloc.free(ptrCover);
    //   context.read<PlaylistProvider>().getAllPlaylists(); // 刷新列表
    // 现在后端还没有歌单接口，先把弹窗拿到的东西打出来
    print('[歌单] 待创建：${result.title}，封面图片：${result.imagePath ?? '（没选）'}');
    return id;
  }

  Future<void> createPlaylist(List<int>? songsList) async {
    final playlistId = await _createEmptyPlaylist();
    if (playlistId == null) return;
    if (songsList != null) {
      final ptr = UtilFunction.string2cPtr(jsonEncode(songsList));
      bindings.addNewSongsToPlaylist(playlistId, ptr);
      malloc.free(ptr);
    }
    // print(
    //   jsonDecode(
    //     UtilFunction.cPtr2String(bindings.getPlaylistBasicInfo(playlistId)),
    //   ),
    // );
    _playlists.add(
      PlaylistBasicInfo.fromJson(
        jsonDecode(
          UtilFunction.cPtr2String(bindings.getPlaylistBasicInfo(playlistId)),
        ),
      ),
    );
    notifyListeners();
  }

  void deletePlaylist(int playlistId) {
    bindings.deletePlaylist(playlistId);
    final index = _playlists.indexWhere((each) => each.id == playlistId);
    if (index != -1) {
      _playlists.removeAt(index);
    }
    notifyListeners();
  }

  /// 把一批歌加进若干个歌单（「收藏到歌单」弹窗用）
  ///
  /// 后端接口是 addNewSongsToPlaylist(playlistId, songIdsJson)，
  /// 多选歌单就循环调；songIdsJson 只分配一次，循环里复用。
  void addSongsTo(List<int> playlistIds, List<int> songIds) {
    if (playlistIds.isEmpty || songIds.isEmpty) return;

    final ptr = UtilFunction.string2cPtr(jsonEncode(songIds));
    try {
      for (final playlistId in playlistIds) {
        bindings.addNewSongsToPlaylist(playlistId, ptr);
      }
    } finally {
      malloc.free(ptr);
    }

    // 歌单卡片的封面/数量可能变了，整表刷一遍最省事（歌单数量不大）
    getAllPlaylists();
  }
}
