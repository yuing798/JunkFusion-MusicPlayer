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

  final Map<int, SongBasicInfo> _idToInfoMap = {};
  Map<int, SongBasicInfo> get idToInfoMap => _idToInfoMap;

  int get songCount => _songs.length;

  // void getAllSongs() {
  //   print("准备后端getAllSongs");
  //   final ptr = bindings.getAllSongs();
  //   // print("获得getAllSongs返回的指针");
  //   final dartString = UtilFunction.cPtr2String(ptr);
  //   print("后端获取的字符串为$dartString");
  //   Map<String, dynamic> obj = jsonDecode(dartString);
  //   final songsRaw = obj[CoordinatorMacro.songsList] as List<dynamic>?;
  //   if (songsRaw == null) return;

  //   // 重新拉之前必须清空：以前这里只 add，调用第二次整个列表就会翻倍。
  //   // 导入文件夹之后要靠它把新歌补进来，所以不能再是"只追加"。
  //   _songs.clear();
  //   for (final song in songsRaw) {
  //     // print("歌曲信息为");
  //     // print(song);
  //     _songs.add(SongBasicInfo.fromJson(song as Map<String, dynamic>));
  //   }
  //   notifyListeners();
  //   // print("getAllSongs执行完毕");
  // }

  void addOrUpdateSong(SongBasicInfo info) {
    _idToInfoMap[info.songId] = info;
    notifyListeners();
  }

  void deleteSong(int songId) {
    _idToInfoMap.removeWhere((int id, _) => id == songId);
    notifyListeners();
  }

  Future<void> toggleMyLike(int songId) async {
    bindings.toggleMyLike(songId);
  }

  // 导入歌曲（单独导入，不是整文件夹）
  //
  // 注意后端契约：libExport 的 importSongs(files) 是拿 Yvar::parse(files) 之后
  // 当**数组**一个个读路径的（SongImport::importSingleSongs），所以要发
  // ["D:\\a.mp3", ...] 这种纯数组，不能发 {filePaths: [...]} 对象。
  //
  // 以前这里调的是 bindings.someImport，但那个函数在 libExport.cpp 里已经被
  // 注释掉了（DLL 里没有这个符号，一调就抛），所以换成了 importSongs。
  //
  // FIXME: importSongs 是同步的，导入多少首就卡多久（每首都要 ffmpeg +
  //   taglib + 写库）。文件多的话最好由后端开线程导入、导完再回调前端。
  Future<void> songsImport() async {
    // 调用 pickFiles，传入配置参数
    FilePickerResult? result = await FilePicker.pickFiles(
      dialogTitle: '请选择音频文件',
      initialDirectory: AppCache.appDocDir,
      allowMultiple: true,
      lockParentWindow: true,
    );

    if (result == null) return;

    // 获取选中的文件路径（绝对路径）
    final filePaths = result.files
        .map((e) => e.path) // 转换为 Iterable<String?>
        .whereType<String>() // 过滤掉 null，转换为 Iterable<String>
        .toList(); // 转为 List<String>
    if (filePaths.isEmpty) return;

    final jsonArrayStr = jsonEncode(filePaths);
    final cPtr = UtilFunction.string2cPtr(jsonArrayStr);
    try {
      // 导入完成之后后端会回调 onLightSongDataImportOver，新歌由
      // CppCallbackManager 补进 SongProvider（见 cpp_func_manager.dart）
      bindings.importSongs(cPtr);
    } finally {
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
