import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:junk_fusion_ui/model/album_basic_info.dart';
import 'package:junk_fusion_ui/providers/album_provider.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/app_cache.dart';
import 'package:junk_fusion_ui/widgets/combo_box.dart';
import 'package:junk_fusion_ui/widgets/pages/pages_route.dart';
import 'package:junk_fusion_ui/widgets/pages/single_album_page.dart';
import 'package:junk_fusion_ui/widgets/popup_window.dart';
import 'package:provider/provider.dart';
import 'package:smooth_scroll_multiplatform/smooth_scroll_multiplatform.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

// ════════════════════════════════════════════════════════════════
// album_view_page.dart — 专辑分类视图（网格）
//
// 布局（自上而下）：
//   第 1 行（50px）：标题「专辑」+「共 N 张 · M 首」
//   第 2 行（80px）：工具栏（排序方式下拉框 + 升降序按钮）
//   第 3 部分：专辑网格（GridView，鼠标滚轮平滑滚动）
//
// 单张专辑卡片自上而下：专辑封面 → 专辑名 → 专辑艺术家。
// 点卡片 → 先把这张专辑的详情装进 AlbumProvider，再往路由栈里压一条
// 单张专辑页（见 pages_route.dart）；退回网格靠左上角那个返回按钮；
// 鼠标悬浮到封面上会浮出一个播放按钮，点它直接整张专辑开播。
//
// 数据来源：AlbumProvider（→ 后端 getAllAlbumView() / getSongsListByAlbumId()）。
// 专辑的分组是后端 albums 表算好的，前端只负责排序和画界面。
// ════════════════════════════════════════════════════════════════

/// 专辑列表的排序方式
enum AlbumSortMode {
  byTitle('专辑名称', 1),
  byAlbumArtists('专辑艺术家', 2);

  final String label;
  final int value;

  const AlbumSortMode(this.label, this.value);
}

class AlbumViewPage extends StatefulWidget {
  const AlbumViewPage({super.key});

  @override
  State<AlbumViewPage> createState() => _AlbumViewPageState();
}

class _AlbumViewPageState extends State<AlbumViewPage> {
  /// 当前选中的排序方式
  AlbumSortMode _selectedSort = AlbumSortMode.byTitle;

  /// 是否升序（true = 升序，false = 降序）
  bool _isAscending = true;

  @override
  void initState() {
    super.initState();

    final frontCache = AppCache.frontCacheRef;
    final sortValue = frontCache.getInt('albumSortWays') ?? 1;
    final isAscending = frontCache.getBool('albumIsAscending') ?? true;
    _selectedSort = AlbumSortMode.values.firstWhere(
      (v) => v.value == sortValue,
      orElse: () => AlbumSortMode.byTitle,
    );
    _isAscending = isAscending;
    WidgetsBinding.instance.addPostFrameCallback((_) {
      context.read<AlbumProvider>().getAllAlbums();
    });
  }

  // 切换升降序
  void _toggleAscending() {
    setState(() {
      _isAscending = !_isAscending;
    });

    AppCache.frontCacheRef.setBool('albumIsAscending', _isAscending);
  }

  /// 按当前排序方式排好（不动 AlbumProvider 里那份原列表）
  List<AlbumBasicInfo> _getSortedAlbums(List<AlbumBasicInfo> source) {
    // 只比较「升序」的方向，降序的时候整体反过来
    // 这里用 switch 表达式：枚举的所有分支都必须写全，否则编译不过
    int compare(AlbumBasicInfo a, AlbumBasicInfo b) => switch (_selectedSort) {
      // 直接按字符串比较，中英文混排的顺序不一定和资源管理器一致
      AlbumSortMode.byTitle => AppCache.collator.compare(a.title, b.title),
      // 同一个艺术家下面再按专辑名排
      AlbumSortMode.byAlbumArtists => _compareArtist(a, b),
    };

    final sorted = List<AlbumBasicInfo>.of(source);
    sorted.sort((a, b) => _isAscending ? compare(a, b) : compare(b, a));
    return sorted;
  }

  int _compareArtist(AlbumBasicInfo a, AlbumBasicInfo b) {
    int result = 0;
    if (a.albumArtists != null && b.albumArtists != null) {
      result = AppCache.collator.compare(
        a.albumArtists!.join("/"),
        b.albumArtists!.join("/"),
      );
    }
    return result == 0 ? AppCache.collator.compare(a.title, b.title) : result;
  }

  /// 播放整张专辑（卡片封面上的悬浮按钮）
  void _playAlbum(AlbumBasicInfo album) {
    final playback = context.read<PlaybackProvider>();

    final songs = context.read<AlbumProvider>().songsInAlbums;

    if (songs.isEmpty) {
      DialogUtil.showInfoDialog('「${album.title}」里还没有歌曲');
      return;
    }

    final firstSongId = songs.first.songId;
    if (playback.playList != songs) {
      // 先设置播放列表再切歌：setNewSong 内部会把当前播放列表缓存下来
      playback.setPlayList(songs);
      playback.setNewSong(firstSongId);
    }
  }

  /// 打开某张专辑：进单张专辑页
  ///
  /// 先把详情装进 AlbumProvider（SingleAlbumPage 自己不接专辑参数，
  /// 是从 AlbumProvider 里读的），再往路由栈里压一条记录 ——
  /// 左上角的返回按钮就能顺着这条记录退回网格
  void _openAlbum(AlbumBasicInfo album) {
    context.read<AlbumProvider>().setNewAlbum(album.albumId);
    context.read<PagesRouteController>().push(
      PagesRouteTable.singleAlbum(album),
    );
  }

  /// 没有专辑时的提示
  Widget _buildEmptyHint(AppTheme theme) {
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(TablerIcons.album, size: 64, color: theme.colorEdge),
          const SizedBox(height: 15),
          Text('还没有任何专辑', style: theme.midTextStyle),
          const SizedBox(height: 5),
          Text('请先在「所有音乐」页导入歌曲，导入后后端会自动按专辑归类', style: theme.littleTextStyle),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    // watch 要放在最前面（不能塞进 if 分支里）：
    // 导入/删除歌曲、改标签之后，AlbumProvider 会重新拉专辑列表，
    // 这里的专辑网格和单张专辑页的头部都要跟着刷新
    final theme = context.watch<AppTheme>();
    final albumProvider = context.watch<AlbumProvider>();
    final allAlbums = albumProvider.albums;

    final albums = _getSortedAlbums(allAlbums);

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
              Text('专辑', style: theme.bigTextStyle),
              const SizedBox(width: 15),
              // 歌曲数直接用曲库总数：每首有专辑名 + 艺术家的歌都会落进某张专辑，
              // 没有专辑标签的歌不在这套分类里（后端也不会给它建专辑）
              Text('共 ${albums.length} 张 ', style: theme.littleTextStyle),
            ],
          ),
        ),

        // ── 第 2 行：工具栏 ──
        SizedBox(
          height: 80,
          child: Row(
            children: [
              // TODO: 这一行左边还空着，后面可以放「专辑搜索框」
              //       或者「只显示有封面的专辑」之类的筛选项。

              // 弹性空间（把排序控件推到最右边）
              const Spacer(),

              // ── 排序方式下拉框 ──
              SizedBox(
                width: 200,
                child: ComboBox<AlbumSortMode>(
                  value: _selectedSort,
                  items: AlbumSortMode.values,
                  itemBuilder: (mode) =>
                      Text(mode.label, style: theme.comboTextStyle),
                  onChanged: (mode) async {
                    setState(() {
                      _selectedSort = mode;
                    });
                    final frontCache = AppCache.frontCacheRef;
                    frontCache.setInt('albumSortWays', mode.value);
                  },
                ),
              ),

              const SizedBox(width: 8),

              // ── 升降序切换按钮 ──
              IconButton(
                icon: Icon(
                  _isAscending
                      ? TablerIcons.arrowBigDownLineFilled
                      : TablerIcons.arrowBigUpLineFilled,
                  size: 32,
                ),
                onPressed: _toggleAscending,
                tooltip: _isAscending ? '升序' : '降序',
                color: theme.colorTextMain,
              ),
            ],
          ),
        ),

        // ── 第 3 部分：专辑网格（虚拟滚动，填充剩余空间） ──
        Expanded(
          child: albums.isEmpty
              ? _buildEmptyHint(theme)
              : DynMouseScroll(
                  builder: (context_, controler_, physics_) {
                    return GridView.builder(
                      controller: controler_,
                      physics: physics_,
                      // maxCrossAxisExtent：每张卡片最宽 220，窗口越宽列数越多
                      gridDelegate:
                          const SliverGridDelegateWithMaxCrossAxisExtent(
                            maxCrossAxisExtent: 220,
                            mainAxisSpacing: 15,
                            crossAxisSpacing: 15,
                            childAspectRatio: 0.72,
                          ),
                      itemCount: albums.length,
                      padding: EdgeInsets.only(
                        right: 10, // 给滚动条留点位置
                        bottom: hasCurrentSong ? 90 : 0, // 有歌在播时别被 PlayBar 遮住
                      ),
                      itemBuilder: (context, index) {
                        final album = albums[index];

                        return _AlbumCard(
                          key: ValueKey(album.albumId),
                          album: album,
                          // 点卡片 → 进专辑
                          onTap: () => _openAlbum(album),
                          // 点封面上的播放按钮 → 直接播放整张专辑
                          onPlay: () => _playAlbum(album),
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
// _AlbumCard — 单张专辑卡片
//
// 自上而下：封面（正方形）→ 专辑名 → 专辑艺术家
// 悬浮时：整张卡片换成悬浮色背景，封面上浮出「播放整张专辑」按钮。
//
// TODO: 卡片还没有右键菜单（播放 / 下一首播放 / 添加到播放列表 / 查看专辑信息）。
//       工程里已经引了 flutter_root_context_menu，用法可以参考
//       widgets/PlayControl/more_play_info.dart 里的 showRootContextMenu。
// ════════════════════════════════════════════════════════════════
class _AlbumCard extends StatefulWidget {
  final AlbumBasicInfo album;

  /// 点卡片本体：打开这张专辑
  final VoidCallback onTap;

  /// 点封面上的播放按钮：直接播放整张专辑
  final VoidCallback onPlay;

  const _AlbumCard({
    super.key,
    required this.album,
    required this.onTap,
    required this.onPlay,
  });

  @override
  State<_AlbumCard> createState() => _AlbumCardState();
}

class _AlbumCardState extends State<_AlbumCard> {
  /// 鼠标是否悬浮在卡片上
  bool _isHovered = false;

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final album = widget.album;

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
              // ── 专辑封面（正方形，居中） ──
              Expanded(
                child: Center(
                  child: AspectRatio(
                    // aspectRatio: 1 → 正方形；在 Center 里会自动取
                    // 「可用宽高里能放下的最大正方形」
                    aspectRatio: 1,
                    child: Stack(
                      fit: StackFit.expand,
                      children: [
                        AlbumCoverImage(hash: album.hash),

                        // 悬浮时才出现的遮罩 + 播放按钮
                        AnimatedOpacity(
                          opacity: _isHovered ? 1 : 0,
                          duration: const Duration(milliseconds: 200),
                          // 不悬浮时必须让点击穿透过去，
                          // 否则点封面会变成「播放」而不是「进专辑」
                          child: IgnorePointer(
                            ignoring: !_isHovered,
                            child: GestureDetector(
                              onTap: widget.onPlay,
                              child: Container(
                                color: Colors.black38,
                                child: const Center(
                                  child: Icon(
                                    TablerIcons.playerPlayFilled,
                                    size: 42,
                                    color: Colors.white,
                                  ),
                                ),
                              ),
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
              ),

              const SizedBox(height: 8),

              // ── 专辑名（单行省略） ──
              Tooltip(
                message: album.title,
                child: Text(
                  album.title,
                  style: theme.midTextStyle,
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
              ),

              const SizedBox(height: 2),

              // ── 专辑艺术家（单行省略） ──
              Tooltip(
                message: album.albumArtists != null
                    ? album.albumArtists!.join(" / ")
                    : "未知艺术家",
                child: Text(
                  album.albumArtists != null
                      ? album.albumArtists!.join(" / ")
                      : "未知艺术家",
                  style: theme.littleTextStyle,
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
