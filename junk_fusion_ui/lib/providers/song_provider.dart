import 'dart:convert';
import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart' as ffi;
import 'package:file_picker/file_picker.dart';
import 'package:flutter/foundation.dart';
import 'package:junk_fusion_ui/Macro/coordinatorMacro.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/model/song_basic_info.dart';
import 'package:junk_fusion_ui/utils/app_cache.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';

// ════════════════════════════════════════════════════════════════
// song_provider.dart — 歌曲列表状态管理
// ════════════════════════════════════════════════════════════════
//这里只存储歌曲的基本信息，全部信息开新歌的时候再去后端拿
class SongProvider extends ChangeNotifier {
  List<SongBasicInfo> _songs = [];

  List<SongBasicInfo> get songs => _songs;
  int get songCount => _songs.length;

  void getAllSongs() {
    print("准备后端getAllSongs");
    final ptr = bindings.getAllSongs();
    // print("获得getAllSongs返回的指针");
    final dartString = UtilFunction.cPtr2String(ptr);
    print("后端获取的字符串为$dartString");
    Map<String, dynamic> obj = jsonDecode(dartString);
    final songsRaw = obj[CoordinatorMacro.songsList] as List<dynamic>?;
    if (songsRaw == null) return;
    for (final song in songsRaw) {
      // print("歌曲信息为");
      // print(song);
      _songs.add(SongBasicInfo.fromJson(song as Map<String, dynamic>));
    }
    notifyListeners();
    // print("getAllSongs执行完毕");
  }

  void addNewSong(List<SongBasicInfo> songsList) {
    _songs.addAll(songsList);
    notifyListeners();
  }

  Future<void> toggleMyLike(int songId) async {
    bindings.toggleMyLike(songId);
  }

  // 导入歌曲
  Future<void> songsImport() async {
    // 调用 pickFiles，传入配置参数
    FilePickerResult? result = await FilePicker.pickFiles(
      dialogTitle: '请选择音频文件',
      initialDirectory: AppCache.appDocDir,
      allowMultiple: true,
      lockParentWindow: true,
    );

    if (result != null) {
      // 获取选中的文件路径（绝对路径）
      List<String> filePaths = result.files
          .map((e) => e.path) // 转换为 Iterable<String?>
          .whereType<String>() // 过滤掉 null，转换为 Iterable<String>
          .toList(); // 转为 List<String>

      Map<String, List<String>> jsonMap = {
        CoordinatorMacro.filePaths: filePaths,
      };

      String jsonMapStr = jsonEncode(jsonMap);
      final cPtr = UtilFunction.string2cPtr(jsonMapStr);
      bindings.someImport(cPtr);
      ffi.malloc.free(cPtr);
    }
  }

  //更新某条歌曲
  void requestUpdateSongBasicInfo({required int songId}) {
    final index = _songs.indexWhere((s) => s.songId == songId);
    if (index == -1) return;

    final old = _songs[index];
    final resultMap = jsonDecode(
      UtilFunction.cPtr2String(bindings.getBasicSongInfo(songId)),
    );
    final updated = SongBasicInfo.fromJson(resultMap);

    if (updated == old) return;

    _songs[index] = updated;
    notifyListeners();
  }
}
