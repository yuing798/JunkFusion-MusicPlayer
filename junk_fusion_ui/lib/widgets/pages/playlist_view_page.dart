import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:junk_fusion_ui/model/playlist_basic_info.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/providers/playlist_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/app_cache.dart';
import 'package:junk_fusion_ui/widgets/combo_box.dart';
import 'package:junk_fusion_ui/widgets/pages/pages_route.dart';
// AlbumCoverImage（哈希 → image/<hash>/original.jpg）目前住在
// single_album_page.dart 里，先复用；以后抽到 widgets/ 下面更合适
import 'package:junk_fusion_ui/widgets/pages/single_album_page.dart';
import 'package:provider/provider.dart';
import 'package:smooth_scroll_multiplatform/smooth_scroll_multiplatform.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

// ════════════════════════════════════════════════════════════════
// playlist_view_page.dart — 歌单视图（网格）
//
// 布局（自上而下）：
//   第 1 行（50px）：标题「歌单」+「共 N 个」
//   第 2 行（80px）：工具栏（新建歌单 [占位] + 排序方式下拉框 + 升降序按钮）
//   第 3 部分：歌单网格（GridView，鼠标滚轮平滑滚动）
//
// 单张卡片自上而下：封面 → 歌单名 →「共 N 首」。
// 点卡片走路由进 SinglePlaylistPage（见 pages_route.dart）。
//
// TODO(后端): 后端现在没有任何歌单接口，数据是空的（PlaylistProvider 里
//             全是 TODO），所以这一页现在只会显示空状态。
//             新建 / 改名 / 删除 / 增删歌曲这些「管理」操作，
//             等 bindings 里注册了对应函数再接（按钮先禁用占位）。
// ════════════════════════════════════════════════════════════════

class PlaylistViewPage extends StatefulWidget {
  const PlaylistViewPage({super.key});

  @override
  State<PlaylistViewPage> createState() => _PlaylistViewPageState();
}

class _PlaylistViewPageState extends State<PlaylistViewPage> {
  @override
  void initState() {
    super.initState();

    // 进页面的时候拉一遍歌单列表（后端接口还没做，现在是空列表）
    WidgetsBinding.instance.addPostFrameCallback((_) {
      context.read<PlaylistProvider>().getAllPlaylists();
    });
  }

  /// 打开某个歌单：先把歌单装进 Provider，再往路由栈里压一条记录
  void _openPlaylist(PlaylistBasicInfo playlist) {
    context.read<PlaylistProvider>().setNewPlaylist(playlist.playlistId);
    context.read<PagesRouteController>().push(
      PagesRouteTable.singlePlaylist(playlist),
    );
  }

  /// 没有歌单时的提示
  Widget _buildEmptyHint(AppTheme theme) {
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(TablerIcons.playlist, size: 64, color: theme.colorEdge),
          const SizedBox(height: 15),
          Text('还没有任何歌单', style: theme.midTextStyle),
          // const SizedBox(height: 5),
          // Text('歌单后端接口还没做，做完就能新建歌单了', style: theme.littleTextStyle),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    // watch 放在最前面：歌单列表变了、主题变了都要重画
    final theme = context.watch<AppTheme>();
    final playlists = context.watch<PlaylistProvider>().playlists.toList()
      ..sort((a, b) => a.playlistId.compareTo(b.playlistId));

    final hasCurrentSong = context.select<PlaybackProvider, bool>(
      (provider) => provider.curBasicInfo != null,
    );

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // ── 第 1 行：标题行 ──
        SizedBox(
          height: 50,
          child: Row(
            children: [
              Text('歌单', style: theme.bigTextStyle),
              const SizedBox(width: 15),
              Text('共 ${playlists.length} 个', style: theme.littleTextStyle),
            ],
          ),
        ),

        // ── 第 2 行：工具栏 ──
        SizedBox(
          height: 80,
          child: Row(
            children: [
              // ── 新建歌单 ──
              // TODO(后端): createPlaylist(title) 注册好之后把这个按钮接上：
              //   先弹一个输入框要名字 → createPlaylist → getAllPlaylists()
              //   现在后端还没有歌单接口，所以先禁用占位
              SizedBox(
                height: 40,
                child: TextButton(
                  onPressed: null, // TODO(后端): 接口好了换成新建歌单的逻辑
                  style: TextButton.styleFrom(
                    backgroundColor: theme.colorHover,
                    foregroundColor: theme.colorTextMain,
                    shape: const RoundedRectangleBorder(
                      borderRadius: BorderRadius.zero, // 直角矩形
                    ),
                  ),
                  child: Text('新建歌单', style: theme.midTextStyle),
                ),
              ),

              // 弹性空间（把排序控件推到最右边）
              const Spacer(),
            ],
          ),
        ),

        // ── 第 3 部分：歌单网格（虚拟滚动，填充剩余空间） ──
        Expanded(
          child: playlists.isEmpty
              ? _buildEmptyHint(theme)
              : DynMouseScroll(
                  builder: (context_, controler_, physics_) {
                    return GridView.builder(
                      controller: controler_,
                      physics: physics_,
                      gridDelegate:
                          const SliverGridDelegateWithMaxCrossAxisExtent(
                            maxCrossAxisExtent: 220,
                            mainAxisSpacing: 15,
                            crossAxisSpacing: 15,
                            childAspectRatio: 0.72,
                          ),
                      itemCount: playlists.length,
                      padding: EdgeInsets.only(
                        right: 10, // 给滚动条留点位置
                        bottom: hasCurrentSong ? 90 : 0, // 有歌在播时别被 PlayBar 遮住
                      ),
                      itemBuilder: (context, index) {
                        final playlist = playlists[index];

                        return _PlaylistCard(
                          key: ValueKey(playlist.playlistId),
                          playlist: playlist,
                          onTap: () => _openPlaylist(playlist),
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
// _PlaylistCard — 单个歌单卡片
//
// 自上而下：封面（正方形）→ 歌单名 →「共 N 首」
//
// TODO(后端): 卡片还没有右键菜单（重命名 / 删除歌单 / 播放）。
//             等歌单管理接口注册好之后，和专辑卡片一起接
//             flutter_root_context_menu（用法参考
//             widgets/PlayControl/more_play_info.dart）。
// ════════════════════════════════════════════════════════════════
class _PlaylistCard extends StatefulWidget {
  final PlaylistBasicInfo playlist;

  /// 点卡片本体：打开这个歌单
  final VoidCallback onTap;

  const _PlaylistCard({super.key, required this.playlist, required this.onTap});

  @override
  State<_PlaylistCard> createState() => _PlaylistCardState();
}

class _PlaylistCardState extends State<_PlaylistCard> {
  /// 鼠标是否悬浮在卡片上
  bool _isHovered = false;

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final playlist = widget.playlist;

    return MouseRegion(
      cursor: SystemMouseCursors.click,
      onEnter: (_) => setState(() => _isHovered = true),
      onExit: (_) => setState(() => _isHovered = false),
      child: GestureDetector(
        onTap: widget.onTap,
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 200),
          padding: const EdgeInsets.all(8),
          color: _isHovered ? theme.colorHover : theme.colorCell,
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              // ── 封面（正方形，居中） ──
              Expanded(
                child: Center(
                  child: AspectRatio(
                    aspectRatio: 1,
                    child: AlbumCoverImage(hash: playlist.hash),
                  ),
                ),
              ),

              const SizedBox(height: 8),

              // ── 歌单名（单行省略） ──
              Tooltip(
                message: playlist.title,
                child: Text(
                  playlist.title,
                  style: theme.midTextStyle,
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
              ),

              const SizedBox(height: 2),

              // ── 歌曲数量 ──
              Text('共 ${playlist.songCount} 首', style: theme.littleTextStyle),
            ],
          ),
        ),
      ),
    );
  }
}
