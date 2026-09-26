import 'dart:convert';
import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';
import 'package:file_picker/file_picker.dart';
import 'package:flutter/foundation.dart';
import 'package:junk_fusion_ui/Macro/coordinatorMacro.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/bridge/dll_invoke.dart';
import 'package:junk_fusion_ui/model/song_info.dart';
import 'package:junk_fusion_ui/utils/utils.dart';
import 'package:junk_fusion_ui/widgets/popup_window.dart';

// ════════════════════════════════════════════════════════════════
// song_provider.dart — 歌曲列表状态管理
// ════════════════════════════════════════════════════════════════
class SongProvider extends ChangeNotifier {
  List<SongInfo> _songs = [];

  List<SongInfo> get songs => _songs;
  int get songCount => _songs.length;

  // 按 ID 查找歌曲
  SongInfo getSongInfo(int songId) {
    return _songs.where((s) => s.songId == songId).first;
  }

  void getAllSongs() {
    final ptr = bindings.getAllSongs();
    final dartString = UtilFunction.cPtr2String(ptr);
    Map<String, dynamic> obj = jsonDecode(dartString);
    final songsRaw = obj[CoordinatorMacro.songsList] as List<dynamic>;
    for (final song in songsRaw) {
      print("歌曲信息为");
      print(song);
      _songs.add(SongInfo.fromJson(song as Map<String, dynamic>));
    }
    notifyListeners();
  }

  void addNewSong(List<SongInfo> songsList) {
    _songs.addAll(songsList);
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

    bool result = bindings.toggleMyLike(songId) == 1;

    if (!result) {
      DialogUtil.showInfoDialog("切换我喜欢状态发生错误，请重试");
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
      initialDirectory: AppCache.getExeDirectory(),
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

      String pathsPtr = jsonEncode(filePaths);
      // print("2");
      final cPtr = pathsPtr.toNativeUtf8().cast<Char>();
      // print("3");
      bindings.someImport(cPtr);
    }
  }

  void saveComment(int songId, String text) {
    final cPtr = text.toNativeUtf8().cast<Char>();
    bindings.saveComment(songId, cPtr);
    malloc.free(cPtr);
  }
}
