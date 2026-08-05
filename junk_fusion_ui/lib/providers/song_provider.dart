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

  // 追加歌曲到列表末尾（用于导入等操作）
  //
  // Dart 语法：
  // - `addAll()` 把一个列表的所有元素追加到另一个列表末尾
  // - `...`（展开运算符）在 Dart 中不在列表字面量中使用，
  //   而是用 `addAll` 方法实现同样功能
  void addSongs(List<SongInfo> newSongs) {
    _songs.addAll(newSongs);
    notifyListeners();
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
