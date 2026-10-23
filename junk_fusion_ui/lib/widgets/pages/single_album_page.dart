import 'dart:io';

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/album_detail_info.dart';
import 'package:junk_fusion_ui/model/song_basic_info.dart';
import 'package:junk_fusion_ui/providers/album_provider.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/app_cache.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';
import 'package:junk_fusion_ui/widgets/combo_box.dart';
import 'package:junk_fusion_ui/widgets/each_song.dart';
import 'package:junk_fusion_ui/widgets/popup_window.dart';
import 'package:provider/provider.dart';
import 'package:smooth_scroll_multiplatform/smooth_scroll_multiplatform.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

// ════════════════════════════════════════════════════════════════
// single_album_page.dart — 单张专辑的内容页
//
// 由 AlbumViewPage 点卡片进来：专辑页会先 AlbumProvider.setNewAlbum()，
// 再往路由栈里压一条单张专辑页记录，所以这一页自己不接专辑参数，
// 直接读 AlbumProvider 里的 curBasicInfo / songsInAlbums。
// 返回交给路由左上角那个按钮（本页不再自带返回箭头）。
//
// 布局（自上而下）：
//   第 1 行（60px）：专辑封面 + 专辑名 +「艺术家 · 共 N 首」
//   第 2 行（80px）：常用操作按钮（播放全部 / 随机播放 / 添加到播放列表 /
//                    喜欢全部 / 专辑信息）+ 右侧排序下拉框和升降序按钮
//   第 3 部分：歌曲列表（复用「所有音乐」页面的 EachSong，左侧补一列音轨号）
//
// 数据来源：AlbumProvider（后端 getSongsListByAlbumId 给出这张专辑有哪些歌），
// 但列表里的 SongInfo 是用 songId 回 SongProvider 取的实时数据，不是快照 ——
// 这样「我喜欢」「播放次数」这些状态一变，页面才会跟着刷新。
//
// 列表底部在有歌正在播时留 90px，免得被 PlayBar 糊住。
// ════════════════════════════════════════════════════════════════

class SingleAlbumPage extends StatefulWidget {
  const SingleAlbumPage({super.key});

  @override
  State<SingleAlbumPage> createState() => _SingleAlbumPageState();
}

class _SingleAlbumPageState extends State<SingleAlbumPage> {
  /// 播放全部：按当前列表顺序整张专辑塞进播放列表，从第一首开始放
  void _playAll() {
    final playback = context.read<PlaybackProvider>();
    final albumP = context.read<AlbumProvider>();

    // print(123);
    // print(playback.playList);
    // print("\n");
    // print(albumP.songsInAlbums);
    if (playback.playList != albumP.songsInAlbums) {
      // 先设置播放列表再切歌：setNewSong 内部会把「当前播放列表」缓存到本地，

      // 反过来的话存下去的就是旧列表了
      // print("准备设置播放全部歌曲");
      playback.setPlayList(albumP.songsInAlbums);
      playback.setNewSong(playback.playList.first.songId);
    }
  }

  /// 添加到播放列表：把专辑里的歌追加到现有播放列表后面（按 songId 去重）
  void _appendToPlayList(List<SongBasicInfo> songs) {
    if (songs.isEmpty) return;

    final playback = context.read<PlaybackProvider>();
    final currentList = playback.playList;

    final existingIds = currentList.map((song) => song.songId).toSet();
    final merged = List<SongBasicInfo>.of(currentList);
    for (final song in songs) {
      // Set.add 返回 false 说明这首歌已经在列表里了
      if (existingIds.add(song.songId)) merged.add(song);
    }

    final addedCount = merged.length - currentList.length;

    playback.setPlayList(merged);

    DialogUtil.showInfoDialog(
      addedCount == 0 ? '这张专辑的歌曲都已经在播放列表里了' : '已把 $addedCount 首歌曲添加到播放列表',
    );
  }

  /// 标题下面那行小字：「艺术家 · 共 12 首 · 总时长 48:12」
  String _buildMetaLine() {
    final albumP = context.read<AlbumProvider>();
    if (albumP.curBasicInfo == null) return "";
    final parts = <String>[];
    parts.add(UtilFunction.buildArtistsText(albumP.curBasicInfo!.albumArtists));

    parts.add('共 ${albumP.songCount} 首');

    return parts.join(' · ');
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();

    // watch：切换「我喜欢」之类的歌曲状态变了，列表要跟着重画
    context.watch<SongProvider>();
    final albumProvider = context.watch<AlbumProvider>();
    final albumSongs = albumProvider.songsInAlbums;

    final basicInfo = albumProvider.curBasicInfo;

    if (basicInfo == null) {
      return const SizedBox.shrink();
    }

    final hasCurrentSong = context.select<PlaybackProvider, bool>(
      (provider) => provider.curBasicInfo != null,
    );

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // ── 第 1 行：封面 + 专辑名 ──
        SizedBox(
          height: 60,
          child: Row(
            children: [
              // 封面（小图，和大图共用 AlbumCoverImage）
              SizedBox(
                width: 50,
                height: 50,
                child: AlbumCoverImage(hash: albumProvider.curBasicInfo?.hash),
              ),

              const SizedBox(width: 10),

              // 专辑名 + 一行元信息
              Expanded(
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Tooltip(
                      message: albumProvider.curBasicInfo!.title,
                      child: Text(
                        albumProvider.curBasicInfo!.title,
                        style: theme.bigTextStyle,
                        maxLines: 1,
                        overflow: TextOverflow.ellipsis,
                      ),
                    ),
                    const SizedBox(height: 2),
                    Text(
                      _buildMetaLine(),
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
                onTap: () {
                  // if (albumSongs.isNotEmpty) {
                  //   _playAll();
                  // }
                  // print(albumSongs);
                  _playAll();
                },
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

              // 弹性空间（把右侧的排序控件推到最右边）
              const Spacer(),

              const SizedBox(width: 10),
            ],
          ),
        ),

        // ── 第 3 部分：歌曲列表 ──
        Expanded(
          child: albumSongs.isEmpty
              ? Center(child: Text('这张专辑里没有歌曲', style: theme.littleTextStyle))
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
// 后端 albums 表里的 hash 就是这张专辑的封面哈希（Model/AlbumInfo.cpp 的
// albumViewtoJson() 吐出来的那个 hash），封面文件跟歌曲封面共用一份缓存：
//   ${AppCache.cacheDirString}/image/<hash>/original.jpg
// 所以不需要单独的专辑封面目录；hash 为空（后端存的是 ""，比如专辑里的歌
// 都没有封面）时退回默认图。
// ════════════════════════════════════════════════════════════════
class AlbumCoverImage extends StatelessWidget {
  /// 封面哈希（来自 AlbumViewInfo.hash）
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
      File('${AppCache.cacheDirString}/image/$coverHash/original.jpg'),
      fit: BoxFit.cover,
      // 缓存文件被删了/解码失败时别让界面炸成红框，退回默认图
      errorBuilder: (_, _, _) => placeholder,
    );
  }
}
