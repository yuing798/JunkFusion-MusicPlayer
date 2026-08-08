// ════════════════════════════════════════════════════════════════
// playback_provider.dart — 播放状态管理
// ════════════════════════════════════════════════════════════════
import 'package:flutter/foundation.dart';
import 'package:shared_preferences/shared_preferences.dart';

class PlaybackProvider extends ChangeNotifier {
  // 当前正在播放的歌曲 ID，null 表示未播放任何歌曲
  int? _currentSongId;

  int? get currentSongId => _currentSongId;

  // 是否正在播放（true = 播放中，false = 暂停）
  bool _isPlaying = false;
  bool get isPlaying => _isPlaying;

  // 当前播放进度时间戳（null 表示无进度信息）
  int? _currentTimeStamp;
  int? get currentTimeStamp => _currentTimeStamp;

  // 播放模式枚举
  // 0 = 顺序播放，1 = 列表循环，2 = 单曲循环，3 = 随机播放
  int _playMode = 0;
  int get playMode => _playMode;

  // 设置当前播放的歌曲 ID，并持久化存储
  //   这是整个 ChangeNotifier 模式的核心！
  Future<void> setCurrentSongId(int songId) async {
    if (songId == _currentSongId) return;
    _currentSongId = songId;
    notifyListeners();
    final localStorage = await SharedPreferences.getInstance();
    await localStorage.setInt("currentSongId", songId);
  }

  // 从持久化存储恢复当前播放歌曲 ID
  Future<void> restoreCurrentSongId() async {
    final prefs = await SharedPreferences.getInstance();
    final songId = prefs.getInt('currentSongId');
    if (songId != null) _currentSongId = songId;
    notifyListeners();
  }

  // 切换播放/暂停状态
  void togglePlayPause() {
    _isPlaying = !_isPlaying;
    notifyListeners();
  }

  void setPlayState(int songId) {
    //把某首歌设置为播放状态
    _currentSongId = songId;
    _isPlaying = true;
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
    _currentTimeStamp = null;
    _playMode = 0;
    notifyListeners();
  }
}
