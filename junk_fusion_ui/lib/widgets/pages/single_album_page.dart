import 'dart:io';

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/album_info.dart';
import 'package:junk_fusion_ui/model/song_info.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/utils.dart';
import 'package:junk_fusion_ui/widgets/combo_box.dart';
import 'package:junk_fusion_ui/widgets/each_song.dart';
import 'package:junk_fusion_ui/widgets/popup_window.dart';
import 'package:provider/provider.dart';
import 'package:smooth_scroll_multiplatform/smooth_scroll_multiplatform.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

// ════════════════════════════════════════════════════════════════
// single_album_page.dart — 单张专辑的内容页
//
// 由 AlbumViewPage 点卡片进来（也可以单独用，只要丢一张 AlbumInfo 进来）。
//
// 布局（自上而下）：
//   第 1 行（60px）：返回按钮 + 专辑封面 + 专辑名 +「艺术家 · 年份 · 共 N 首 · 总时长」
//   第 2 行（80px）：常用操作按钮（播放全部 / 随机播放 / 添加到播放列表 /
//                    喜欢全部 / 专辑信息）+ 右侧排序下拉框和升降序按钮
//   第 3 部分：歌曲列表（复用「所有音乐」页面的 EachSong，左侧补一列音轨号）
//
// 列表底部在有歌正在播时留 90px，免得被 PlayBar 糊住。
//
// 注意：列表里的歌是从 SongProvider 里实时筛出来的，不是直接用
// AlbumInfo.songs 那份快照 —— 这样「我喜欢」「播放次数」这些状态一变，
// 页面才会跟着刷新。
// ════════════════════════════════════════════════════════════════

/// 单张专辑内部的排序方式
enum AlbumSongSortMode {
  byTrack('音轨顺序', 0),
  byTitle('歌曲名称', 1),
  byDuration('歌曲时长', 2),
  byPlayTimes('播放次数', 3);

  final String label;
  final int value;

  const AlbumSongSortMode(this.label, this.value);
}

class SingleAlbumPage extends StatefulWidget {
  /// 要展示的专辑
  final AlbumInfo album;

  /// 返回专辑列表的回调；为 null 时右上角不显示返回按钮
  final VoidCallback? onBack;

  const SingleAlbumPage({super.key, required this.album, this.onBack});

  @override
  State<SingleAlbumPage> createState() => _SingleAlbumPageState();
}

class _SingleAlbumPageState extends State<SingleAlbumPage> {
  /// 当前选中的排序方式
  AlbumSongSortMode _selectedSort = AlbumSongSortMode.byTrack;

  /// 是否升序（true = 升序，false = 降序）
  bool _isAscending = true;

  @override
  void initState() {
    super.initState();

    _delayInit();
  }

  Future<void> _delayInit() async {
    final frontCache = AppCache.frontCacheRef;
    final sortValue = frontCache.getInt('albumSongSortWays') ?? 0;
    final isAscending = frontCache.getBool('albumSongIsAscending') ?? true;
    final sortMode = AlbumSongSortMode.values.firstWhere(
      (v) => v.value == sortValue,
      orElse: () => AlbumSongSortMode.byTrack,
    );

    if (!mounted) return;

    setState(() {
      _selectedSort = sortMode;
      _isAscending = isAscending;
    });
  }

  // ────────────────────────────────────────────────────────────
  // 取数据 / 排序
  // ────────────────────────────────────────────────────────────

  /// 从 SongProvider 里实时筛出这张专辑的歌，并按当前排序方式排好
  List<SongInfo> _getAlbumSongs(BuildContext context) {
    final songs = context.read<SongProvider>().songs;
    final result = <SongInfo>[
      for (final song in songs)
        if (widget.album.contains(song)) song,
    ];

    // 用 switch 表达式：枚举的每个分支都要写全，漏了编译不过
    int compare(SongInfo a, SongInfo b) => switch (_selectedSort) {
      // 碟号 → 音轨号 → 歌名，和元数据自带的顺序一致
      AlbumSongSortMode.byTrack => AlbumInfo.compareSongs(a, b),
      AlbumSongSortMode.byTitle => a.title.compareTo(b.title),
      AlbumSongSortMode.byDuration => a.duration.compareTo(b.duration),
      AlbumSongSortMode.byPlayTimes => a.playNum.compareTo(b.playNum),
    };

    result.sort((a, b) => _isAscending ? compare(a, b) : compare(b, a));
    return result;
  }

  /// 标题下面那行小字：「艺术家 · 2020年 · 共 12 首 · 总时长 48:12」
  String _buildMetaLine(List<SongInfo> songs) {
    final parts = <String>[widget.album.albumArtist];

    final year = widget.album.year;
    if (year != null) parts.add('$year年');

    parts.add('共 ${songs.length} 首');

    if (songs.isNotEmpty) {
      final totalDuration = songs.fold<double>(
        0.0,
        (sum, song) => sum + song.duration,
      );
      parts.add('总时长 ${UtilFunction.formatDuration(totalDuration)}');
    }

    return parts.join(' · ');
  }

  // ────────────────────────────────────────────────────────────
  // 常用操作
  // ────────────────────────────────────────────────────────────

  /// 播放全部：按当前列表顺序整张专辑塞进播放列表，从第一首开始放
  void _playAll(List<SongInfo> songs) {
    if (songs.isEmpty) return;

    final playback = context.read<PlaybackProvider>();

    // 先设置播放列表再切歌：setNewSong 内部会把「当前播放列表」缓存到本地，
    // 反过来的话存下去的就是旧列表了
    playback.setPlayList(songs);

    final firstSongId = songs.first.songId;
    if (playback.currentSong?.songId == firstSongId) {
      // 已经停在这首歌上了：setNewSong 会直接 return，这里手动补一下播放状态
      if (!playback.isPlaying) playback.togglePlayPause();
    } else {
      playback.setNewSong(firstSongId);
    }
  }

  /// 随机播放：把专辑打乱之后当成播放列表
  void _shufflePlay(List<SongInfo> songs) {
    if (songs.isEmpty) return;

    final playback = context.read<PlaybackProvider>();
    final shuffled = List<SongInfo>.of(songs)..shuffle();

    // TODO: PlaybackProvider 只有 cyclePlayMode()（0→1→2→3 循环切），
    //       没有「直接设置成随机播放」的接口，所以这里只是打乱了播放列表的顺序，
    //       后端的播放模式并没有真的变成随机。等 Provider 加了 setPlayMode() 再补。
    playback.setPlayList(shuffled);
    playback.setNewSong(shuffled.first.songId);
  }

  /// 添加到播放列表：把专辑里的歌追加到现有播放列表后面（按 songId 去重）
  void _appendToPlayList(List<SongInfo> songs) {
    if (songs.isEmpty) return;

    final playback = context.read<PlaybackProvider>();
    final currentList = playback.playList;

    final existingIds = currentList.map((song) => song.songId).toSet();
    final merged = List<SongInfo>.of(currentList);
    for (final song in songs) {
      // Set.add 返回 false 说明这首歌已经在列表里了
      if (existingIds.add(song.songId)) merged.add(song);
    }

    final addedCount = merged.length - currentList.length;

    // TODO: 后端没有「往播放队列里追加歌曲」的接口，
    //       这里是前端整表替换；等后端接管播放队列之后改成调用后端接口。
    playback.setPlayList(merged);

    DialogUtil.showInfoDialog(
      addedCount == 0
          ? '这张专辑的歌曲都已经在播放列表里了'
          : '已把 $addedCount 首歌曲添加到播放列表',
    );
  }

  /// 喜欢 / 取消喜欢整张专辑
  void _toggleLikeAll(List<SongInfo> songs) {
    if (songs.isEmpty) return;

    final songProvider = context.read<SongProvider>();
    // 已经全喜欢了 → 这一下把所有歌取消喜欢；否则把没喜欢的补齐
    final allLiked = songs.every((song) => song.isMyLike);

    // TODO: 后端只有单曲的 toggleMyLike(songId)，没有批量接口，
    //       这里是一首一首调的（Provider 内部已经做了乐观更新 + 失败回滚）。
    //       等后端加了批量接口再换成一次调用。
    for (final song in songs) {
      if (song.isMyLike == allLiked) {
        songProvider.toggleMyLike(song.songId);
      }
    }
  }

  /// 专辑信息弹窗
  void _showAlbumInfo(List<SongInfo> songs) {
    final album = widget.album;

    final buffer = StringBuffer()
      ..writeln('专辑名：${album.name}')
      ..writeln('专辑艺术家：${album.albumArtist}');

    final year = album.year;
    if (year != null) buffer.writeln('年份：$year年');

    buffer.writeln('歌曲数量：${songs.length} 首');

    if (songs.isNotEmpty) {
      final totalDuration = songs.fold<double>(
        0.0,
        (sum, song) => sum + song.duration,
      );
      buffer.writeln('总时长：${UtilFunction.formatDuration(totalDuration)}');
    }

    // FIXME: 后端还没有专辑维度的接口，上面这些信息全是前端从歌曲元数据里拼的，
    //        所以像专辑简介、唱片公司、专辑封面路径这些字段暂时没得显示。
    DialogUtil.showInfoDialog(buffer.toString());
  }

  // ────────────────────────────────────────────────────────────
  // 工具栏辅助
  // ────────────────────────────────────────────────────────────

  // 切换升降序
  void _toggleAscending() {
    setState(() {
      _isAscending = !_isAscending;
    });

    AppCache.frontCacheRef.setBool('albumSongIsAscending', _isAscending);
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();

    // watch：歌曲元数据变了（比如切换「我喜欢」）列表要跟着重画
    context.watch<SongProvider>();
    final albumSongs = _getAlbumSongs(context);

    final isAllLiked =
        albumSongs.isNotEmpty && albumSongs.every((song) => song.isMyLike);
    final hasCurrentSong = context.select<PlaybackProvider, bool>(
      (provider) => provider.currentSong != null,
    );

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // ── 第 1 行：返回 + 封面 + 专辑名 ──
        SizedBox(
          height: 60,
          child: Row(
            children: [
              if (widget.onBack != null)
                IconButton(
                  icon: const Icon(TablerIcons.arrowLeft, size: 32),
                  tooltip: '返回专辑列表',
                  onPressed: widget.onBack,
                ),

              // 封面（小图，和大图共用 AlbumCoverImage）
              SizedBox(
                width: 50,
                height: 50,
                child: AlbumCoverImage(hash: widget.album.coverHash),
              ),

              const SizedBox(width: 10),

              // 专辑名 + 一行元信息
              Expanded(
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Tooltip(
                      message: widget.album.name,
                      child: Text(
                        widget.album.name,
                        style: theme.bigTextStyle,
                        maxLines: 1,
                        overflow: TextOverflow.ellipsis,
                      ),
                    ),
                    const SizedBox(height: 2),
                    Text(
                      _buildMetaLine(albumSongs),
                      style: theme.littleTextStyle,
                      maxLines: 1,
                      overflow: TextOverflow.ellipsis,
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),

        // ── 第 2 行：常用按钮 + 排序 ──
        SizedBox(
          height: 80,
          child: Row(
            children: [
              // ── 播放全部（主按钮，用强调色） ──
              _AlbumActionButton(
                icon: TablerIcons.playerPlayFilled,
                label: '播放全部',
                isPrimary: true,
                onTap: albumSongs.isEmpty ? null : () => _playAll(albumSongs),
              ),

              const SizedBox(width: 10),

              // ── 随机播放 ──
              _AlbumActionButton(
                icon: TablerIcons.arrowsShuffle2,
                label: '随机播放',
                onTap: albumSongs.isEmpty ? null : () => _shufflePlay(albumSongs),
              ),

              const SizedBox(width: 10),

              // ── 添加到播放列表 ──
              _AlbumActionButton(
                icon: TablerIcons.playlistAdd,
                label: '添加到播放列表',
                onTap: albumSongs.isEmpty
                    ? null
                    : () => _appendToPlayList(albumSongs),
              ),

              const SizedBox(width: 10),

              // ── 喜欢 / 取消喜欢整张专辑 ──
              _AlbumActionButton(
                icon: isAllLiked ? TablerIcons.heartFilled : TablerIcons.heart,
                label: isAllLiked ? '取消喜欢' : '喜欢全部',
                onTap: albumSongs.isEmpty
                    ? null
                    : () => _toggleLikeAll(albumSongs),
              ),

              // 弹性空间（把右侧的排序控件推到最右边）
              const Spacer(),

              // ── 专辑信息 ──
              IconButton(
                icon: const Icon(TablerIcons.infoCircle, size: 28),
                tooltip: '专辑信息',
                color: theme.colorTextMain,
                onPressed: () => _showAlbumInfo(albumSongs),
              ),

              const SizedBox(width: 10),

              // ── 排序方式下拉框 ──
              SizedBox(
                width: 180,
                child: ComboBox<AlbumSongSortMode>(
                  value: _selectedSort,
                  items: AlbumSongSortMode.values,
                  itemBuilder: (mode) =>
                      Text(mode.label, style: theme.comboTextStyle),
                  onChanged: (mode) async {
                    setState(() {
                      _selectedSort = mode;
                    });
                    final frontCache = AppCache.frontCacheRef;
                    frontCache.setInt('albumSongSortWays', mode.value);
                  },
                ),
              ),

              const SizedBox(width: 8),

              // ── 升降序切换按钮 ──
              IconButton(
                icon: Icon(
                  _isAscending
                      ? TablerIcons.arrowBigDownLineFilled
                      : TablerIcons.arrowBigUpLineFilled,
                  size: 32,
                ),
                onPressed: _toggleAscending,
                tooltip: _isAscending ? '升序' : '降序',
                color: theme.colorTextMain,
              ),
            ],
          ),
        ),

        // ── 第 3 部分：歌曲列表 ──
        Expanded(
          child: albumSongs.isEmpty
              ? Center(
                  child: Text('这张专辑里没有歌曲', style: theme.littleTextStyle),
                )
              : DynMouseScroll(
                  builder: (context_, controler_, physics_) {
                    return ListView.builder(
                      controller: controler_,
                      physics: physics_,
                      itemCount: albumSongs.length,
                      // 固定行高（和 EachSong 的 80 对齐，性能优化）
                      itemExtent: 80,
                      padding: EdgeInsets.only(bottom: hasCurrentSong ? 90 : 0),
                      itemBuilder: (context, index) {
                        final song = albumSongs[index];

                        return Row(
                          key: ValueKey(song.songId),
                          children: [
                            // ── 音轨号 ──
                            // FIXME: 多碟专辑现在是「按碟号→音轨号」排完一条龙显示，
                            //        音轨号会在碟与碟之间重复，还没有插「碟 x」分隔行。
                            SizedBox(
                              width: 36,
                              child: Center(
                                child: Text(
                                  song.trackNumber?.toString() ??
                                      '${index + 1}',
                                  style: theme.littleTextStyle,
                                ),
                              ),
                            ),

                            // ── 歌曲行（直接复用「所有音乐」页面的 EachSong） ──
                            Expanded(
                              child: EachSong(
                                song: song,
                                onSongPlay: () {
                                  // 点封面播放时，把「整张专辑」当成播放列表
                                  final playback = context
                                      .read<PlaybackProvider>();
                                  playback.setPlayList(albumSongs);
                                  // EachSong 是「先切歌 → 再回调」的，
                                  // 切歌那一步已经缓存过旧列表了，这里补存一次，
                                  // 保证下次启动恢复的是这张专辑
                                  playback.saveState();
                                },
                              ),
                            ),
                          ],
                        );
                      },
                    );
                  },
                ),
        ),
      ],
    );
  }
}

// ════════════════════════════════════════════════════════════════
// _AlbumActionButton — 专辑页工具栏上的按钮
//
// 样式和「所有音乐」页面的「导入文件」按钮保持一致：
// 直角矩形、悬浮色背景、midTextStyle 文字，只是多了个图标。
// ════════════════════════════════════════════════════════════════
class _AlbumActionButton extends StatelessWidget {
  final IconData icon;
  final String label;
  final VoidCallback? onTap;

  /// 主按钮（播放全部）用强调色背景，其他的用悬浮色
  final bool isPrimary;

  const _AlbumActionButton({
    required this.icon,
    required this.label,
    this.onTap,
    this.isPrimary = false,
  });

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();

    return SizedBox(
      height: 40,
      child: TextButton(
        // onTap 为 null 时 TextButton 自动进入禁用态（专辑没歌的时候用得上）
        onPressed: onTap,
        style: TextButton.styleFrom(
          backgroundColor: isPrimary ? theme.colorStress : theme.colorHover,
          foregroundColor: theme.colorTextMain,
          shape: const RoundedRectangleBorder(
            borderRadius: BorderRadius.zero, // 直角矩形
          ),
          padding: const EdgeInsets.symmetric(horizontal: 12),
        ),
        child: Row(
          // mainAxisSize.min：宽度 = 图标 + 文字，不撑满父级
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(icon, size: 20),
            const SizedBox(width: 6),
            Text(label, style: theme.midTextStyle),
          ],
        ),
      ),
    );
  }
}

// ════════════════════════════════════════════════════════════════
// AlbumCoverImage — 专辑封面
//
// 专辑封面目前没有独立缓存，先借用专辑里第一首歌的封面，路径和
// EachSong / PlayBar 里的歌曲封面保持一致：
//   ${AppCache.cacheDirString}/songImage/<hash>/original.jpg
//
// TODO: 等后端把专辑封面单独缓存成
//       ${AppCache.cacheDirString}/albumImage/<albumId>/original.jpg
//       之后，这里改成读专辑封面（并且删掉 hash 参数）。
// ════════════════════════════════════════════════════════════════
class AlbumCoverImage extends StatelessWidget {
  /// 封面哈希（来自 SongInfo.hash）
  final String? hash;

  const AlbumCoverImage({super.key, required this.hash});

  @override
  Widget build(BuildContext context) {
    final coverHash = hash;
    final placeholder = Image.asset(
      'assets/image/JunkFusion.png',
      fit: BoxFit.cover,
    );

    if (coverHash == null || coverHash.isEmpty) return placeholder;

    return Image.file(
      File('${AppCache.cacheDirString}/songImage/$coverHash/original.jpg'),
      fit: BoxFit.cover,
      // 缩略图用中等插值就够了，比默认的 low 好看不少
      filterQuality: FilterQuality.medium,
      // 缓存文件被删了/解码失败时别让界面炸成红框，退回默认图
      errorBuilder: (_, _, _) => placeholder,
    );
  }
}
