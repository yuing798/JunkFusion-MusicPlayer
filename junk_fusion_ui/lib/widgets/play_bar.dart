// ════════════════════════════════════════════════════════════════
// play_bar.dart — 底部播放栏
//
// 布局（固定底部 90px）：
//   Grid 三区: left-area (500px) | spacer | mid-area (330px) | spacer | right-area (550px)
//   简化为 Row: left | Expanded | center | Expanded | right
//
// 左区：封面 50×50 + 歌名/艺术家 + 喜欢按钮 + 歌曲详情弹窗
// 中区：播放模式切换 + 上一首 + 播放/暂停 + 下一首 + 播放列表
// 右区：（预留，当前为空）
// ════════════════════════════════════════════════════════════════

import 'dart:io';

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/bridge/dll/dll_invoke.dart';
import 'package:junk_fusion_ui/model/song_info.dart';
import 'package:junk_fusion_ui/widgets/helper_widget.dart';
import 'package:provider/provider.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';
import '../providers/playback_provider.dart';
import '../providers/song_provider.dart';
import '../theme/app_theme.dart';
import 'popup_window.dart';
import 'song_detail_info.dart';

// PlayBar — 底部播放栏
class PlayBar extends StatelessWidget {
  const PlayBar({super.key});

  @override
  Widget build(BuildContext context) {
    final playback = context.watch<PlaybackProvider>();

    // 安全地获取当前歌曲（如果 currentId 为空，返回 null）
    final song = context.read<SongProvider>().getSongInfo(
      playback.currentSongId!,
    );

    assert(song != null);

    if (song == null) return SizedBox.shrink();

    Widget buildLikeButton() {
      return IconButton(
        icon: (song.isMyLike)
            ? Icon(TablerIcons.heartFilled, color: Colors.red)
            : Icon(TablerIcons.heart),
        onPressed: () => context.read<SongProvider>().toggleMyLike(song.songId),
      );
    }

    // 构建左区：封面 + 歌名/艺术家 + 喜欢 + 详情弹窗
    Widget buildLeftArea() {
      return Row(
        children: [
          (song.hash != null)
              ? Image.file(
                  File('$cacheDir/image/songs/${song.hash}/original.jpg'),
                  width: 50,
                  height: 50,
                  fit: BoxFit.cover,
                  cacheHeight: 50,
                  cacheWidth: 50,
                )
              : Image.asset(
                  "assets/JunkFusion.png",
                  width: 50,
                  height: 50,
                  fit: BoxFit.cover,
                  cacheHeight: 50,
                  cacheWidth: 50,
                ),

          const SizedBox(width: 10),

          // ── 歌名 + 艺术家 ──
          Expanded(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  song.title,
                  style: AppTheme.midTextStyle,
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
                Text(
                  song.artist ?? '未知',
                  style: AppTheme.littleTextStyle,
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
              ],
            ),
          ),

          const SizedBox(width: 10),

          // ── 喜欢按钮
          buildLikeButton(),

          // ── 歌曲详情弹窗──
          PopupWindow(
            title: '歌曲详情',
            triggerBuilder: (open) => IconButton(
              onPressed: open,
              icon: createIcon(TablerIcons.infoHexagonFilled),
            ),
            contentBuilder: () => SongDetailInfo(song: song),
          ),
        ],
      );
    }

    // 构建中区：播放模式 + 播放控制
    Widget buildMidArea() {
      // `Row` 的子元素之间用 gap（间距）
      return Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          // 播放模式按钮
          _PlayModeIcon(
            mode: playback.playMode,
            onTap: () {
              context.read<PlaybackProvider>().cyclePlayMode();
            },
          ),

          const SizedBox(width: 10),

          // 上一首
          IconButton(
            icon: createIcon(TablerIcons.squareArrowLeftFilled),
            onPressed: () {
              // TODO: 桥接层 - 调用上一首
            },
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(minWidth: 32, minHeight: 32),
          ),

          // 播放 / 暂停
          IconButton(
            icon: createIcon(
              playback.isPlaying
                  ? TablerIcons.playerPauseFilled
                  : TablerIcons.playerPlayFilled,
            ),
            onPressed: () {
              context.read<PlaybackProvider>().togglePlayPause();
            },
            padding: EdgeInsets.zero,
          ),

          // 下一首
          IconButton(
            icon: createIcon(TablerIcons.squareArrowRightFilled),
            onPressed: () {
              // TODO: 桥接层 - 调用下一首
            },
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(minWidth: 32, minHeight: 32),
          ),

          const SizedBox(width: 10),

          // 播放列表
          IconButton(
            icon: createIcon(TablerIcons.listFilled),
            onPressed: () {
              // TODO: 桥接层 - 打开播放列表
            },
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(minWidth: 32, minHeight: 32),
          ),
        ],
      );
    }

    return Container(
      height: 90,
      padding: const EdgeInsets.symmetric(horizontal: 30, vertical: 10),
      color: AppTheme.colorHover,

      // Row 布局（简化为三区）
      child: Row(
        children: [
          // ── 左区：550px（封面 + 歌名/艺术家 + 喜欢 + 详情） ──
          SizedBox(width: 550, child: buildLeftArea()),

          // 弹性空间
          const Expanded(child: SizedBox()),

          // ── 中区：330px（播放控制） ──
          SizedBox(width: 330, child: buildMidArea()),

          // 弹性空间
          const Expanded(child: SizedBox()),

          // ── 右区：550px（预留） ──
          const SizedBox(width: 550),
        ],
      ),
    );
  }
}

// _PlayModeIcon — 播放模式图标组件
//
// 根据 mode 值显示不同图标：
//   0 = 顺序播放 (arrow_right_alt)
//   1 = 列表循环 (repeat)
//   2 = 单曲循环 (repeat_one)
//   3 = 随机播放 (shuffle)
class _PlayModeIcon extends StatelessWidget {
  final int mode;
  final VoidCallback onTap;

  const _PlayModeIcon({required this.mode, required this.onTap});

  @override
  Widget build(BuildContext context) {
    // 根据 mode 选择对应的图标和提示文字
    IconData icon;
    String tooltip;

    // Dart 的 switch 表达式
    switch (mode) {
      case 0:
        icon = TablerIcons.arrowsRight; // 顺序播放
        tooltip = '顺序播放';
      case 1:
        icon = TablerIcons.repeat; // 列表循环
        tooltip = '列表循环';
      case 2:
        icon = TablerIcons.repeatOnce; // 单曲循环
        tooltip = '单曲循环';
      case 3:
        icon = TablerIcons.arrowsShuffle2; // 随机播放
        tooltip = '随机播放';
      default:
        icon = TablerIcons.arrowsRight;
        tooltip = '未知';
    }

    return IconButton(
      icon: createIcon(icon),
      onPressed: onTap,
      tooltip: tooltip, // 悬浮时显示提示文字
      padding: EdgeInsets.zero,
      constraints: const BoxConstraints(minWidth: 32, minHeight: 32),
    );
  }
}

// ════════════════════════════════════════════════════════════════
// Consumer2 说明：
//
// `Consumer2<A, B>` 是 provider 包提供的便捷 Widget，
// 可以同时监听两个 Provider 的变化。
//
// 一般形式：
// ```dart
// Consumer2<ProviderA, ProviderB>(
//   builder: (context, a, b, child) {
//     return Text('${a.value} ${b.value}');
//   },
// )
// ```
//
// 等价于嵌套 Consumer:
// ```dart
// Consumer<ProviderA>(
//   builder: (context, a, child) {
//     return Consumer<ProviderB>(
//       builder: (context, b, child) {
//         return Text('${a.value} ${b.value}');
//       },
//     );
//   },
// )
// ```
//
// Provider 包提供 Consumer 到 Consumer6（监听 1 到 6 个 Provider）。
// ════════════════════════════════════════════════════════════════
