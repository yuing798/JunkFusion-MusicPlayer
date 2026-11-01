import 'dart:convert';

import 'package:ffi/ffi.dart' as ffi;
import 'package:file_picker/file_picker.dart';
import 'package:flutter/foundation.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/model/song_basic_info.dart';
import 'package:junk_fusion_ui/utils/app_cache.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';

// ════════════════════════════════════════════════════════════════
// song_provider.dart — 歌曲基本信息的缓存
//
// 现在只有一份「songId -> 基本信息」的缓存（idToInfoMap），不再维护全量列表：
//   · 导入 / 元数据变化：后端回调 registerOnAddOrUpdateSong → addOrUpdateSong
//   · 从库里删歌：后端回调 registerOnDeleteSong → deleteSong
//   · 页面要读歌：先 ensureSongsLoaded(ids)，再按 id 从 idToInfoMap 取
//     （文件夹视图、专辑详情、歌单详情、播放状态都是这个套路）
//
// 歌曲详细信息（时长/码率/波形那些）开新歌播放的时候才回后端拿，不在这里存。
// ════════════════════════════════════════════════════════════════
class SongProvider extends ChangeNotifier {
  final Map<int, SongBasicInfo> _idToInfoMap = {};
  Map<int, SongBasicInfo> get idToInfoMap => _idToInfoMap;

  int get songCount => _idToInfoMap.length;

  /// 后端通知某首歌新增 / 更新（导入完成、我喜欢、播放次数都会走这里）
  void addOrUpdateSong(SongBasicInfo info) {
    _idToInfoMap[info.songId] = info;
    notifyListeners();
  }

  /// 后端通知某首歌从库里删掉了（比如文件夹里的文件被删了）
  void deleteSong(int songId) {
    if (_idToInfoMap.remove(songId) == null) return;
    notifyListeners();
  }

  /// 确保这些歌的基本信息已经在缓存里（缺的回后端一个个补）
  ///
  /// 文件夹视图点开目录、专辑/歌单详情页打开时，后端只给 songId，
  /// 靠这个把缺的歌补进来；补完如果真有新增会 notifyListeners。
  ///
  /// TODO: 现在是一个 id 一次 FFI 调用。如果一个目录几百首、又都是第一次打开，
  ///   这里会有点慢；以后可以让后端加一个按 id 数组批量返回基本信息的接口。
  void ensureSongsLoaded(Iterable<int> songIds) {
    var added = false;

    for (final songId in songIds) {
      if (_idToInfoMap.containsKey(songId)) continue;

      try {
        final raw = UtilFunction.cPtr2String(bindings.getBasicSongInfo(songId));
        if (raw.isEmpty) continue;

        final info = SongBasicInfo.fromJson(
          jsonDecode(raw) as Map<String, dynamic>,
        );
        _idToInfoMap[songId] = info;
        added = true;
      } catch (error) {
        print('[SongProvider] 取歌曲基本信息失败 songId=$songId：$error');
      }
    }

    if (added) notifyListeners();
  }

  Future<void> toggleMyLike(int songId) async {
    // 后端改完库会回调 onAddOrUpdateSong，把最新的 isMyLike 推回来
    bindings.toggleMyLike(songId);
  }

  /// 单独导入歌曲（不是整文件夹）
  ///
  /// 后端契约：libExport 的 importSongs(files) 把参数当**路径数组**读
  /// （SongImport::importSongs），所以发的是 ["D:\\a.mp3", ...]；
  /// 导入时后端会把文件拷到 <缓存目录>/songs 下再入库。
  ///
  /// FIXME: importSongs 是同步的，导入多少首就卡多久（每首都要 ffmpeg +
  ///   taglib + 写库）。文件多的话最好由后端开线程导入。
  Future<void> songsImport() async {
    FilePickerResult? result = await FilePicker.pickFiles(
      dialogTitle: '请选择音频文件',
      initialDirectory: AppCache.appDocDir,
      allowMultiple: true,
      lockParentWindow: true,
    );

    if (result == null) return;

    final filePaths = result.files
        .map((e) => e.path)
        .whereType<String>()
        .toList();
    if (filePaths.isEmpty) return;

    final jsonArrayStr = jsonEncode(filePaths);
    final cPtr = UtilFunction.string2cPtr(jsonArrayStr);
    try {
      // 每导入/更新完一首，后端会回调 registerOnAddOrUpdateSong，
      // 由 CppCallbackManager 塞进 idToInfoMap（见 cpp_func_manager.dart）
      bindings.importSongs(cPtr);
    } finally {
      ffi.malloc.free(cPtr);
    }
  }
}
