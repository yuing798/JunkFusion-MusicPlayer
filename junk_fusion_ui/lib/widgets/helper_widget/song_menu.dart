/// 歌曲行的右键菜单
///
/// ── 用途 ─────────────────────────────────────────────────────────────
/// EachSong（以及以后任何展示单首歌的地方）右键时弹出的菜单：
///
///     播放                 → 切到这首（先让页面处理播放列表，再切歌）
///     添加到播放列表        → 追加到当前播放队列（按 songId 去重）
///     收藏到歌单            → 弹「收藏到歌单」窗口（favorite_playlist.dart）
///     喜欢 / 取消喜欢       → 切换 isMyLike
///
/// ── 用法 ─────────────────────────────────────────────────────────────
///
///     GestureDetector(
///       onSecondaryTapDown: (details) => showSongActionMenu(
///         context: context,
///         song: song,
///         position: details.globalPosition,
///         onBeforePlay: onSongPlay,   // 页面自己的「先设播放列表」逻辑，可以不传
///       ),
///       child: ...,
///     )
///
/// 「喜欢」的文字跟着 song.isMyLike 变，所以菜单在弹出那一刻就是最新的状态。
library;

import 'package:flutter/material.dart';
import 'package:flutter_root_context_menu/flutter_root_context_menu.dart';
import 'package:junk_fusion_ui/model/song_basic_info.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/widgets/popupWindow/favorite_playlist.dart';
import 'package:junk_fusion_ui/widgets/popup_window.dart';
import 'package:provider/provider.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

/// 弹出单曲操作菜单
void showSongActionMenu({
  required BuildContext context,
  required SongBasicInfo song,
  required Offset position,
  VoidCallback? onBeforePlay,
}) {
  // 菜单回调是异步的，先把 provider 抓在手里
  final playback = context.read<PlaybackProvider>();
  final songProvider = context.read<SongProvider>();
  final colorScheme = Theme.of(context).colorScheme;

  showRootContextMenu(
    context: context,
    position: position,
    useBarrier: true,
    config: ContextMenuConfig(
      minWidth: 190,
      itemHeight: 40,
      borderRadius: BorderRadius.circular(8),
      elevation: 8,
      animationDuration: const Duration(milliseconds: 120),
      animationBuilder: ContextMenuAnimations.fade,
      backgroundColor: colorScheme.surface,
      hoverColor: colorScheme.primaryContainer,
      screenPadding: const EdgeInsets.all(8),
    ),
    items: [
      // ── 播放 ──
      ContextMenuItem(
        label: '播放',
        icon: Icon(
          TablerIcons.playerPlayFilled,
          size: 18,
          color: colorScheme.primary,
        ),
        onTap: () {
          // 和点封面的顺序保持一致：先切歌，再让页面去调整播放列表
          playback.setNewSong(song.songId);
          onBeforePlay?.call();
        },
      ),

      // ── 添加到播放列表 ──
      ContextMenuItem(
        label: '添加到播放列表',
        icon: Icon(TablerIcons.playlistAdd, size: 18),
        onTap: () {
          final currentList = playback.playList;
          if (currentList.any((each) => each.songId == song.songId)) {
            DialogUtil.showInfoDialog('这首歌已经在播放列表里了');
            return;
          }

          playback.setPlayList([...currentList, song]);
          DialogUtil.showInfoDialog('已把「${song.title}」添加到播放列表');
        },
      ),

      // ── 收藏到歌单 ──
      ContextMenuItem(
        label: '收藏到歌单',
        icon: Icon(TablerIcons.playlistAdd, size: 18),
        onTap: () => showFavoritePlaylistPicker(
          context: context,
          songIds: [song.songId],
        ),
      ),

      // ── 喜欢 / 取消喜欢 ──
      ContextMenuItem(
        label: song.isMyLike ? '取消喜欢' : '喜欢',
        icon: Icon(
          song.isMyLike ? TablerIcons.heartFilled : TablerIcons.heart,
          size: 18,
          color: song.isMyLike ? Colors.red : null,
        ),
        onTap: () => songProvider.toggleMyLike(song.songId),
      ),
    ],
  );
}
