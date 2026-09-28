import 'dart:io';
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/song_info.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:provider/provider.dart';
import 'package:window_manager/window_manager.dart';
import 'package:windows_taskbar/windows_taskbar.dart';

class WindowsTaskbarController extends StatefulWidget {
  const WindowsTaskbarController({super.key});

  @override
  State<WindowsTaskbarController> createState() =>
      _WindowsTaskbarControllerState();
}

class _WindowsTaskbarControllerState extends State<WindowsTaskbarController> {
  int? _lastSongId;
  bool? _lastIsPlaying;
  PlaybackProvider? _playbackProvider;

  @override
  void initState() {
    super.initState();

    // 确保只在 Windows 平台执行
    if (!Platform.isWindows) return;

    // 核心修复：必须等 Flutter 第一帧渲染完毕（Windows 窗口句柄创建完成）后再去调用系统 API
    WidgetsBinding.instance.addPostFrameCallback((_) {
      // 拿到 Provider 实例（这里用 read，不触发 build）
      _playbackProvider = context.read<PlaybackProvider>();

      // 添加监听器：只要数据变了，就触发 _sync
      _playbackProvider?.addListener(_onPlaybackChanged);

      // 初始化时主动同步一次
      _onPlaybackChanged();
    });
  }

  @override
  void dispose() {
    // 组件销毁时务必移除监听，防止内存泄漏
    _playbackProvider?.removeListener(_onPlaybackChanged);
    super.dispose();
  }

  // 这里的函数专门处理状态变化，完全脱离了 build 渲染管线
  void _onPlaybackChanged() {
    if (_playbackProvider == null) return;
    _sync(_playbackProvider!);
  }

  void _sync(PlaybackProvider playback) async {
    final song = playback.currentSong;

    if (song == null) {
      if (_lastSongId != null) {
        _lastSongId = null;
        _lastIsPlaying = null;
        WindowsTaskbar.resetThumbnailToolbar();

        // 【修改点 1】：改用 windowManager 安全地设置标题
        await windowManager.setTitle('Junk Fusion');
      }
      return;
    }

    final title = _formatTitle(song);

    if (song.songId != _lastSongId) {
      _lastSongId = song.songId;

      // 【修改点 2】：废弃 WindowsTaskbar 的标题方法，使用 windowManager
      await windowManager.setTitle(title);
    }

    if (song.songId != _lastSongId || playback.isPlaying != _lastIsPlaying) {
      _lastIsPlaying = playback.isPlaying;

      // 现在可以安全地解开按钮的注释了！
      WindowsTaskbar.setThumbnailToolbar(_buildButtons(playback));
    }
  }

  String _formatTitle(SongInfo song) {
    final artist = song.artists.isEmpty ? '未知' : song.artists.join(' / ');
    return '${song.title} - $artist';
  }

  List<ThumbnailToolbarButton> _buildButtons(PlaybackProvider playback) {
    final isPlaying = playback.isPlaying;
    return [
      ThumbnailToolbarButton(
        ThumbnailToolbarAssetIcon('assets/image/player-skip-back_24x24.ico'),
        '上一首',
        () => playback.playPreviousSong(),
      ),
      ThumbnailToolbarButton(
        ThumbnailToolbarAssetIcon(
          isPlaying
              ? 'assets/image/player-pause_24x24.ico'
              : 'assets/image/player-play_24x24.ico',
        ),
        isPlaying ? '暂停' : '播放',
        () => playback.togglePlayPause(),
      ),
      ThumbnailToolbarButton(
        ThumbnailToolbarAssetIcon('assets/image/player-skip-forward_24x24.ico'),
        '下一首',
        () => playback.playNextSong(),
      ),
    ];
  }

  @override
  Widget build(BuildContext context) {
    // build 必须是纯净的，只返回一个空的占位符
    return const SizedBox.shrink();
  }
}
