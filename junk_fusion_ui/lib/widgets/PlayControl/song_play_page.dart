/// 全屏歌曲播放页
///
/// ── 布局 ─────────────────────────────────────────────────────────────
///   ┌──────────────────────────────────────────────────────────────┐
///   │ ⌄ 关闭            歌名 - 艺术家                              │
///   ├───────────────────────────┬──────────────────────────────────┤
///   │        环形频谱            │                                  │
///   │      ┌──────────┐         │        歌词（滚动 + 点击跳转）     │
///   │      │ 旋转封面  │         │                                  │
///   │      └──────────┘         │                                  │
///   ├───────────────────────────┴──────────────────────────────────┤
///   │  00:42 ───────●────────────────────────── 04:05              │
///   │              ◀◀    ▶    ▶▶                                   │
///   └──────────────────────────────────────────────────────────────┘
///
/// ── 三个数据源 ───────────────────────────────────────────────────────
///   PlaybackProvider  当前歌曲 / 进度 / 播放状态
///   LyricProvider     歌词（后端解析好的时间轴，含翻译）
///   FfiSpectrumSource 实时频谱（共享内存 → FFI），只在页面打开时创建
///
/// ── 交互 ─────────────────────────────────────────────────────────────
///   · 点播放栏的封面打开，点左上角 ⌄ 关闭（滑动动画）
///   · 播放时封面旋转，暂停时停住
///   · 点歌词任意一行 → 跳到那一行
///   · 关闭页面会 dispose 频谱数据源，后端随之停掉 FFT
library;

import 'dart:math' as math;
import 'dart:ui' show ImageFilter;

import 'package:audio_video_progress_bar/audio_video_progress_bar.dart';
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/song_basic_info.dart';
import 'package:junk_fusion_ui/providers/lyric_provider.dart';
import 'package:junk_fusion_ui/providers/play_page_provider.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/spectrum_source.dart';
import 'package:junk_fusion_ui/widgets/PlayControl/lyric_view.dart';
import 'package:junk_fusion_ui/widgets/PlayControl/spectrum_ring.dart';
import 'package:junk_fusion_ui/widgets/pages/single_album_page.dart';
import 'package:provider/provider.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

class SongPlayPage extends StatefulWidget {
  const SongPlayPage({super.key});

  @override
  State<SongPlayPage> createState() => _SongPlayPageState();
}

class _SongPlayPageState extends State<SongPlayPage>
    with TickerProviderStateMixin {
  /// 页面上滑 / 下滑
  late final AnimationController _slide;

  /// 封面旋转（播放时转，暂停时停）
  late final AnimationController _rotate;

  FfiSpectrumSource? _spectrum;
  int? _loadedSongId;
  bool _wasOpen = false;
  bool _wasPlaying = false;

  @override
  void initState() {
    super.initState();

    _slide = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 420),
    );

    _rotate = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 24),
    );
  }

  @override
  void dispose() {
    _spectrum?.dispose();
    _slide.dispose();
    _rotate.dispose();
    super.dispose();
  }

  /// 处理打开/关闭、切歌、播放状态变化带来的副作用
  ///
  /// 必须在**帧后**调用：这里会启停动画、创建/释放频谱源、让 LyricProvider
  /// 发通知，任何一件在 build 期间做都会触发 "markNeedsBuild during build"。
  void _syncSideEffects(bool isOpen, int? songId, bool isPlaying) {
    // 打开/关闭：滑动动画 + 频谱源的创建/销毁
    if (isOpen != _wasOpen) {
      _wasOpen = isOpen;
      if (isOpen) {
        _spectrum ??= FfiSpectrumSource();
        _slide.forward();
      } else {
        _slide.reverse();
        _spectrum?.dispose();
        _spectrum = null;
      }
    }

    // 切歌：拉一次歌词
    if (songId != _loadedSongId) {
      _loadedSongId = songId;
      context.read<LyricProvider>().loadForSong(songId);
    }

    // 播放状态：控制封面转不转
    if (isPlaying != _wasPlaying) {
      _wasPlaying = isPlaying;
      if (isPlaying) {
        if (!_rotate.isAnimating) _rotate.repeat();
      } else {
        if (_rotate.isAnimating) _rotate.stop();
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final playPage = context.watch<PlayPageProvider>();
    final playback = context.watch<PlaybackProvider>();
    final song = playback.curBasicInfo;

    // 有变化才排一次帧后回调（不每次都排）
    if (playPage.isOpen != _wasOpen ||
        song?.songId != _loadedSongId ||
        playback.isPlaying != _wasPlaying) {
      final isOpen = playPage.isOpen;
      final songId = song?.songId;
      final isPlaying = playback.isPlaying;
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (!mounted) return;
        _syncSideEffects(isOpen, songId, isPlaying);
      });
    }

    return AnimatedBuilder(
      animation: _slide,
      builder: (context, _) {
        // 完全关闭时不参与布局，也不吃点击
        if (_slide.value == 0.0 || song == null) {
          return const SizedBox.shrink();
        }

        return SlideTransition(
          position: Tween<Offset>(begin: const Offset(0, 1), end: Offset.zero)
              .animate(
                CurvedAnimation(parent: _slide, curve: Curves.easeOutCubic),
              ),
          child: _buildPage(context, song),
        );
      },
    );
  }

  Widget _buildPage(BuildContext context, SongBasicInfo song) {
    final theme = context.watch<AppTheme>();

    return ColoredBox(
      color: theme.colorMain,
      child: Stack(
        children: [
          // ── 背景：模糊封面 + 主题色渐变压暗 ──
          Positioned.fill(child: _buildBackground(song.hash, theme)),

          // ── 内容 ──
          SafeArea(
            child: Column(
              children: [
                _buildTopBar(context, song, theme),
                Expanded(
                  child: Row(
                    children: [
                      Expanded(flex: 5, child: _buildCoverArea(song, theme)),
                      Expanded(flex: 5, child: _buildLyricArea(theme)),
                    ],
                  ),
                ),
                _buildBottomBar(context, theme),
              ],
            ),
          ),
        ],
      ),
    );
  }

  // ── 背景 ────────────────────────────────────────────────────────────
  Widget _buildBackground(String? hash, AppTheme theme) {
    return Stack(
      fit: StackFit.expand,
      children: [
        // 封面放大 + 重模糊，直接当背景用（不用再算主题色）
        ImageFiltered(
          imageFilter: ImageFilter.blur(
            sigmaX: 80,
            sigmaY: 80,
            tileMode: TileMode.clamp,
          ),
          child: AlbumCoverImage(hash: hash),
        ),
        // 上淡下实：保证文字可读
        DecoratedBox(
          decoration: BoxDecoration(
            gradient: LinearGradient(
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter,
              colors: [
                theme.colorMain.withValues(alpha: 0.55),
                theme.colorMain.withValues(alpha: 0.82),
                theme.colorMain,
              ],
            ),
          ),
        ),
      ],
    );
  }

  // ── 顶栏 ────────────────────────────────────────────────────────────
  Widget _buildTopBar(
    BuildContext context,
    SongBasicInfo song,
    AppTheme theme,
  ) {
    final artists = song.artists == null || song.artists!.isEmpty
        ? '未知艺术家'
        : song.artists!.join(' / ');

    return SizedBox(
      height: 56,
      child: Row(
        children: [
          const SizedBox(width: 8),
          IconButton(
            icon: const Icon(TablerIcons.chevronDown, size: 30),
            tooltip: '收起',
            color: theme.colorTextMain,
            onPressed: () => context.read<PlayPageProvider>().close(),
          ),
          const SizedBox(width: 8),
          Expanded(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  song.title,
                  style: theme.midTextStyle.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
                Text(
                  artists,
                  style: theme.littleTextStyle,
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
              ],
            ),
          ),
          const SizedBox(width: 16),
        ],
      ),
    );
  }

  // ── 封面 + 环形频谱 ─────────────────────────────────────────────────
  Widget _buildCoverArea(SongBasicInfo song, AppTheme theme) {
    return LayoutBuilder(
      builder: (context, constraints) {
        final shortest = math.min(constraints.maxWidth, constraints.maxHeight);
        final coverSize = (shortest * 0.62).clamp(120.0, 420.0);
        final ringRadius = coverSize / 2 + 16;
        final spectrum = _spectrum;

        return Center(
          child: SizedBox(
            width: coverSize + 120,
            height: coverSize + 120,
            child: Stack(
              alignment: Alignment.center,
              children: [
                // 环形频谱在封面外圈，不跟着转
                if (spectrum != null)
                  SpectrumRing(
                    bands: spectrum.bands,
                    peaks: spectrum.peaks,
                    radius: ringRadius,
                    color: theme.colorStress,
                    maxBarLength: 38,
                    barWidth: 3,
                  ),

                // 旋转封面
                ClipOval(
                  child: SizedBox(
                    width: coverSize,
                    height: coverSize,
                    child: RotationTransition(
                      turns: _rotate,
                      child: AlbumCoverImage(hash: song.hash),
                    ),
                  ),
                ),
              ],
            ),
          ),
        );
      },
    );
  }

  // ── 歌词 ────────────────────────────────────────────────────────────
  Widget _buildLyricArea(AppTheme theme) {
    final lyrics = context.watch<LyricProvider>().lyrics;
    final playback = context.watch<PlaybackProvider>();

    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 8),
      child: LyricView(
        lyrics: lyrics,
        positionSeconds: playback.currentTimeStamp,
        onSeekToLine: (index) {
          if (lyrics == null || index < 0 || index >= lyrics.lines.length) {
            return;
          }
          final target = lyrics.lines[index].startMs / 1000.0;
          context.read<PlaybackProvider>().seekPreferPTS(target);
        },
      ),
    );
  }

  // ── 底部：进度条 + 控制按钮 ─────────────────────────────────────────
  Widget _buildBottomBar(BuildContext context, AppTheme theme) {
    final playback = context.watch<PlaybackProvider>();
    final song = playback.curBasicInfo;
    if (song == null) return const SizedBox.shrink();

    return Padding(
      padding: const EdgeInsets.fromLTRB(40, 0, 40, 24),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          ProgressBar(
            progress: Duration(
              milliseconds: (playback.currentTimeStamp * 1000).toInt(),
            ),
            total: Duration(milliseconds: (song.duration * 1000).toInt()),
            onSeek: (value) {
              context.read<PlaybackProvider>().seekPreferPTS(
                value.inMilliseconds.toDouble() / 1000.0,
              );
            },
            timeLabelLocation: TimeLabelLocation.sides,
            timeLabelTextStyle: theme.littleTextStyle,
            barHeight: 5,
            thumbRadius: 6,
            baseBarColor: theme.colorEdge,
            bufferedBarColor: theme.colorHover,
            progressBarColor: theme.colorStress,
            thumbColor: theme.colorStress,
            thumbGlowColor: theme.colorStress.withValues(alpha: 0.3),
          ),
          const SizedBox(height: 8),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              IconButton(
                icon: Icon(
                  TablerIcons.squareArrowLeftFilled,
                  size: 34,
                  color: theme.colorTextMain,
                ),
                onPressed: () =>
                    context.read<PlaybackProvider>().playPreviousSong(),
              ),
              const SizedBox(width: 28),
              IconButton(
                icon: Icon(
                  playback.isPlaying
                      ? TablerIcons.playerPauseFilled
                      : TablerIcons.playerPlayFilled,
                  size: 46,
                  color: theme.colorTextMain,
                ),
                onPressed: () =>
                    context.read<PlaybackProvider>().togglePlayPause(),
              ),
              const SizedBox(width: 28),
              IconButton(
                icon: Icon(
                  TablerIcons.squareArrowRightFilled,
                  size: 34,
                  color: theme.colorTextMain,
                ),
                onPressed: () =>
                    context.read<PlaybackProvider>().playNextSong(),
              ),
            ],
          ),
        ],
      ),
    );
  }
}
