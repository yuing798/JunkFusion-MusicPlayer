import 'dart:io';

import 'package:file_picker/file_picker.dart';
import 'package:flutter/foundation.dart';
import 'package:junk_fusion_ui/bridge/dll/dllBridgeName.dart';
import 'package:junk_fusion_ui/bridge/dll/dll_invoke.dart';
import 'package:junk_fusion_ui/model/song_info.dart';
import 'package:junk_fusion_ui/widgets/info_window.dart';

// ════════════════════════════════════════════════════════════════
// song_provider.dart — 歌曲列表状态管理
// ════════════════════════════════════════════════════════════════
class SongProvider extends ChangeNotifier {
  List<SongInfo> _songs = [];

  List<SongInfo> get songs => _songs;

  // 按 ID 查找歌曲
  SongInfo? getSongInfo(int songId) {
    return _songs.where((s) => s.songId == songId).firstOrNull;
  }

  int get songCount => _songs.length;

  Future<void> getAllSongs() async {
    final results = await sendDLLIsolateTask(B_getAllSongs.name, {});
    _songs = results[B_getAllSongs.songsList] as List<SongInfo>;
    notifyListeners();
  }

  Future<void> toggleMyLike(int songId) async {
    // 找到目标歌曲在列表中的索引
    final index = _songs.indexWhere((s) => s.songId == songId);
    if (index == -1) return; // 没找到，直接返回

    // 保存原始状态，用于失败时回滚
    final originSong = _songs[index];

    // --- 乐观更新：立即翻转 UI 状态 ---
    _songs[index] = originSong.copyWith(isMyLike: !originSong.isMyLike);
    notifyListeners(); // 通知 UI 刷新（UI 立刻看到变化）

    // --- 异步调用后端 ---
    final results = await sendDLLIsolateTask(B_toggleMyLike.name, {
      B_toggleMyLike.songId: songId,
    });
    final successOrError = results[B_toggleMyLike.successOrError] as bool;
    if (!successOrError) {
      InfoWindow.show("切换我喜欢状态发生错误，请重试");
      // --- 失败回滚：恢复原始状态 ---
      _songs[index] = originSong;
      notifyListeners(); // 通知 UI 刷新回原始状态
    }
  }

  // 导入歌曲
  Future<void> songsImport() async {
    // 调用 pickFiles，传入配置参数
    FilePickerResult? result = await FilePicker.pickFiles(
      dialogTitle: '请选择音视频文件',
      initialDirectory: _getExeDirectory(),
      allowMultiple: true,
      lockParentWindow: true,
      // cancelUploadOnWindowBlur: false, //失焦时自动取消导入窗口，这个只在web中有用
    );

    if (result != null) {
      // 获取选中的文件路径（绝对路径）
      List<String> filePaths = result.files
          .map((e) => e.path) // 转换为 Iterable<String?>
          .whereType<String>() // 过滤掉 null，转换为 Iterable<String>
          .toList(); // 转为 List<String>

      // 接下来，把这个 filePath 通过 Isolate 或直接传给 DLL
      // sendTask('processAudioFile', {'path': filePath});
      final results = await sendDLLIsolateTask(B_songImport.name, {
        B_songImport.filePaths: filePaths,
      });
      final errorFiles = results[B_songImport.errorFiles] as List<String>;
      final songsList = results[B_songImport.songs] as List<SongInfo>;
      _songs.addAll(songsList);
      if (errorFiles.isEmpty) {
        InfoWindow.show("全部歌曲导入成功，总计${songsList.length}首歌曲", 3000);
      } else {
        final buffer = StringBuffer()
          ..write(
            '歌曲导入完成，总共导入${songsList.length + errorFiles.length}首，成功${songsList.length}首\n失败文件:\n',
          );

        for (final file in errorFiles) {
          buffer.writeln(file); // writeln 会自动加上换行符
        }

        final message = buffer.toString();
        InfoWindow.show(message, 30000);
      }
      notifyListeners();
    }
  }

  String _getExeDirectory() {
    // 1. 获取 exe 的绝对路径（解析符号链接）
    final String exePath = Platform.resolvedExecutable;
    // 2. 获取该文件所在的父级目录
    final Directory exeDir = File(exePath).parent;
    return exeDir.path;
  }

  // 将秒数格式化为 分:秒 或 时:分:秒 的可读字符串
  //可选：是否显示毫秒数
  static String formatDuration(double seconds, [bool needMs = false]) {
    if (seconds < 0) return '0:00';

    // 1. 分离整数部分（秒）和小数部分（毫秒）
    final totalSecs = seconds.floor(); // 向下取整，得到整秒
    final milliseconds = ((seconds - totalSecs) * 1000).round();

    // 2. 计算 时/分/秒
    final hours = totalSecs ~/ 3600;
    final minutes = (totalSecs % 3600) ~/ 60;
    final secs = totalSecs % 60;

    // 3. 拼接主要字符串
    String result;
    if (hours > 0) {
      result =
          '$hours:${minutes.toString().padLeft(2, '0')}:${secs.toString().padLeft(2, '0')}';
    } else {
      result = '$minutes:${secs.toString().padLeft(2, '0')}';
    }
    if (needMs) {
      result = '$result.${milliseconds.toString().padLeft(3, '0')}';
    }
    return result;
  }
}

// ════════════════════════════════════════════════════════════════
// Provider 使用模式总结：
//
// ## 注入 Provider 到 Widget Tree（在 main.dart 中）：
// ```dart
// MultiProvider(
//   providers: [
//     ChangeNotifierProvider(create: (_) => SongProvider()),
//     ChangeNotifierProvider(create: (_) => PlaybackProvider()),
//   ],
//   child: const MyApp(),
// )
// ```
//
// ## 在 Widget 中使用：
//
// ### 方式 1: context.watch (推荐，自动监听)
// ```dart
// @override
// Widget build(BuildContext context) {
//   final songs = context.watch<SongProvider>().songs;
//   return ListView(...);
// }
// ```
//
// ### 方式 2: context.read (不监听，只调用方法)
// ```dart
// ElevatedButton(
//   onPressed: () {
//     context.read<SongProvider>().getAllSongs();
//   },
//   child: Text('刷新'),
// )
// ```
//
// ### 方式 3: Consumer widget (局部重建)
// ```dart
// Consumer<SongProvider>(
//   builder: (context, provider, child) {
//     return Text('共 ${provider.songs.length} 首');
//   },
// )
// ```
