// ════════════════════════════════════════════════════════════════
// windows_taskbar_controller.dart — Windows 任务栏缩略图视图
//
// 负责同步播放状态到 Windows 任务栏：
//   1. 任务栏按钮标题：`歌曲名 - 歌手`（setWindowTitle）
//   2. 缩略图底部三个图标：上一首 / 播放暂停 / 下一首（setThumbnailToolbar）
//   3. 鼠标悬停在缩略图上时显示完整标题，避免标题过长被省略号截断（setThumbnailTooltip）
//
// 本组件无任何可见 UI，仅挂载在 Widget Tree 中监听 PlaybackProvider 变化。
// ════════════════════════════════════════════════════════════════

import 'dart:io';

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/song_info.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:provider/provider.dart';
import 'package:windows_taskbar/windows_taskbar.dart';

/// WindowsTaskbarController — 无画布组件，负责把播放状态同步到 Windows 任务栏
class WindowsTaskbarController extends StatefulWidget {
  const WindowsTaskbarController({super.key});

  @override
  State<WindowsTaskbarController> createState() =>
      _WindowsTaskbarControllerState();
}

class _WindowsTaskbarControllerState extends State<WindowsTaskbarController> {
  // 缓存上一次同步的歌曲 ID 与播放状态，避免重复调用系统接口
  int? _lastSongId;
  bool? _lastIsPlaying;

  @override
  Widget build(BuildContext context) {
    // windows_taskbar 是 Windows 专用插件，其他平台直接跳过
    if (Platform.isWindows) {
      final playback = context.watch<PlaybackProvider>();
      _sync(playback);
    }
    return const SizedBox.shrink();
  }

  /// 把当前播放状态同步到任务栏
  void _sync(PlaybackProvider playback) {
    final song = playback.currentSong;

    // 无歌曲：清空任务栏上的歌曲信息
    if (song == null) {
      if (_lastSongId != null) {
        _lastSongId = null;
        _lastIsPlaying = null;
        WindowsTaskbar.resetThumbnailToolbar();
        WindowsTaskbar.setWindowTitle('Junk Fusion');
        WindowsTaskbar.setThumbnailTooltip('Junk Fusion');
      }
      return;
    }

    final title = _formatTitle(song);

    // 歌曲切换：更新标题与悬停提示
    if (song.songId != _lastSongId) {
      _lastSongId = song.songId;
      WindowsTaskbar.setWindowTitle(title);
      WindowsTaskbar.setThumbnailTooltip(title);
    }

    // 歌曲切换或播放状态变化：重建缩略图底部三个按钮
    if (song.songId != _lastSongId || playback.isPlaying != _lastIsPlaying) {
      _lastIsPlaying = playback.isPlaying;
      WindowsTaskbar.setThumbnailToolbar(_buildButtons(playback));
    }
  }

  /// 组装 `歌曲名 - 歌手` 标题
  String _formatTitle(SongInfo song) {
    final artist = song.artists.isEmpty ? '未知' : song.artists.join(' / ');
    return '${song.title} - $artist';
  }

  /// 构建缩略图底部三个按钮：上一首 / 播放暂停 / 下一首
  List<ThumbnailToolbarButton> _buildButtons(PlaybackProvider playback) {
    final isPlaying = playback.isPlaying;

    return [
      // 上一首
      ThumbnailToolbarButton(
        ThumbnailToolbarAssetIcon(
          'assets/image/player-skip-back_24x24.ico',
        ),
        '上一首',
        () => playback.playPreviousSong(),
      ),
      // 播放 / 暂停（随状态切换图标）
      ThumbnailToolbarButton(
        ThumbnailToolbarAssetIcon(
          isPlaying
              ? 'assets/image/player-pause_24x24.ico'
              : 'assets/image/player-play_24x24.ico',
        ),
        isPlaying ? '暂停' : '播放',
        () => playback.togglePlayPause(),
      ),
      // 下一首
      ThumbnailToolbarButton(
        ThumbnailToolbarAssetIcon(
          'assets/image/player-skip-forward_24x24.ico',
        ),
        '下一首',
        () => playback.playNextSong(),
      ),
    ];
  }
}
