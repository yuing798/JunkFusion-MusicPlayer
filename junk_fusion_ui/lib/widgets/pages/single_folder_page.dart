import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/song_basic_info.dart';
import 'package:junk_fusion_ui/providers/folder_provider.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/widgets/each_song.dart';
import 'package:junk_fusion_ui/widgets/popup_window.dart';
// AlbumCoverImage（哈希 → image/<hash>/original.jpg）目前住在
// single_album_page.dart 里，先复用；以后抽到 widgets/ 下面更合适
import 'package:junk_fusion_ui/widgets/pages/single_album_page.dart';
import 'package:provider/provider.dart';
import 'package:smooth_scroll_multiplatform/smooth_scroll_multiplatform.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

// ════════════════════════════════════════════════════════════════
// single_folder_page.dart — 单个文件夹的内容页
//
// 由 FolderViewPage 点卡片进来：那边会先 FolderProvider.setNewFolder(path)，
// 再往路由栈里压一条记录，所以这一页自己不接参数，
// 直接读 FolderProvider 里的 curBasicInfo / songsInFolder。
// 返回交给路由左上角那个按钮。
//
// 布局（自上而下）：
//   第 1 行（60px）：封面/文件夹图标 + 文件夹名 +「路径 · 共 N 首」
//   第 2 行（80px）：常用操作（播放全部 / 添加到播放列表）
//   第 3 部分：歌曲列表（复用 EachSong，左边补一列音轨号）
//
// TODO(后端): 文件夹接口还没做（FolderProvider 里全是 TODO），
//             所以这一页现在会显示空状态。
// ════════════════════════════════════════════════════════════════

class SingleFolderPage extends StatefulWidget {
  const SingleFolderPage({super.key});

  @override
  State<SingleFolderPage> createState() => _SingleFolderPageState();
}

class _SingleFolderPageState extends State<SingleFolderPage> {
  /// 播放全部：按当前顺序把整个文件夹塞进播放列表，从第一首开始放
  void _playAll(List<SongBasicInfo> songs) {
    if (songs.isEmpty) return;

    final playback = context.read<PlaybackProvider>();
    // 先设置播放列表再切歌：setNewSong 内部会把当前播放列表缓存下来
    playback.setPlayList(songs);
    playback.setNewSong(songs.first.songId);
  }

  /// 添加到播放列表：把文件夹里的歌追加到现有播放列表后面（按 songId 去重）
  ///
  /// 这个功能只用前端数据，不需要后端接口
  void _appendToPlayList(List<SongBasicInfo> songs) {
    if (songs.isEmpty) return;

    final playback = context.read<PlaybackProvider>();
    final currentList = playback.playList;

    final existingIds = currentList.map((song) => song.songId).toSet();
    final merged = List<SongBasicInfo>.of(currentList);
    for (final song in songs) {
      // Set.add 返回 false 说明这首歌已经在列表里了
      if (existingIds.add(song.songId)) merged.add(song);
    }

    final addedCount = merged.length - currentList.length;
    playback.setPlayList(merged);

    DialogUtil.showInfoDialog(
      addedCount == 0 ? '这个文件夹的歌曲都已经在播放列表里了' : '已把 $addedCount 首歌曲添加到播放列表',
    );
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();

    // watch：歌曲状态（比如「我喜欢」、播放次数）变了列表要跟着重画
    context.watch<SongProvider>();
    final folderProvider = context.watch<FolderProvider>();
    final songs = folderProvider.songsInFolder;

    final basicInfo = folderProvider.curBasicInfo;
    if (basicInfo == null) {
      return const SizedBox.shrink();
    }

    final hash = basicInfo.hash;
    final hasCurrentSong = context.select<PlaybackProvider, bool>(
      (provider) => provider.curBasicInfo != null,
    );

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // ── 第 1 行：封面/图标 + 文件夹名 ──
        SizedBox(
          height: 60,
          child: Row(
            children: [
              // 封面（这个文件夹里第一首有封面的歌；没有就画文件夹图标）
              SizedBox(
                width: 50,
                height: 50,
                child: (hash == null || hash.isEmpty)
                    ? Center(
                        child: Icon(
                          TablerIcons.folderFilled,
                          size: 40,
                          color: theme.colorEdge,
                        ),
                      )
                    : AlbumCoverImage(hash: hash),
              ),

              const SizedBox(width: 10),

              // 文件夹名 + 一行元信息（路径 + 歌曲数）
              Expanded(
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Tooltip(
                      message: basicInfo.path,
                      child: Text(
                        basicInfo.name,
                        style: theme.bigTextStyle,
                        maxLines: 1,
                        overflow: TextOverflow.ellipsis,
                      ),
                    ),
                    const SizedBox(height: 2),
                    Text(
                      '${basicInfo.path} · 共 ${songs.length} 首',
                      style: theme.littleTextStyle,
                      maxLines: 1,
                      overflow: TextOverflow.ellipsis,
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),

        // ── 第 2 行：常用按钮 ──
        SizedBox(
          height: 80,
          child: Row(
            children: [
              // ── 播放全部（主按钮，用强调色） ──
              _FolderActionButton(
                icon: TablerIcons.playerPlayFilled,
                label: '播放全部',
                isPrimary: true,
                onTap: songs.isEmpty ? null : () => _playAll(songs),
              ),

              const SizedBox(width: 10),

              // ── 添加到播放列表 ──
              _FolderActionButton(
                icon: TablerIcons.playlistAdd,
                label: '添加到播放列表',
                onTap: songs.isEmpty ? null : () => _appendToPlayList(songs),
              ),

              const Spacer(),
            ],
          ),
        ),

        // ── 第 3 部分：歌曲列表 ──
        Expanded(
          child: songs.isEmpty
              ? Center(child: Text('这个文件夹里还没有歌曲', style: theme.littleTextStyle))
              : DynMouseScroll(
                  builder: (context_, controler_, physics_) {
                    return ListView.builder(
                      controller: controler_,
                      physics: physics_,
                      itemCount: songs.length,
                      // 固定行高（和 EachSong 的 80 对齐，性能优化）
                      itemExtent: 80,
                      padding: EdgeInsets.only(bottom: hasCurrentSong ? 90 : 0),
                      itemBuilder: (context, index) {
                        final song = songs[index];

                        return Row(
                          key: ValueKey(song.songId),
                          children: [
                            // ── 音轨号（没写音轨号就退回列表序号） ──
                            SizedBox(
                              width: 36,
                              child: Center(
                                child: Text(
                                  "a",
                                  // song.trackNumber?.toString() ??
                                  //     '${index + 1}',
                                  style: theme.littleTextStyle,
                                ),
                              ),
                            ),

                            // ── 歌曲行（复用「所有音乐」页面的 EachSong） ──
                            Expanded(
                              child: EachSong(
                                song: song,
                                onSongPlay: () {
                                  // 点封面播放时，把「整个文件夹」当成播放列表
                                  final playback = context
                                      .read<PlaybackProvider>();
                                  playback.setPlayList(songs);
                                  playback.saveState();
                                },
                              ),
                            ),
                          ],
                        );
                      },
                    );
                  },
                ),
        ),
      ],
    );
  }
}

// ════════════════════════════════════════════════════════════════
// _FolderActionButton — 文件夹页工具栏上的按钮
//
// 样式和「所有音乐」页面的「导入文件」按钮、专辑页的按钮保持一致：
// 直角矩形、悬浮色背景、midTextStyle 文字。
// ════════════════════════════════════════════════════════════════
class _FolderActionButton extends StatelessWidget {
  final IconData icon;
  final String label;
  final VoidCallback? onTap;

  /// 主按钮（播放全部）用强调色背景，其他的用悬浮色
  final bool isPrimary;

  const _FolderActionButton({
    required this.icon,
    required this.label,
    this.onTap,
    this.isPrimary = false,
  });

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();

    return SizedBox(
      height: 40,
      child: TextButton(
        // onTap 为 null 时 TextButton 自动进入禁用态
        onPressed: onTap,
        style: TextButton.styleFrom(
          backgroundColor: isPrimary ? theme.colorStress : theme.colorHover,
          foregroundColor: theme.colorTextMain,
          shape: const RoundedRectangleBorder(
            borderRadius: BorderRadius.zero, // 直角矩形
          ),
          padding: const EdgeInsets.symmetric(horizontal: 12),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(icon, size: 20),
            const SizedBox(width: 6),
            Text(label, style: theme.midTextStyle),
          ],
        ),
      ),
    );
  }
}
