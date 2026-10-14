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
import 'package:junk_fusion_ui/widgets/popup_window.dart';

// ════════════════════════════════════════════════════════════════
// song_provider.dart — 歌曲列表状态管理
// ════════════════════════════════════════════════════════════════
//这里只存储歌曲的基本信息，全部信息开新歌的时候再去后端拿
class SongProvider extends ChangeNotifier {
  List<SongBasicInfo> _songs = [];

  List<SongBasicInfo> get songs => _songs;
  int get songCount => _songs.length;

  // 歌曲数据的版本号：只有「歌曲集合 / 元数据真的变了」才 +1
  // （导入新歌、后端回传了新的元数据）。专辑这种按歌曲聚合出来的视图靠它
  // 判断需不需要重新向后端要数据；切换「我喜欢」不算，它不改分组结果。
  int _revision = 0;
  int get revision => _revision;

  void getAllSongs() {
    print("准备后端getAllSongs");
    final ptr = bindings.getAllSongs();
    // print("获得getAllSongs返回的指针");
    final dartString = UtilFunction.cPtr2String(ptr);
    Map<String, dynamic> obj = jsonDecode(dartString);
    final songsRaw = obj[CoordinatorMacro.songsList] as List<dynamic>;
    for (final song in songsRaw) {
      // print("歌曲信息为");
      // print(song);
      _songs.add(SongBasicInfo.fromJson(song as Map<String, dynamic>));
    }
    _revision++;
    notifyListeners();
  }

  void addNewSong(List<SongBasicInfo> songsList) {
    _songs.addAll(songsList);
    _revision++;
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
  void requestUpdateSongInfo({required int songId}) {
    final index = _songs.indexWhere((s) => s.songId == songId);
    if (index == -1) return;

    final old = _songs[index];
    final resultMap = jsonDecode(
      UtilFunction.cPtr2String(bindings.getCompleteSongInfo(songId)),
    );
    final updated = SongBasicInfo.fromJson(resultMap);

    if (updated == old) return;

    _songs[index] = updated;
    notifyListeners();
  }
}
