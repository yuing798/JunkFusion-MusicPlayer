/// 三个「固定歌单」：我喜欢 / 最近播放 / 最多播放
///
/// ── 为什么是枚举而不是数据类 ──────────────────────────────────────────
/// 它们**不是** `playlists` 表里的行，而是对 songs 表的三种固定查询
/// （后端 `SongsManage::getFixedPlaylistSongs`），没有 id、没有创建时间、
/// 不能改名、不能删除。所以这里只需要"是哪一种"这个标识 + 展示用的文案，
/// 用枚举最贴切（freezed 是给有字段的数据类用的，枚举用不上）。
///
/// ── 200 首从哪来 ──────────────────────────────────────────────────────
/// 后端 `SongsManage::kFixedPlaylistLimit = 200` 写死的，前端不再传数字，
/// 只在副标题里说明"最多 200 首"。两边要改一起改。
///
/// ── 用法 ──────────────────────────────────────────────────────────────
///     for (final kind in FixedPlaylistKind.values) { kind.title / kind.icon ... }
///     provider.openFixedPlaylist(kind);   // 见 PlaylistProvider
library;

import 'package:flutter/widgets.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

enum FixedPlaylistKind {
  /// 我点过"喜欢"的歌
  liked,

  /// 按最后一次播放时间排
  recent,

  /// 按播放次数排
  mostPlayed,
}

extension FixedPlaylistKindX on FixedPlaylistKind {
  /// 和 FFI `getFixedPlaylistSongs(kind)` 通信用的名字
  String get wireName => switch (this) {
    FixedPlaylistKind.liked => 'liked',
    FixedPlaylistKind.recent => 'recent',
    FixedPlaylistKind.mostPlayed => 'mostPlayed',
  };

  /// 歌单名（列表卡片、单歌单页标题都用它）
  String get title => switch (this) {
    FixedPlaylistKind.liked => '我喜欢',
    FixedPlaylistKind.recent => '最近播放',
    FixedPlaylistKind.mostPlayed => '最多播放',
  };

  /// 一句话说明这一份是怎么来的（卡片副标题）
  String get subtitle => switch (this) {
    FixedPlaylistKind.liked => '点过喜欢的歌，按最近听过排序',
    FixedPlaylistKind.recent => '按最后一次播放的时间排序',
    FixedPlaylistKind.mostPlayed => '按播放次数排序',
  };

  /// 卡片封面位置的图标（固定歌单没有封面图，用图标代替）
  IconData get icon => switch (this) {
    FixedPlaylistKind.liked => TablerIcons.heart,
    FixedPlaylistKind.recent => TablerIcons.history,
    FixedPlaylistKind.mostPlayed => TablerIcons.chartBar,
  };

  /// 固定歌单没有真实 id，但 PlaylistProvider 当前歌单字段是 int id，
  /// 这里用**负数**合成一个：真实歌单的 id 从 1 开始，负数永远不会撞。
  int get syntheticId => -1 - index;
}
