import 'package:junk_fusion_ui/model/song_info.dart';

// ════════════════════════════════════════════════════════════════
// album_info.dart — 专辑数据模型
//
// 后端（JunkFusionCoordinator.dll）目前只吐歌曲列表，没有任何「专辑」维度
// 的接口，所以专辑信息全部是前端从 SongProvider.songs 里聚合出来的：
//   专辑名 + 专辑艺术家 相同的歌曲 → 归成同一张专辑
//
// 这个类刻意不写 freezed：它不需要 JSON 序列化，也就省得跑 build_runner。
//
// TODO: 等后端提供 getAllAlbums() / getAlbumSongs(albumId) 之类的接口后，
//       把 groupFromSongs() 换成解析后端 JSON，并在这个类里补上
//       albumId、专辑封面路径、发行公司这些字段。
// FIXME: 现在没有 albumId，只能拿字符串「专辑名 + 专辑艺术家」当唯一标识，
//        所以两个不同艺术家、且都没写 albumArtist 标签的同名专辑会被并成一张。
// ════════════════════════════════════════════════════════════════

/// AlbumInfo — 一张专辑的聚合结果
class AlbumInfo {
  /// 专辑名（SongInfo.album，为空时统一显示成「未知专辑」）
  final String name;

  /// 专辑艺术家（SongInfo.albumArtist，缺失时退化成第一首歌的艺术家）
  final String albumArtist;

  /// 这张专辑下的歌曲（已按 碟号 → 音轨号 → 歌名 排好序）
  final List<SongInfo> songs;

  /// 封面哈希：暂时借用专辑里第一首有封面的歌
  ///
  /// TODO: 后端把专辑封面单独缓存好之后（例如
  ///       ${AppCache.cacheDirString}/albumImage/<albumId>/original.jpg），
  ///       改成读专辑自己的封面，这个字段就可以删掉了。
  final String? coverHash;

  /// 聚合用的唯一键（专辑名 + 专辑艺术家），可以直接拿来当 ValueKey
  String get groupKey => _groupKey;
  final String _groupKey;

  // 私有构造函数：外部只能通过 groupFromSongs() 造专辑，
  // 免得有人手搓一个和聚合规则对不上的 AlbumInfo 出来
  AlbumInfo._({
    required this.name,
    required this.albumArtist,
    required this.songs,
    required this.coverHash,
    required String groupKey,
  }) : _groupKey = groupKey;

  /// 专辑里的歌曲数量
  int get songCount => songs.length;

  /// 专辑总时长（秒）
  double get totalDuration =>
      songs.fold<double>(0.0, (sum, song) => sum + song.duration);

  /// 年份：取专辑里最小的那个非空年份（同一张专辑的年份一般是一致的）
  int? get year {
    int? result;
    for (final song in songs) {
      final songYear = song.year;
      if (songYear == null || songYear <= 0) continue;
      if (result == null || songYear < result) result = songYear;
    }
    return result;
  }

  /// 某首歌是否属于这张专辑
  ///
  /// SingleAlbumPage 用它从 SongProvider 里实时筛歌：直接用构造时的 songs
  /// 快照的话，「我喜欢」之类的状态变化不会反映到列表上（SongInfo 是不可变的，
  /// Provider 里换的是列表元素）。
  bool contains(SongInfo song) => groupKeyOf(song) == _groupKey;

  // ────────────────────────────────────────────────────────────
  // 聚合相关
  // ────────────────────────────────────────────────────────────

  /// 专辑名缺失时的统一占位名
  static const String unknownAlbumName = '未知专辑';

  /// 艺术家缺失时的统一占位名
  static const String unknownArtistName = '未知艺术家';

  /// 聚合键：专辑名 + '\u0000' + 专辑艺术家
  ///
  /// 用 \u0000 当分隔符是为了避免「专辑名叫 A、艺术家叫 B」和
  /// 「专辑名叫 A B、艺术家为空」撞键。
  static String groupKeyOf(SongInfo song) {
    final album = _clean(song.album) ?? unknownAlbumName;
    final artist = _clean(song.albumArtist) ?? '';
    return '$album\u0000$artist';
  }

  /// 把歌曲列表聚合成专辑列表
  ///
  /// TODO: 后端能做聚合的话，这个函数整个换掉，前端不用每次都重算。
  static List<AlbumInfo> groupFromSongs(List<SongInfo> songs) {
    // Map 的 key 就是聚合键，value 是落进这张专辑的歌
    final Map<String, List<SongInfo>> buckets = <String, List<SongInfo>>{};

    for (final song in songs) {
      buckets.putIfAbsent(groupKeyOf(song), () => <SongInfo>[]).add(song);
    }

    final albums = <AlbumInfo>[];

    buckets.forEach((key, bucketSongs) {
      // 先按碟号/音轨号把专辑内部的顺序排好
      bucketSongs.sort(compareSongs);

      // 找第一首带封面的歌当专辑封面
      String? coverHash;
      for (final song in bucketSongs) {
        final hash = _clean(song.hash);
        if (hash != null) {
          coverHash = hash;
          break;
        }
      }
      albums.add(
        AlbumInfo._(
          // 同一张专辑里专辑名是一致的，取第一首即可
          name: _clean(bucketSongs.first.album) ?? unknownAlbumName,
          albumArtist: _artistOf(bucketSongs),
          // 复制一份，别把上面 buckets 里的列表引用直接暴露出去
          songs: List<SongInfo>.of(bucketSongs),
          coverHash: coverHash,
          groupKey: key,
        ),
      );
    });

    return albums;
  }

  /// 专辑内排序：碟号 → 音轨号 → 歌名
  ///
  /// 没有碟号的当成第 1 碟；有音轨号的排在没音轨号的前面。
  static int compareSongs(SongInfo a, SongInfo b) {
    final disc = (a.discNumber ?? 1).compareTo(b.discNumber ?? 1);
    if (disc != 0) return disc;

    final trackA = a.trackNumber;
    final trackB = b.trackNumber;
    if (trackA != null && trackB != null) {
      final track = trackA.compareTo(trackB);
      if (track != 0) return track;
    } else if (trackA != null) {
      return -1; // a 有音轨号，b 没有 → a 在前
    } else if (trackB != null) {
      return 1;
    }

    // 音轨号也没有（或者相同）的时候按歌名兜底，保证顺序稳定
    return a.title.compareTo(b.title);
  }

  /// 专辑艺术家的取值顺序：albumArtist → 第一首有艺术家的歌 → 未知
  static String _artistOf(List<SongInfo> songs) {
    for (final song in songs) {
      final albumArtist = _clean(song.albumArtist);
      if (albumArtist != null) return albumArtist;
    }
    for (final song in songs) {
      final artists = song.artists;
      if (artists != null && artists.isNotEmpty) {
        return artists.join(' / ');
      }
    }
    return unknownArtistName;
  }

  /// 去掉首尾空白，空字符串统一当成 null（元数据里挺常见 "" 和 null 混用）
  static String? _clean(String? value) {
    if (value == null) return null;
    final trimmed = value.trim();
    return trimmed.isEmpty ? null : trimmed;
  }
}
