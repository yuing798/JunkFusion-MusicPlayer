/// 歌词数据源（当前播放歌曲的歌词）
///
/// ── 职责 ─────────────────────────────────────────────────────────────
/// 只做一件事：**按 songId 拉一次歌词并缓存**。
/// 切歌时调 [loadForSong]，同一个 songId 重复调不会重复请求。
///
/// ── 数据从哪来 ────────────────────────────────────────────────────────
/// 后端 getSongLyrics(songId) → {"lyric":{hasTranslation, lines[]}}。
/// 解析（LRC → 时间轴）在后端做，这里只做 JSON → 模型。
///
/// ── 用法 ─────────────────────────────────────────────────────────────
///     context.read<LyricProvider>().loadForSong(songId);
///     final lyrics = context.watch<LyricProvider>().lyrics;   // 可能为 null
library;

import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/model/lyric_info.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';

class LyricProvider extends ChangeNotifier {
  int? _songId;
  LyricDocument? _lyrics;

  /// 当前已经加载的歌曲 id（null = 还没加载过）
  int? get songId => _songId;

  /// 当前歌曲的歌词；null 表示没有歌词（或者还没加载）
  LyricDocument? get lyrics => _lyrics;

  /// 切歌时调用：songId 没变就直接返回
  void loadForSong(int? songId) {
    if (songId == _songId) return;

    _songId = songId;
    _lyrics = null;

    if (songId == null) {
      notifyListeners();
      return;
    }

    _fetch(songId);
  }

  void _fetch(int songId) {
    try {
      final raw = UtilFunction.cPtr2String(bindings.getSongLyrics(songId));
      final decoded = jsonDecode(raw);

      final lyricJson = decoded is Map ? decoded['lyric'] : null;
      if (lyricJson is Map) {
        _lyrics = LyricDocument.fromJson(
          lyricJson.cast<String, dynamic>(),
        );
      }
    } catch (e) {
      // 歌词坏了不该影响播放，退化成"没有歌词"
      print('LyricProvider: 读取歌词失败 songId=$songId, $e');
      _lyrics = null;
    }

    notifyListeners();
  }
}
