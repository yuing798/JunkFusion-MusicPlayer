/// 专辑的右键菜单（专辑详情页的「...」按钮也用同一份）
///
/// ── 为什么要单独一个文件 ────────────────────────────────────────────────
/// 专辑网格的卡片右键、单张专辑页的「...」按钮要做的事完全一样，
/// 抽成一个函数之后，两边都只管把「在哪个位置弹」算出来就行。
///
/// ── 用法 ─────────────────────────────────────────────────────────────
///
///     // 卡片右键
///     GestureDetector(
///       onSecondaryTapDown: (details) => showAlbumActionMenu(
///         context: context,
///         album: album,
///         position: details.globalPosition,
///       ),
///       child: ...,
///     )
///
///     // 「...」按钮：弹在按钮左下角
///     final box = buttonKey.currentContext!.findRenderObject() as RenderBox;
///     final offset = box.localToGlobal(Offset.zero);
///     showAlbumActionMenu(
///       context: context,
///       album: album,
///       position: Offset(offset.dx, offset.dy + box.size.height),
///     );
///
/// ── 菜单项 ───────────────────────────────────────────────────────────
///   播放全部              整张专辑替换播放列表并从第一首开始播
///   添加所有歌曲到播放列表  追加到当前播放列表（按 songId 去重，会提示加了几首）
///   联网搜索专辑封面       后端 iTunes 搜图 + 落盘 + 写 albums.hash
///                         网络请求会阻塞 UI，所以丢到 Isolate 里跑
library;

import 'dart:convert';
import 'dart:isolate';

import 'package:flutter/material.dart';
import 'package:flutter_root_context_menu/flutter_root_context_menu.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/model/album_basic_info.dart';
import 'package:junk_fusion_ui/model/song_basic_info.dart';
import 'package:junk_fusion_ui/providers/album_provider.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';
import 'package:junk_fusion_ui/widgets/popup_window.dart';
import 'package:provider/provider.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

/// 弹出专辑操作菜单
///
/// [position] 是**全局坐标**（右键点、或者按钮左下角）。
/// 菜单项内部会自己用 provider 干活，调用方不用再传回调。
void showAlbumActionMenu({
  required BuildContext context,
  required AlbumBasicInfo album,
  required Offset position,
}) {
  // 先把 provider 抓在手里：菜单回调是异步的，那时 widget 可能已经重建了
  final albumProvider = context.read<AlbumProvider>();
  final playback = context.read<PlaybackProvider>();
  final colorScheme = Theme.of(context).colorScheme;

  List<SongBasicInfo> albumSongs() => albumProvider.songsOfAlbum(album.albumId);

  showRootContextMenu(
    context: context,
    position: position,
    useBarrier: true,
    config: ContextMenuConfig(
      minWidth: 220,
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
      // ── 播放全部 ──
      ContextMenuItem(
        label: '播放全部',
        icon: Icon(TablerIcons.playerPlayFilled, size: 18, color: colorScheme.primary),
        onTap: () {
          final songs = albumSongs();
          if (songs.isEmpty) {
            DialogUtil.showInfoDialog('这张专辑里没有歌曲');
            return;
          }

          // 先设播放列表再切歌：setNewSong 内部会把当前列表缓存起来，
          // 反过来的话存下去的就是旧列表
          playback.setPlayList(songs);
          playback.setNewSong(songs.first.songId);
        },
      ),

      // ── 添加所有歌曲到播放列表 ──
      ContextMenuItem(
        label: '添加所有歌曲到播放列表',
        icon: Icon(TablerIcons.playlistAdd, size: 18),
        onTap: () {
          final songs = albumSongs();
          if (songs.isEmpty) {
            DialogUtil.showInfoDialog('这张专辑里没有歌曲');
            return;
          }

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
        },
      ),

      // ── 联网搜索专辑封面 ──
      ContextMenuItem(
        label: '联网搜索专辑封面',
        icon: Icon(TablerIcons.photoSearch, size: 18),
        onTap: () => _searchAlbumCover(
          album: album,
          albumProvider: albumProvider,
        ),
      ),
    ],
  );
}

/// 联网搜索封面
///
/// 后端是同步的 HTTP 请求（最长 15s），直接在主 isolate 调会把整个界面卡住，
/// 所以丢进 [Isolate.run]。新 isolate 会自己重新打开一次 DLL。
Future<void> _searchAlbumCover({
  required AlbumBasicInfo album,
  required AlbumProvider albumProvider,
}) async {
  String? newHash;
  try {
    newHash = await Isolate.run<String?>(() {
      final raw = UtilFunction.cPtr2String(
        bindings.searchAlbumCover(album.albumId),
      );
      final decoded = jsonDecode(raw);
      if (decoded is! Map) return null;
      final hash = decoded['hash'];
      return hash is String ? hash : null;
    });
  } catch (e) {
    print('联网搜索专辑封面失败: $e');
    await DialogUtil.showInfoDialog('联网搜索封面失败，请稍后再试');
    return;
  }

  if (newHash == null || newHash.isEmpty) {
    await DialogUtil.showInfoDialog('没有搜到「${album.title}」的封面');
    return;
  }

  // 本地更新这一张专辑，网格和详情页都会跟着换图
  albumProvider.addOrUpdateAlbum(album.copyWith(hash: newHash));
  await DialogUtil.showInfoDialog('已更新「${album.title}」的封面');
}
