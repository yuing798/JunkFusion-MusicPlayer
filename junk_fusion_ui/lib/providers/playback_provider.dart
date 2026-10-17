import 'dart:async';
import 'dart:convert';

import 'package:collection/collection.dart';
import 'package:ffi/ffi.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/Macro/coordinatorMacro.dart';
import 'package:junk_fusion_ui/Macro/sliderParam.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/model/song_basic_info.dart';
import 'package:junk_fusion_ui/model/song_detail_info.dart';
import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/utils/app_cache.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';
import 'package:windows_taskbar/windows_taskbar.dart';

class PlaybackProvider extends ChangeNotifier {
  final SongProvider songProvider;
  PlaybackProvider({required this.songProvider});

  SongDetailInfo? _curDetailInfo;
  SongDetailInfo? get curDetailInfo => _curDetailInfo;

  SongBasicInfo? _curBasicInfo;
  SongBasicInfo? get curBasicInfo => _curBasicInfo;

  // 是否正在播放（true = 播放中，false = 暂停）
  bool _isPlaying = false;
  bool get isPlaying => _isPlaying;

  // 当前播放进度时间戳
  double _currentTimeStamp = 0;
  double get currentTimeStamp => _currentTimeStamp;

  List<SongBasicInfo> _playList = [];
  List<SongBasicInfo> get playList => _playList;
  int get playListCount => _playList.length;

  // 播放模式枚举
  // 0 = 顺序播放，1 = 列表循环，2 = 单曲循环，3 = 随机播放
  int _playMode = 0;
  int get playMode => _playMode;

  double _volume = 0.0;
  double get volume => _volume;

  void setVolume(double value) {
    _volume = value;
    notifyListeners();
  }

  void setPlayList(List<SongBasicInfo> list) {
    _playList = list;
    notifyListeners();
  }

  void _setSongDetailInfo(int songId) {
    _curDetailInfo = SongDetailInfo.fromJson(
      jsonDecode(
        UtilFunction.cPtr2String(bindings.getCompleteSongInfo(songId)),
      ),
    );
  }

  void _setSongBasicInfo(int songId) {
    _curBasicInfo = songProvider.songs
        .where((each) => each.songId == songId)
        .firstOrNull;
  }

  void setNewInfo(int songId) {
    _setSongBasicInfo(songId);
    _setSongDetailInfo(songId);
  }

  Future<void> setNewSong(int songId) async {
    if (songId == _curBasicInfo?.songId) return;
    setNewInfo(songId);
    if (_curBasicInfo == null || _curDetailInfo == null) return;
    _isPlaying = true;

    notifyListeners();

    bindings.play(_curBasicInfo!.songId, 0.0, 0, 1);

    saveState(); //切换歌曲的时候顺便缓存数据
    _setWindowBarTitleAndArtists();
  }

  /// 恢复状态，初始化的时候调用
  void restoreState() {
    final songId = AppCache.frontCacheRef.getInt('currentSongId');

    if (songId != null) {
      setNewInfo(songId);
    }

    _volume = AppCache.frontCacheRef.getDouble("masterVolume") ?? 0.5;
    UtilFunction.sendSliderMsg(SliderParam.masterVolume, _volume, false);
    _currentTimeStamp = AppCache.frontCacheRef.getDouble("pts") ?? 0.0;
    {
      String jsonString =
          AppCache.frontCacheRef.getString("playListSongIdList") ?? "";
      if (jsonString != "") {
        final tempList = jsonDecode(jsonString) as List<dynamic>;
        for (final one in tempList) {
          final id = (one as num).toInt();
          final info = songProvider.songs
              .where((i) => i.songId == id)
              .firstOrNull;
          if (info != null) {
            _playList.add(info);
          }
        }
      }
    }
    _playMode = AppCache.frontCacheRef.getInt("cycleMode") ?? 0;

    notifyListeners();

    if (_curBasicInfo != null) {
      bindings.play(_curBasicInfo!.songId, _currentTimeStamp, 0, 1);
      bindings.pausePlay();
    } //为了在后端初始化的时候存储播放信息
  }

  void initPlaybackProvider() {
    restoreState();
    if (_curBasicInfo != null) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        _setWindowTaskbarBtn();
        _setWindowBarTitleAndArtists();
      });
    }
  }

  void _setWindowBarTitleAndArtists() {
    if (_curBasicInfo != null) {
      WindowsTaskbar.setThumbnailTooltip(
        "${_curBasicInfo!.title}${_curBasicInfo!.artists == null ? "" : " - ${_curBasicInfo!.artists!.join(" / ")}"}",
      );
      WindowsTaskbar.setWindowTitle(
        "${_curBasicInfo!.title}${_curBasicInfo!.artists == null ? "" : " - ${_curBasicInfo!.artists!.join(" / ")}"}",
      );
    }
  }

  void _setWindowTaskbarBtn() {
    WindowsTaskbar.setThumbnailToolbar([
      ThumbnailToolbarButton(
        ThumbnailToolbarAssetIcon('assets/image/player-skip-back_24x24.ico'),
        '上一首',
        () {
          playPreviousSong();
        },
      ),
      ThumbnailToolbarButton(
        ThumbnailToolbarAssetIcon(
          isPlaying
              ? 'assets/image/player-pause_24x24.ico'
              : 'assets/image/player-play_24x24.ico',
        ),
        isPlaying ? '暂停' : '播放',
        () {
          togglePlayPause();
        },
      ),
      ThumbnailToolbarButton(
        ThumbnailToolbarAssetIcon('assets/image/player-skip-forward_24x24.ico'),
        '下一首',
        () {
          playNextSong();
        },
      ),
    ]);
  }

  Future<void> saveState() async {
    if (_curBasicInfo != null) {
      await AppCache.frontCacheRef.setInt(
        "currentSongId",
        _curBasicInfo!.songId,
      );
    }
    await AppCache.frontCacheRef.setDouble("masterVolume", _volume);
    await AppCache.frontCacheRef.setDouble("pts", _currentTimeStamp);
    {
      List<int> playListSongIdList = [];
      for (final song in _playList) {
        playListSongIdList.add(song.songId);
      }
      final jsonString = jsonEncode(playListSongIdList);
      await AppCache.frontCacheRef.setString("playListSongIdList", jsonString);
    }
    {
      await AppCache.frontCacheRef.setInt("cycleMode", _playMode);
    }
  }

  //这个在后端通过smtc改变播放状态的时候调用
  void setPlayPauseIcon(bool state) {
    _isPlaying = state;
    notifyListeners();
    _setWindowTaskbarBtn();
  }

  // 切换播放/暂停状态(前端直接调用)
  void togglePlayPause() {
    _isPlaying = !_isPlaying;
    notifyListeners();
    if (!_isPlaying) {
      //注意这里是切换完成后的状态
      bindings.pausePlay();
    } else {
      if (_curBasicInfo == null) return;
      bindings.play(_curBasicInfo!.songId, _currentTimeStamp, 0, 0);
    }
    _setWindowTaskbarBtn();
  }

  // 切换播放模式（循环 0 → 1 → 2 → 3 → 0 → ...）
  //0 = 顺序播放，1 = 列表循环，2 = 单曲循环，3 = 随机播放
  void cyclePlayMode() async {
    _playMode = (_playMode + 1) % 4;
    notifyListeners();
  }

  void seekPreferPTS(double targetSeconds) {
    _currentTimeStamp = targetSeconds;
    // print("seekCurrentPTS收到跳转到PTS通知:$targetSeconds");
    if (_isPlaying) {
      bindings.play(_curBasicInfo!.songId, targetSeconds, 1, 0);
    }
    notifyListeners();
  }

  void updateCurrentPTS(double currentPTS) {
    _currentTimeStamp = currentPTS;
    notifyListeners();
  }

  void playNextSong() {
    if (_curBasicInfo == null) return;
    final currentSongIndex = _playList.indexWhere(
      (i) => i.songId == _curBasicInfo!.songId,
    );
    if (currentSongIndex < _playList.length - 1) {
      final targetSongId = _playList[currentSongIndex + 1].songId;
      setNewSong(targetSongId);
    } else {
      _currentTimeStamp = 0.0; //播完且之后没有任何歌曲直接把pts重置回0.0其他不动
      _isPlaying = false;
    }

    notifyListeners();
  }

  //播放播放列表的上一首歌曲
  void playPreviousSong() {
    final currentSongIndex = _playList.indexWhere(
      (i) => i.songId == _curBasicInfo!.songId,
    );
    if (currentSongIndex > 0) {
      final targetSongId = _playList[currentSongIndex - 1].songId;
      setNewSong(targetSongId);
    } else {
      _currentTimeStamp = 0.0; //播完且之后没有任何歌曲直接把pts重置回0.0其他不动
      _isPlaying = false;
    }

    notifyListeners();
  }
}
