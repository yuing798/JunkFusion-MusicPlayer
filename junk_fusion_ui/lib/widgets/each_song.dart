import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/song_info.dart';
import 'package:provider/provider.dart';
import '../providers/playback_provider.dart';
import '../providers/song_provider.dart';
import '../theme/app_theme.dart';
import 'popup_window.dart';
import 'song_detail_info.dart';

// EachSong — 单首歌曲行
//
// 参数：
// - `song`：要显示的歌曲数据
// - `onPlayChanged`：播放状态变化回调（可选）
// // 7 列 Grid 布局：
//   1. 封面/播放状态 (50px)
//   2. 歌名 + 艺术家 (flex: 1)
//   3. 专辑名称 (150px)
//   4. AI 分类标签 (120px)
//   5. 播放次数 (80px)
//   6. 喜欢按钮 (40px)
//   7. 更多信息/歌曲详情 (40px)
// //StatefulWidget 拥有可以“随时间变化”的内部状态（State），而 StatelessWidget 的所有数据都是外部传入且永远不变的。
class EachSong extends StatelessWidget {
  // 歌曲数据
  final SongInfo song;

  const EachSong({super.key, required this.song});

  @override
  Widget build(BuildContext context) {
    // 监听播放状态（当前播放歌曲变化时刷新）
    final playback = context.watch<PlaybackProvider>();
    //context.watch<T>()：获取 + 订阅。当数据变化时，调用它的 Widget 会自动重绘
    //context.read<T>()：仅获取，不订阅。调用后拿到实例，但数据变时 Widget 不会重绘。
    final isCurrentSong = playback.currentSongId == song.songId;

    return Container(
      height: 80,
      padding: const EdgeInsets.symmetric(horizontal: 5, vertical: 10),
      color: isCurrentSong ? AppTheme.colorHover : AppTheme.colorCell,
      child: Row(
        children: [
          // ── 第 1 列：封面/播放状态 (50px) ──
          SizedBox(
            //sizedBox只能标注尺寸功能
            width: 50,
            height: 50,
            child: GestureDetector(
              onTap: () {
                //箭头函数后面只能接上一句表达式
                if (isCurrentSong) {
                  playback.togglePlayPause();
                } else {
                  playback.setPlayState(song.songId);
                }
              },
              child: Stack(
                //Stack 是 Flutter 中的层叠布局（Stack Layout）组件，它允许你将子组件重叠放置，
                //像叠罗汉一样，后添加的子组件会覆盖在先添加的上面。
                alignment: Alignment.center,
                children: [
                  // 封面图（非当前歌曲时显示）
                  if (!isCurrentSong) ...[
                    // TODO: 桥接层 - 从后端资源地址加载图片
                    // 原 Vue: <img :src="getBackendResourceAddress(`songId/${songId}/image/50x50`)" />
                    const Icon(Icons.play_arrow, size: 32, color: Colors.white),
                  ] else ...[
                    // 当前歌曲：显示播放/暂停状态
                  ],
                ],
              ),
            ),
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

  // 构建第 2 列：歌名 + 艺术家
  //
  // 对应原 Vue .song-name-artist
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

  // 构建溢出省略文本
  //
  // 对应原 Vue .ellipsis-text
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

  // 构建喜欢按钮
  //
  // 对应原 Vue .cell-like
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
}

// _CircleIconButton — 圆形图标按钮（复用组件）
//
// 对应原 Vue .cell-like 和 .cell-more：
//   36×36 圆形区域，悬浮时背景变色
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
