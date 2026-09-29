import 'dart:async';
import 'dart:convert';

import 'package:collection/collection.dart';
import 'package:ffi/ffi.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/Macro/coordinatorMacro.dart';
import 'package:junk_fusion_ui/Macro/sliderParam.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/bridge/dll_invoke.dart';
import 'package:junk_fusion_ui/model/song_info.dart';
import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/utils/utils.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:windows_taskbar/windows_taskbar.dart';

class PlaybackProvider extends ChangeNotifier {
  final SongProvider songProvider;
  PlaybackProvider({required this.songProvider});

  SongInfo? _currentSong;
  SongInfo? get currentSong => _currentSong;

  // 是否正在播放（true = 播放中，false = 暂停）
  bool _isPlaying = false;
  bool get isPlaying => _isPlaying;

  // 当前播放进度时间戳
  double _currentTimeStamp = 0;
  double get currentTimeStamp => _currentTimeStamp;

  List<double> _timeDomainSpec = [];
  List<double> get timeDomainSpec => _timeDomainSpec;

  List<SongInfo> _playList = [];
  List<SongInfo> get playList => _playList;
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

  void setPlayList(List<SongInfo> playList) {
    //设置播放列表
    _playList = playList;
    notifyListeners();
  }

  // 设置当前播放的歌曲 ID，并持久化存储
  //   这是整个 ChangeNotifier 模式的核心！
  Future<void> setNewSong(SongInfo song) async {
    if (song == _currentSong) return;
    _currentSong = song;
    // print(
    //   "_currentSong的duration为${_currentSong!.duration},song的duration为${song.duration}",
    // );
    if (_currentSong == null) return;
    _isPlaying = true;
    final ptr = bindings.getTimeDomainSpecBySongId(song.songId);
    final dartStr = ptr.cast<Utf8>().toDartString();
    // print(dartStr);
    final obj = jsonDecode(dartStr) as Map<String, dynamic>;
    bindings.freeString(ptr);
    final timeDomainSpec =
        obj[CoordinatorMacro.timeDomainSpecs] as List<dynamic>;
    _timeDomainSpec = timeDomainSpec
        .map((e) => (e as num).toDouble())
        .toList(); //歌曲时域图数据

    // print(_timeDomainSpec);

    notifyListeners();

    await AppCache.frontCacheRef.setInt("currentSongId", song.songId);
    bindings.play(song.songId, 0.0);
  }

  void restoreState() {
    final songId = AppCache.frontCacheRef.getInt('currentSongId');

    // print(
    //   '[PlaybackProvider.restoreState] 缓存 currentSongId=$songId，当前 songs 数量=${songProvider.songCount}',
    // );
    if (songId != null) {
      _currentSong = songProvider.getSongInfo(songId);
      // if (_currentSong == null) {
      //   print(
      //     '[PlaybackProvider.restoreState] 缓存中的歌曲($songId)不在已加载列表中，跳过恢复',
      //   );
      // }
    }

    _volume = AppCache.frontCacheRef.getDouble("masterVolume") ?? 0.0;
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
    {
      String jsonString =
          AppCache.frontCacheRef.getString("timeDomainSpec") ?? "";
      if (jsonString != "") {
        final tempList = jsonDecode(jsonString) as List<dynamic>;
        for (final one in tempList) {
          _timeDomainSpec.add((one as num).toDouble());
        }
      }
    }

    notifyListeners();
  }

  void initPlaybackProvider() {
    restoreState();
    if (currentSong != null) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        _setWindowTaskbarBtn();
        WindowsTaskbar.setThumbnailTooltip(
          "${currentSong!.title}${currentSong!.artists == null ? "" : " - ${currentSong!.artists!.join(" / ")}"}",
        );
        WindowsTaskbar.setWindowTitle(
          "${currentSong!.title}${currentSong!.artists == null ? "" : " - ${currentSong!.artists!.join(" / ")}"}",
        );
      });
    }
  }

  void _setWindowTaskbarBtn() {
    WindowsTaskbar.setThumbnailToolbar([
      ThumbnailToolbarButton(
        ThumbnailToolbarAssetIcon('assets/image/player-skip-back_24x24.ico'),
        '上一首',
        () {
          playPreviousSong();
          if (currentSong != null) {
            WindowsTaskbar.setThumbnailTooltip(
              "${currentSong!.title}${currentSong!.artists == null ? "" : " - ${currentSong!.artists!.join(" / ")}"}",
            );
            WindowsTaskbar.setWindowTitle(
              "${currentSong!.title}${currentSong!.artists == null ? "" : " - ${currentSong!.artists!.join(" / ")}"}",
            );
          }
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
          _setWindowTaskbarBtn();
        },
      ),
      ThumbnailToolbarButton(
        ThumbnailToolbarAssetIcon('assets/image/player-skip-forward_24x24.ico'),
        '下一首',
        () {
          playNextSong();
          if (currentSong != null) {
            WindowsTaskbar.setThumbnailTooltip(
              "${currentSong!.title}${currentSong!.artists == null ? "" : " - ${currentSong!.artists!.join(" / ")}"}",
            );
            WindowsTaskbar.setWindowTitle(
              "${currentSong!.title}${currentSong!.artists == null ? "" : " - ${currentSong!.artists!.join(" / ")}"}",
            );
          }
        },
      ),
    ]);
  }

  Future<void> saveState() async {
    //这个函数只在退出应用的时候调用
    // _isPTSLock = true;
    if (_currentSong != null) {
      await AppCache.frontCacheRef.setInt(
        "currentSongId",
        _currentSong!.songId,
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
      final jsonString = jsonEncode(_timeDomainSpec);
      await AppCache.frontCacheRef.setString("timeDomainSpec", jsonString);
    }
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
      bindings.play(_currentSong!.songId, _currentTimeStamp);
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

  void seekPreferPTS(double targetSeconds) {
    //跳转到目标秒数

    _currentTimeStamp = targetSeconds;
    // print("seekCurrentPTS收到跳转到PTS通知:$targetSeconds");
    if (_isPlaying) {
      bindings.play(_currentSong!.songId, targetSeconds);
    }
    notifyListeners();
  }

  void setCurrentPTS(double currentPTS) {
    // if (_isPTSLock) return;
    _currentTimeStamp = currentPTS;
    // print("seekCurrentPTS收到设置当前PTS通知:$currentPTS");
    notifyListeners();
  }

  void playNextSong() {
    // print("playNextSong准备阶段歌曲为${_currentSong!.title}");
    final currentSongIndex = _playList.indexWhere(
      (i) => i.songId == _currentSong!.songId,
    );
    if (currentSongIndex < _playList.length - 1) {
      _currentSong = _playList[currentSongIndex + 1];
      bindings.play(_currentSong!.songId, 0.0);
      // print("改变后的歌曲为${_currentSong!.title}");
      _isPlaying = true;
    } else {
      _currentTimeStamp = 0.0; //播完且之后没有任何歌曲直接把pts重置回0.0其他不动
      _isPlaying = false;
    }

    notifyListeners();
  }

  //播放播放列表的上一首歌曲
  void playPreviousSong() {
    // print("playNextSong准备阶段歌曲为${_currentSong!.title}");
    final currentSongIndex = _playList.indexWhere(
      (i) => i.songId == _currentSong!.songId,
    );
    if (currentSongIndex > 0) {
      _currentSong = _playList[currentSongIndex - 1];
      bindings.play(_currentSong!.songId, 0.0);
      // print("改变后的歌曲为${_currentSong!.title}");
      _isPlaying = true;
    } else {
      _currentTimeStamp = 0.0; //播完且之后没有任何歌曲直接把pts重置回0.0其他不动
      _isPlaying = false;
    }

    notifyListeners();
  }
}
