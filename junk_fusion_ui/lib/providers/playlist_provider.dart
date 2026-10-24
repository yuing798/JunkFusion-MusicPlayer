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
// TODO(后端): 后端现在**一个歌单接口都没有**
//   （Coordinator/CatagoryViewManage/PlayListManage 只有个空构造函数，
//     dbTable.hpp 里也没有 playlists 表），
//   所以这个 Provider 现在只把「要什么数据」写清楚，查询一律返回空列表。
//   等 bindings 里注册了下面这些函数，把 TODO 处换成注释里那几行即可。
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
    _playlists =
        (jsonDecode(
                  UtilFunction.cPtr2String(bindings.getAllPlaylistView()),
                )?[CoordinatorMacro.playlistsList]
                as List<dynamic>)
            .map((e) => PlaylistBasicInfo.fromJson(e))
            .toList();
    notifyListeners();
  }

  /// 进某个歌单：把歌单详情和里面的歌准备好
  /// （和 AlbumProvider.setNewAlbum() 一个套路，SinglePlaylistPage 从这里读）
  void setNewPlaylist(int playlistId) {
    _curBasicInfo = _playlists
        .where((each) => each.playlistId == playlistId)
        .firstOrNull;

    _curDetailInfo = PlaylistDetailInfo.fromJson(
      jsonDecode(
        UtilFunction.cPtr2String(bindings.getDetailPlaylistInfo(playlistId)),
      ),
    );
    final songIdMap = {for (final e in songProvider.songs) e.songId: e};
    _songsInPlaylist = _curDetailInfo!.songsLists
        .map((each) => songIdMap[each])
        .whereType<SongBasicInfo>()
        .toList();

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
        : nullptr;
    final id = bindings.createNewPlaylist(ptrTitle, ptrCover);
    malloc.free(ptrTitle);
    malloc.free(ptrCover);
    //   context.read<PlaylistProvider>().getAllPlaylists(); // 刷新列表
    // 现在后端还没有歌单接口，先把弹窗拿到的东西打出来
    print('[歌单] 待创建：${result.title}，封面图片：${result.imagePath ?? '（没选）'}');
    return id;
  }

  void createPlaylist(List<int>? songsList) async {
    final playlistId = await _createEmptyPlaylist();
    if (playlistId == null) return;
    if (songsList != null) {
      final ptr = UtilFunction.string2cPtr(jsonEncode(songsList));
      bindings.addNewSongsToPlaylist(playlistId, ptr);
      malloc.free(ptr);
    }
    _playlists.add(
      jsonDecode(
        UtilFunction.cPtr2String(bindings.getBasicSongInfo(playlistId)),
      ),
    );
    notifyListeners();
  }

  void deletePlaylist(int playlistId) {
    bindings.deletePlaylist(playlistId);
    final index = _playlists.indexWhere(
      (each) => each.playlistId == playlistId,
    );
    if (index != -1) {
      _playlists.removeAt(index);
    }
    notifyListeners();
  }

  // ────────────────────────────────────────────────────────────
  // 歌单管理（新建 / 改名 / 删除 / 增删歌曲）
  //
  // TODO(后端): 上面列的 5 个管理函数注册好之后再在这里实现，
  //             每个方法结束后调一次 getAllPlaylists()（或局部更新）
  //             再 notifyListeners()。
  //             UI 那边现在是禁用按钮占位，见
  //             widgets/pages/playlist_view_page.dart 和
  //             widgets/pages/single_playlist_page.dart 里的 TODO。
  // ────────────────────────────────────────────────────────────
}
