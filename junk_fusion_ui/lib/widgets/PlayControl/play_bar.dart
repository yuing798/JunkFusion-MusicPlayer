// ════════════════════════════════════════════════════════════════
// play_bar.dart — 底部播放栏（常驻，属于布局而不是浮层）
//
// 布局（固定 116px，自上而下两行）：
//   第 1 行：进度条 + 左右时间标签（左右各留 30，和下面三区对齐）
//   第 2 行：三区 Row —— left(flex 5) | mid(flex 4) | right(flex 5)
//
// 为什么进度条在栏里面、而且偏上：
//   它以前浮在播放栏顶沿（离窗口底边只有十几像素），鼠标甩到屏幕底部
//   想点列表时很容易先点到它，一点就跳转。现在下面压着整行控制区，
//   误触不到；代价是栏比原来高了一点，但不再有"浮层盖内容"的问题。
//
// 左区：封面 50×50 + 歌名/艺术家 + 喜欢按钮 + 歌曲详情弹窗
// 中区：播放模式切换 + 上一首 + 播放/暂停 + 下一首 + 播放列表
// 右区：音量 + 更多播放信息
// ════════════════════════════════════════════════════════════════

import 'dart:io';

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/providers/play_page_provider.dart';
import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/app_cache.dart';
import 'package:junk_fusion_ui/widgets/PlayControl/more_play_info.dart';
import 'package:junk_fusion_ui/widgets/PlayControl/play_list.dart';
import 'package:junk_fusion_ui/widgets/PlayControl/song_progress_area.dart';
import 'package:junk_fusion_ui/widgets/PlayControl/volumn_slider.dart';
import 'package:provider/provider.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

// PlayBar — 底部播放栏
class PlayBar extends StatefulWidget {
  const PlayBar({super.key, this.backgroundColor});

  /// 播放页可传透明色，让完整播放栏融入封面背景。
  final Color? backgroundColor;

  @override
  State<StatefulWidget> createState() {
    return PlayBarState();
  }
}

class PlayBarState extends State<PlayBar> with TickerProviderStateMixin {
  late final PlayList _playList;

  /// 播放栏高度：进度条那一行（约 32）+ 三区主体（约 84）
  static const double _height = 100;
  @override
  void initState() {
    super.initState();

    _playList = PlayList(this);
  }

  @override
  void dispose() {
    _playList.dispose();
    super.dispose();
  }

  Widget _buildRightArea(BuildContext context) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.end,
      children: [const VolumeControllerButton(), const MorePlayInfoWidget()],
    );
  }

  @override
  Widget build(BuildContext context) {
    final playback = context.watch<PlaybackProvider>();

    final theme = context.watch<AppTheme>();

    // 构建左区：封面 + 歌名/艺术家 + 喜欢 + 详情弹窗
    Widget buildLeftArea() {
      return Row(
        children: [
          // 点封面打开全屏播放页（歌曲仍在播，页面是浮层）
          GestureDetector(
            onTap: () => context.read<PlayPageProvider>().toggle(),
            child: MouseRegion(
              cursor: SystemMouseCursors.click,
              child: (playback.curBasicInfo?.hash != null)
                  ? Image.file(
                      File(
                        '${AppCache.cacheDirString}/image/${playback.curBasicInfo!.hash}/original.jpg',
                      ),
                      width: 50,
                      height: 50,
                      fit: BoxFit.cover,
                    )
                  : Image.asset(
                      "assets/image/JunkFusion.png",
                      width: 50,
                      height: 50,
                      fit: BoxFit.cover,
                      // cacheHeight: 50,
                      // cacheWidth: 50,
                    ),
            ),
          ),

          const SizedBox(width: 10),

          // ── 歌名 + 艺术家 ──
          Flexible(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  playback.curBasicInfo?.title ?? "",
                  style: theme.midTextStyle,
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
                Text(
                  playback.curBasicInfo?.artists == null
                      ? "未知"
                      : playback.curBasicInfo!.artists!.join(" / "),
                  style: theme.littleTextStyle,
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
              ],
            ),
          ),

          const SizedBox(width: 10),

          // ── 喜欢按钮
          (playback.curBasicInfo != null)
              ? IconButton(
                  icon: (playback.curBasicInfo!.isMyLike)
                      ? Icon(TablerIcons.heartFilled, color: theme.colorLike)
                      : Icon(TablerIcons.heart),
                  onPressed: () => context.read<SongProvider>().toggleMyLike(
                    playback.curBasicInfo!.songId,
                  ),
                )
              : SizedBox.shrink(),
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
            icon: Icon(TablerIcons.squareArrowLeftFilled, size: 32),
            onPressed: () {
              context.read<PlaybackProvider>().clickPlatPreviousSongBtn();
            },
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(minWidth: 32, minHeight: 32),
          ),

          // 播放 / 暂停
          IconButton(
            icon: Icon(
              playback.isPlaying
                  ? TablerIcons.playerPauseFilled
                  : TablerIcons.playerPlayFilled,
              size: 32,
            ),
            onPressed: () {
              context.read<PlaybackProvider>().togglePlayPause();
            },
            padding: EdgeInsets.zero,
          ),

          // 下一首
          IconButton(
            icon: Icon(TablerIcons.squareArrowRightFilled, size: 32),
            onPressed: () {
              context.read<PlaybackProvider>().clickPlayNextSongBtn();
            },
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(minWidth: 32, minHeight: 32),
          ),

          const SizedBox(width: 10),

          // 播放列表
          IconButton(
            icon: Icon(TablerIcons.listFilled, size: 32),
            onPressed: () {
              _playList.showPlayList(context);
            },
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(minWidth: 32, minHeight: 32),
          ),
        ],
      );
    }

    return Container(
      height: _height,
      // 常驻播放栏：自己就是一块实心背景，不再需要外层 Stack 托着
      color: widget.backgroundColor ?? theme.colorHover,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          // ── 进度条（偏上，不贴窗口底边）──
          //
          // 以前它用 Positioned(bottom: 80) 浮在播放栏顶沿，离窗口底边只有
          // 十几像素：鼠标甩到屏幕底部想点列表/歌单时很容易先撞上它，
          // 一点就把歌跳走了。现在把它放进播放栏内部，下面还压着整行控制区，
          // 手再抖也够不到。
          //
          // 左右 30 的边距和下面三区对齐；ConstrainedBox 只给最小高度——
          // 没有歌在播时 SongProgressArea 返回 SizedBox.shrink()，
          // 不给最小高度的话播放栏会突然变矮，整个界面跟着跳一下。
          Padding(
            padding: const EdgeInsets.fromLTRB(30, 5, 30, 0),
            child: ConstrainedBox(
              constraints: const BoxConstraints(minHeight: 20),
              child: const SongProgressArea(),
            ),
          ),

          // ── 主体三区（按比例瓜分空间，自适应窗口宽度）──
          // flex=5 : 4 : 5 ≈ 左区 / 中区 / 右区
          Expanded(
            child: Padding(
              padding: const EdgeInsets.fromLTRB(30, 0, 30, 6),
              child: Row(
                children: [
                  // ── 左区（封面 + 歌名/艺术家 + 喜欢 + 详情） ──
                  Expanded(flex: 5, child: buildLeftArea()),

                  // ── 中区（播放控制） ──
                  Expanded(flex: 4, child: buildMidArea()),

                  // ── 右区（音量 / 更多） ──
                  Expanded(flex: 5, child: _buildRightArea(context)),
                ],
              ),
            ),
          ),
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
      icon: Icon(icon, size: 32),
      onPressed: onTap,
      tooltip: tooltip, // 悬浮时显示提示文字
      padding: EdgeInsets.zero,
      constraints: const BoxConstraints(minWidth: 32, minHeight: 32),
    );
  }
}
