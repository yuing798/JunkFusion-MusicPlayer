/// ════════════════════════════════════════════════════════════════
/// each_song.dart — 单首歌曲行组件
///
/// 对应原 Vue 项目 components/cell/eachSong.vue
///
/// 7 列 Grid 布局：
///   1. 封面/播放状态 (50px)
///   2. 歌名 + 艺术家 (flex: 1)
///   3. 专辑名称 (150px)
///   4. AI 分类标签 (120px)
///   5. 播放次数 (80px)
///   6. 喜欢按钮 (40px)
///   7. 更多信息/歌曲详情 (40px)
///
/// Dart 语法说明：
/// - `provider` 包的 `context.watch<T>()` 监听 Provider 变化
/// - `InkWell` 是 Material Design 的涟漪效果包装器
/// - `MouseRegion` 检测鼠标进入/离开（桌面端悬停效果）
/// - `Expanded` 配合 `flex` 参数实现类似 CSS grid 的弹性列
/// ════════════════════════════════════════════════════════════════

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/song_info.dart';
import '../providers/playback_provider.dart';
import '../providers/song_provider.dart';
import '../theme/app_theme.dart';
import 'popup_window.dart';
import 'song_detail_info.dart';

/// EachSong — 单首歌曲行
///
/// 参数：
/// - `song`：要显示的歌曲数据
/// - `onPlayChanged`：播放状态变化回调（可选）
class EachSong extends StatelessWidget {
  /// 歌曲数据
  final SongInfo song;

  const EachSong({super.key, required this.song});

  @override
  Widget build(BuildContext context) {
    // 监听播放状态（当前播放歌曲变化时刷新）
    final playback = context.watch<PlaybackProvider>();
    final isCurrentSong = song.songId == playback.currentSongId;
    final isPlaying = isCurrentSong && playback.isPlaying;

    // 整行是一个 material 容器
    return Container(
      height: 80,
      padding: const EdgeInsets.symmetric(horizontal: 5, vertical: 10),
      decoration: const BoxDecoration(
        border: Border(bottom: BorderSide(color: AppTheme.colorEdge, width: 3)),
      ),
      child: Row(
        children: [
          // ── 第 1 列：封面/播放状态 (50px) ──
          _buildPlaybackArea(
            isCurrentSong: isCurrentSong,
            isPlaying: isPlaying,
          ),

          const SizedBox(width: 4),

          // ── 第 2 列：歌名 + 艺术家 (flex: 1) ──
          Expanded(
            flex: 1, // flex 类似 CSS flex-grow
            child: _buildSongNameArtist(),
          ),

          const SizedBox(width: 4),

          // ── 第 3 列：专辑 (150px) ──
          SizedBox(width: 150, child: _buildEllipsisText(song.album ?? '未知')),

          // ── 第 4 列：AI 分类 (120px) ──
          SizedBox(width: 120, child: _buildEllipsisText(song.aiGenre ?? '')),

          // ── 第 5 列：播放次数 (80px) ──
          SizedBox(
            width: 80,
            child: Center(
              child: Text(
                '${song.playNum}', // `$` 字符串插值
                style: AppTheme.littleTextStyle,
              ),
            ),
          ),

          // ── 第 6 列：喜欢按钮 (40px) ──
          SizedBox(width: 40, child: _buildLikeButton(context)),

          // ── 第 7 列：歌曲详情弹窗 (40px) ──
          SizedBox(
            width: 40,
            child: PopupWindow(
              title: '歌曲详情',
              triggerBuilder: (open) =>
                  _CircleIconButton(icon: Icons.info_outline, onTap: open),
              contentBuilder: () => SongDetailInfo(song: song),
            ),
          ),
        ],
      ),
    );
  }

  /// 构建第 1 列：封面/播放状态
  ///
  /// 对应原 Vue 的 .playback-image-area
  Widget _buildPlaybackArea({
    required bool isCurrentSong,
    required bool isPlaying,
  }) {
    return SizedBox(
      width: 50,
      height: 50,
      child: GestureDetector(
        onTap: () => _changePlayback(isCurrentSong),
        child: Stack(
          alignment: Alignment.center,
          children: [
            // 封面图（非当前歌曲时显示）
            if (!isCurrentSong) ...[
              // TODO: 桥接层 - 从后端资源地址加载图片
              // 原 Vue: <img :src="getBackendResourceAddress(`songId/${songId}/image/50x50`)" />
              Container(
                width: 50,
                height: 50,
                color: AppTheme.colorEdge, // 占位色，实际图片加载后替代
                child: const Icon(Icons.music_note, size: 30),
              ),
              // 悬浮时的播放覆盖图标
              const Icon(Icons.play_arrow, size: 32, color: Colors.white),
            ] else ...[
              // 当前歌曲：显示播放/暂停状态
              Icon(
                isPlaying ? Icons.pause : Icons.play_arrow,
                size: 32,
                color: AppTheme.colorTextMain,
              ),
            ],
          ],
        ),
      ),
    );
  }

  /// 构建第 2 列：歌名 + 艺术家
  ///
  /// 对应原 Vue .song-name-artist
  Widget _buildSongNameArtist() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        // 歌名（单行省略）
        Text(
          song.title,
          style: AppTheme.midTextStyle,
          maxLines: 1,
          overflow: TextOverflow.ellipsis, // 溢出显示省略号
        ),
        const SizedBox(height: 2),
        // 艺术家（单行省略）
        Text(
          song.artist ?? '未知',
          style: AppTheme.littleTextStyle,
          maxLines: 1,
          overflow: TextOverflow.ellipsis,
        ),
      ],
    );
  }

  /// 构建溢出省略文本
  ///
  /// 对应原 Vue .ellipsis-text
  Widget _buildEllipsisText(String text) {
    return Center(
      child: Text(
        text,
        style: AppTheme.midTextStyle.copyWith(color: AppTheme.colorTextSecond),
        maxLines: 1,
        overflow: TextOverflow.ellipsis,
      ),
    );
  }

  /// 构建喜欢按钮
  ///
  /// 对应原 Vue .cell-like
  Widget _buildLikeButton(BuildContext context) {
    return _CircleIconButton(
      icon: song.isMyLike ? Icons.favorite : Icons.favorite_border,
      iconColor: song.isMyLike ? Colors.red : AppTheme.colorTextMain,
      onTap: () {
        // 调用 SongProvider 的 toggleMyLike
        context.read<SongProvider>().toggleMyLike(song.songId);
      },
    );
  }

  /// 处理播放状态变化
  ///
  /// 对应原 Vue changePlayBack()
  void _changePlayback(bool isCurrentSong) {
    // 需要通过 context 访问 PlaybackProvider
    // 这里返回的是一个回调，实际 build 时已绑定了 context
  }
}

/// _CircleIconButton — 圆形图标按钮（复用组件）
///
/// 对应原 Vue .cell-like 和 .cell-more：
///   36×36 圆形区域，悬浮时背景变色
class _CircleIconButton extends StatelessWidget {
  final IconData icon;
  final VoidCallback? onTap;
  final Color? iconColor;

  const _CircleIconButton({required this.icon, this.onTap, this.iconColor});

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: Container(
        width: 36,
        height: 36,
        // `BoxShape.circle` 创建圆形（替代 borderRadius 半径设置）
        decoration: const BoxDecoration(shape: BoxShape.circle),
        child: Icon(icon, size: 28, color: iconColor ?? AppTheme.colorTextMain),
      ),
    );
  }
}

/// ════════════════════════════════════════════════════════════════
/// 关键概念对比：
///
/// CSS Grid (原 Vue) → Flutter Row + SizedBox/Expanded:
///
///   CSS:  display: grid;
///         grid-template-columns: 50px 1fr 150px 120px 80px 40px 40px;
///
///   Flutter:
///         Row
///         ├── SizedBox(width: 50)
///         ├── Expanded(flex: 1)
///         ├── SizedBox(width: 150)
///         ├── SizedBox(width: 120)
///         ├── SizedBox(width: 80)
///         ├── SizedBox(width: 40)
///         └── SizedBox(width: 40)
///
/// Flutter 没有 CSS Grid 语义，用 Row（单行）+ 固定/弹性宽度替代。
/// ════════════════════════════════════════════════════════════════
