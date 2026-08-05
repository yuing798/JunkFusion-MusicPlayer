/// ════════════════════════════════════════════════════════════════
/// play_bar.dart — 底部播放栏
///
/// 对应原 Vue 项目 components/playBar.vue
///
/// 布局（固定底部 90px）：
///   Grid 三区: left-area (500px) | spacer | mid-area (330px) | spacer | right-area (550px)
///   简化为 Row: left | Expanded | center | Expanded | right
///
/// 左区：封面 50×50 + 歌名/艺术家 + 喜欢按钮 + 歌曲详情弹窗
/// 中区：播放模式切换 + 上一首 + 播放/暂停 + 下一首 + 播放列表
/// 右区：（预留，当前为空）
///
/// Dart 语法说明：
/// - `Consumer<PlaybackProvider>` 局部监听 Provider，只重建必要部分
/// - `context.watch` 在 build 中监听多个 Provider
/// - 图标使用 Flutter 内置 Material Icons 替代 @tabler/icons-vue
/// - 播放模式 0-3 循环切换
/// ════════════════════════════════════════════════════════════════

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../model/song_info.dart';
import '../providers/playback_provider.dart';
import '../providers/song_provider.dart';
import '../theme/app_theme.dart';
import 'popup_window.dart';
import 'song_detail_info.dart';

/// PlayBar — 底部播放栏
///
/// 这是一个 const 构造函数 widget（StatelessWidget）
class PlayBar extends StatelessWidget {
  const PlayBar({super.key});

  @override
  Widget build(BuildContext context) {
    return Container(
      height: 90,
      padding: const EdgeInsets.symmetric(horizontal: 30, vertical: 10),
      color: AppTheme.colorHover,

      // Row 布局（简化为三区）
      child: Row(
        children: [
          // ── 左区：500px（封面 + 歌名/艺术家 + 喜欢 + 详情） ──
          SizedBox(width: 500, child: _buildLeftArea(context)),

          // 弹性空间
          const Expanded(child: SizedBox()),

          // ── 中区：330px（播放控制） ──
          SizedBox(width: 330, child: _buildMidArea(context)),

          // 弹性空间
          const Expanded(child: SizedBox()),

          // ── 右区：550px（预留） ──
          const SizedBox(width: 550),
        ],
      ),
    );
  }

  /// 构建左区：封面 + 歌名/艺术家 + 喜欢 + 详情弹窗
  ///
  /// 对应原 Vue .left-area
  Widget _buildLeftArea(BuildContext context) {
    // Consumer 只重建包裹的部分（局部重建）
    return Consumer2<PlaybackProvider, SongProvider>(
      builder: (context, playback, songStore, child) {
        // 查找当前播放歌曲的信息
        final info = songStore.getSongInfo(playback.currentSongId!);

        return Row(
          children: [
            // ── 封面图 50×50 ──
            // TODO: 桥接层 - 从后端资源地址加载封面图
            // 原 Vue: <img :src="getBackendResourceAddress(`songId/${playBackStore.currentSongId}/image/50x50`)" />
            Container(
              width: 50,
              height: 50,
              color: AppTheme.colorEdge,
              child: const Icon(Icons.music_note, size: 30),
            ),

            const SizedBox(width: 10),

            // ── 歌名 + 艺术家 ──
            Expanded(
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    info?.title ?? '',
                    style: AppTheme.midTextStyle,
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                  Text(
                    info?.artist ?? '未知',
                    style: AppTheme.littleTextStyle,
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                ],
              ),
            ),

            const SizedBox(width: 10),

            // ── 喜欢按钮 ──
            if (info != null)
              GestureDetector(
                onTap: () {
                  songStore.toggleMyLike(info.songId);
                },
                child: Container(
                  width: 36,
                  height: 36,
                  decoration: const BoxDecoration(shape: BoxShape.circle),
                  child: Icon(
                    info.isMyLike ? Icons.favorite : Icons.favorite_border,
                    size: 28,
                    color: info.isMyLike ? Colors.red : AppTheme.colorTextMain,
                  ),
                ),
              ),

            const SizedBox(width: 10),

            // ── 歌曲详情弹窗 ──
            if (info != null)
              PopupWindow(
                title: '歌曲详情',
                triggerBuilder: (open) => GestureDetector(
                  onTap: open,
                  child: Container(
                    width: 36,
                    height: 36,
                    decoration: const BoxDecoration(shape: BoxShape.circle),
                    child: const Icon(
                      Icons.info_outline,
                      size: 28,
                      color: AppTheme.colorTextMain,
                    ),
                  ),
                ),
                contentBuilder: () => SongDetailInfo(song: info),
              ),
          ],
        );
      },
    );
  }

  /// 构建中区：播放模式 + 播放控制
  ///
  /// 对应原 Vue .mid-area
  Widget _buildMidArea(BuildContext context) {
    final playback = context.watch<PlaybackProvider>();

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
          icon: const Icon(Icons.skip_previous, size: 32),
          onPressed: () {
            // TODO: 桥接层 - 调用上一首
          },
          color: AppTheme.colorTextMain,
          padding: EdgeInsets.zero,
          constraints: const BoxConstraints(minWidth: 32, minHeight: 32),
        ),

        // 播放 / 暂停
        IconButton(
          icon: Icon(
            playback.isPlaying
                ? Icons.pause_circle_filled
                : Icons.play_circle_filled,
            size: 40,
          ),
          onPressed: () {
            context.read<PlaybackProvider>().togglePlayPause();
          },
          color: AppTheme.colorTextMain,
          padding: EdgeInsets.zero,
          constraints: const BoxConstraints(minWidth: 40, minHeight: 40),
        ),

        // 下一首
        IconButton(
          icon: const Icon(Icons.skip_next, size: 32),
          onPressed: () {
            // TODO: 桥接层 - 调用下一首
          },
          color: AppTheme.colorTextMain,
          padding: EdgeInsets.zero,
          constraints: const BoxConstraints(minWidth: 32, minHeight: 32),
        ),

        const SizedBox(width: 10),

        // 播放列表
        IconButton(
          icon: const Icon(Icons.queue_music, size: 32),
          onPressed: () {
            // TODO: 桥接层 - 打开播放列表
          },
          color: AppTheme.colorTextMain,
          padding: EdgeInsets.zero,
          constraints: const BoxConstraints(minWidth: 32, minHeight: 32),
        ),
      ],
    );
  }
}

/// _PlayModeIcon — 播放模式图标组件
///
/// 根据 mode 值显示不同图标：
///   0 = 顺序播放 (arrow_right_alt)
///   1 = 列表循环 (repeat)
///   2 = 单曲循环 (repeat_one)
///   3 = 随机播放 (shuffle)
///
/// 对应原 Vue 中 v-if/v-else-if 的图标切换逻辑
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
        icon = Icons.arrow_right_alt; // 顺序播放
        tooltip = '顺序播放';
      case 1:
        icon = Icons.repeat; // 列表循环
        tooltip = '列表循环';
      case 2:
        icon = Icons.repeat_one; // 单曲循环
        tooltip = '单曲循环';
      case 3:
        icon = Icons.shuffle; // 随机播放
        tooltip = '随机播放';
      default:
        icon = Icons.arrow_right_alt;
        tooltip = '未知';
    }

    return IconButton(
      icon: Icon(icon, size: 32),
      onPressed: onTap,
      color: AppTheme.colorTextMain,
      tooltip: tooltip, // 悬浮时显示提示文字
      padding: EdgeInsets.zero,
      constraints: const BoxConstraints(minWidth: 32, minHeight: 32),
    );
  }
}

/// ════════════════════════════════════════════════════════════════
/// Consumer2 说明：
///
/// `Consumer2<A, B>` 是 provider 包提供的便捷 Widget，
/// 可以同时监听两个 Provider 的变化。
///
/// 一般形式：
/// ```dart
/// Consumer2<ProviderA, ProviderB>(
///   builder: (context, a, b, child) {
///     return Text('${a.value} ${b.value}');
///   },
/// )
/// ```
///
/// 等价于嵌套 Consumer:
/// ```dart
/// Consumer<ProviderA>(
///   builder: (context, a, child) {
///     return Consumer<ProviderB>(
///       builder: (context, b, child) {
///         return Text('${a.value} ${b.value}');
///       },
///     );
///   },
/// )
/// ```
///
/// Provider 包提供 Consumer 到 Consumer6（监听 1 到 6 个 Provider）。
/// ════════════════════════════════════════════════════════════════
