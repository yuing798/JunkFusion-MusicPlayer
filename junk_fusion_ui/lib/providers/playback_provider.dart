// ════════════════════════════════════════════════════════════════
// playback_provider.dart — 播放状态管理
// ════════════════════════════════════════════════════════════════
import 'dart:async';
import 'dart:convert';

import 'package:ffi/ffi.dart';
import 'package:flutter/foundation.dart';
import 'package:junk_fusion_ui/bridge/dllAndFlutterBridgeDefs.dart';
import 'package:junk_fusion_ui/bridge/dll_invoke.dart';
import 'package:junk_fusion_ui/utils/utils.dart';
import 'package:shared_preferences/shared_preferences.dart';

class PlaybackProvider extends ChangeNotifier {
  // 当前正在播放的歌曲 ID，null 表示未播放任何歌曲
  int? _currentSongId;

  int? get currentSongId => _currentSongId;

  // 是否正在播放（true = 播放中，false = 暂停）
  bool _isPlaying = false;
  bool get isPlaying => _isPlaying;

  // 当前播放进度时间戳
  double _currentTimeStamp = 0;
  double get currentTimeStamp => _currentTimeStamp;

  List<double> _timeDomainSpec = [];
  List<double> get timeDomainSpec => _timeDomainSpec;

  // 播放模式枚举
  // 0 = 顺序播放，1 = 列表循环，2 = 单曲循环，3 = 随机播放
  int _playMode = 0;
  int get playMode => _playMode;

  // 设置当前播放的歌曲 ID，并持久化存储
  //   这是整个 ChangeNotifier 模式的核心！
  Future<void> setNewSong(int songId) async {
    if (songId == _currentSongId) return;
    _currentSongId = songId;
    if (_currentSongId == null) return;
    _isPlaying = true;
    final ptr = bindings.getTimeDomainSpecBySongId(_currentSongId!);
    final dartStr = ptr.cast<Utf8>().toDartString();
    final obj = jsonDecode(dartStr) as Map<String, dynamic>;
    bindings.freeString(ptr);
    final timeDomainSpec = obj[B_getTimeDomainSpec.specList] as List<dynamic>;
    _timeDomainSpec = timeDomainSpec
        .map((e) => (e as num).toDouble())
        .toList(); //歌曲时域图数据

    notifyListeners();

    await AppCache.frontCacheRef.setInt("currentSongId", songId);
    bindings.playNewSong(songId);
  }

  // 从持久化存储恢复当前播放歌曲 ID
  void restoreCurrentSongId() async {
    final songId = AppCache.frontCacheRef.getInt('currentSongId');
    if (songId != null) _currentSongId = songId;
    // print(_currentSongId);
    notifyListeners();
  }

  // 切换播放/暂停状态
  void togglePlayPause() {
    // print("准备切换播放暂停状态");
    _isPlaying = !_isPlaying;
    notifyListeners();
    if (!_isPlaying) {
      //注意这里是切换完成后的状态
      bindings.pausePlay();
    } else {
      bindings.continuePlay();
    }
  }

  // 切换播放模式（循环 0 → 1 → 2 → 3 → 0 → ...）
  //0 = 顺序播放，1 = 列表循环，2 = 单曲循环，3 = 随机播放
  void cyclePlayMode() async {
    _playMode = (_playMode + 1) % 4;
    notifyListeners();
    final localStorage = await SharedPreferences.getInstance();
    await localStorage.setInt("cycleMode", _playMode);
  }

  // 重置所有播放状态
  void reset() {
    _currentSongId = null;
    _isPlaying = false;
    _currentTimeStamp = 0;
    _playMode = 0;
    notifyListeners();
  }

  void seekPreferPTS(double targetSeconds) {
    //跳转到目标秒数
    bindings.seekTargetPTS(targetSeconds);
  }

  void setCurrentPTS(double currentPTS) {
    _currentTimeStamp = currentPTS;
    notifyListeners();
  }

  void setCurrentTimeDomainSpec() {
    //设置当前这首歌曲的时域图
  }
}
