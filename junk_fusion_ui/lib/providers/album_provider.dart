import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:junk_fusion_ui/Macro/coordinatorMacro.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/model/album_view_info.dart';
import 'package:junk_fusion_ui/model/song_info.dart';
import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/utils/app_cache.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';

// ════════════════════════════════════════════════════════════════
// album_provider.dart — 专辑视图的数据源
//
// 后端提供了两个专辑维度的接口（见 bridge/native_bindings_generated.dart）：
//   getAllAlbumView()          → { "albumsList": [ AlbumViewInfo, ... ] }
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

  AlbumProvider({required this.songProvider}) {
    songProvider.addListener(_onSongProviderChanged);
  }

  List<AlbumViewInfo> _albums = [];

  /// 全部专辑（顺序 = 后端返回的顺序，各页面自己排）
  List<AlbumViewInfo> get albums => _albums;
  int get albumCount => _albums.length;

  /// 上一次同步时 SongProvider 的版本号；null = 从来没同步过
  int? _syncedRevision;

  void _onSongProviderChanged() {
    if (_syncedRevision == null) return;
    ensureSynced();
  }

  /// 让专辑列表跟上 SongProvider 的当前状态
  ///
  /// 幂等：revision 没变就什么都不做，所以可以放心重复调。
  ///
  /// [notify] 说明：页面在 initState 里做第一次同步时传 false。
  /// 因为那时候还在 build 阶段里，notifyListeners 会让 Provider 自己的
  /// InheritedElement 去 markNeedsBuild，debug 下直接报
  /// 「setState() or markNeedsBuild() called during build」。
  /// 页面自己的 build 紧接着就会跑，本来也不需要这一次广播。
  void ensureSynced({bool notify = true}) {
    if (_syncedRevision == songProvider.revision) return;
    _syncedRevision = songProvider.revision;
    refreshAlbums(notify: notify);
  }

  /// 无条件重新拉一遍专辑列表
  ///
  /// [notify] = false 时只换数据不广播（只有 initState 里的首次同步用得到）
  void refreshAlbums({bool notify = true}) {
    final obj =
        jsonDecode(UtilFunction.cPtr2String(bindings.getAllAlbumView()))
            as Map<String, dynamic>;
    final raw = obj[CoordinatorMacro.albumsList] as List<dynamic>? ?? const [];

    _albums = [
      for (final each in raw)
        AlbumViewInfo.fromBackendJson(each as Map<String, dynamic>),
    ];

    if (notify) notifyListeners();
  }

  /// 某张专辑下的 songId
  List<int> getSongIdListOfAlbum(int albumId) {
    final songIdList =
        (jsonDecode(
                  UtilFunction.cPtr2String(
                    bindings.getSongsListByAlbumId(albumId),
                  ),
                )?[CoordinatorMacro.songIdList]
                as List<dynamic>)
            .map((e) => e as int)
            .toList();

    return songIdList;
  }

  /// 某张专辑下的歌曲（SongProvider 里的实时数据）
  ///
  /// 查不到的 songId 直接跳过：歌曲被删掉、或者前端还没加载完的时候不至于崩。
  List<SongInfo> songsOfAlbum(int albumId) {
    final songs = <SongInfo>[];
    for (final songId in songIdsOfAlbum(albumId)) {
      final song = songProvider.getSongInfo(songId);
      if (song != null) songs.add(song);
    }
    return songs;
  }

  /// 专辑内歌曲的自然顺序：音轨号 → 歌名（升序）
  ///
  /// 没写音轨号的排在最后（不然后端给的默认 0/空值会插到音轨 1 前面）。
  ///
  /// FIXME: songs 表和 SongInfo 里都没有碟号（discNumber）字段，多碟专辑
  ///        只能靠音轨号排，做不到「先按碟、再按音轨」。等后端给 SongInfo
  ///        补上碟号之后，这里要改成 碟号 → 音轨号 → 歌名。
  static int compareByTrack(SongInfo a, SongInfo b) {
    final trackA = a.trackNumber;
    final trackB = b.trackNumber;
    if (trackA != null && trackB != null) {
      final result = trackA.compareTo(trackB);
      if (result != 0) return result;
    } else if (trackA != null) {
      return -1; // a 有音轨号、b 没有 → a 在前
    } else if (trackB != null) {
      return 1;
    }

    // 音轨号相同（或者都没有）的时候按歌名兜底，保证顺序稳定
    return AppCache.collator.compare(a.title, b.title);
  }

  @override
  void dispose() {
    songProvider.removeListener(_onSongProviderChanged);
    super.dispose();
  }
}
