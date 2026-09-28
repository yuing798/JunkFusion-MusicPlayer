// import 'dart:io';
// import 'package:flutter/material.dart';
// import 'package:junk_fusion_ui/model/song_info.dart';
// import 'package:junk_fusion_ui/providers/playback_provider.dart';
// import 'package:provider/provider.dart';
// import 'package:window_manager/window_manager.dart';
// import 'package:windows_taskbar/windows_taskbar.dart';

// class WindowsTaskberManager {
//   int? _lastSongId;
//   bool? _lastIsPlaying;
//   final PlaybackProvider playbackProvider;

//   WindowsTaskberManager({required this.playbackProvider});

//   void initState() {
//     // 确保只在 Windows 平台执行
//     if (!Platform.isWindows) return;

//     WidgetsBinding.instance.addPostFrameCallback((_) {
//       // 添加监听器：只要数据变了，就触发 _sync
//       playbackProvider.addListener(_onPlaybackChanged);

//       // 初始化时主动同步一次
//       _onPlaybackChanged();
//     });
//   }

//   void dispose() {
//     // 组件销毁时务必移除监听，防止内存泄漏
//     playbackProvider.removeListener(_onPlaybackChanged);
//   }

//   // 这里的函数专门处理状态变化，完全脱离了 build 渲染管线
//   void _onPlaybackChanged() {
//     _sync();
//   }

//   void _sync() async {
//     final song = playback.currentSong;

//     if (song == null) {
//       if (_lastSongId != null) {
//         _lastSongId = null;
//         _lastIsPlaying = null;
//         WindowsTaskbar.resetThumbnailToolbar();

//         // 【修改点 1】：改用 windowManager 安全地设置标题
//         await windowManager.setTitle('Junk Fusion');
//       }
//       return;
//     }

//     final title = _formatTitle(song);

//     if (song.songId != _lastSongId) {
//       _lastSongId = song.songId;

//       // 【修改点 2】：废弃 WindowsTaskbar 的标题方法，使用 windowManager
//       await windowManager.setTitle(title);
//     }

//     if (song.songId != _lastSongId || playback.isPlaying != _lastIsPlaying) {
//       _lastIsPlaying = playback.isPlaying;

//       // 现在可以安全地解开按钮的注释了！
//       WindowsTaskbar.setThumbnailToolbar(_buildButtons(playback));
//     }
//   }

//   String _formatTitle(SongInfo song) {
//     final artist = song.artists.isEmpty ? '未知' : song.artists.join(' / ');
//     return '${song.title} - $artist';
//   }

// }
