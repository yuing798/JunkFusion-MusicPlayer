import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:junk_fusion_ui/model/folder_basic_info.dart';
import 'package:junk_fusion_ui/providers/folder_provider.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/app_cache.dart';
import 'package:junk_fusion_ui/widgets/helper_widget/combo_box.dart';
import 'package:junk_fusion_ui/widgets/pages/pages_route.dart';
// AlbumCoverImage（哈希 → image/<hash>/original.jpg）目前住在
// single_album_page.dart 里，先复用；以后抽到 widgets/ 下面更合适
import 'package:junk_fusion_ui/widgets/pages/single_album_page.dart';
import 'package:provider/provider.dart';
import 'package:smooth_scroll_multiplatform/smooth_scroll_multiplatform.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

// ════════════════════════════════════════════════════════════════
// folder_view_page.dart — 文件夹视图（网格）
//
// 布局（自上而下）：
//   第 1 行（50px）：标题「文件夹」+「共 N 个」
//   第 2 行（80px）：工具栏（排序方式下拉框 + 升降序按钮）
//   第 3 部分：文件夹网格（GridView，鼠标滚轮平滑滚动）
//
// 卡片：封面（这个文件夹里第一首有封面的歌；没有就用文件夹图标）
//       → 文件夹名 →「共 N 首」。
// 点卡片走路由进 SingleFolderPage（见 pages_route.dart）。
//
// TODO(后端): 文件夹目前完全没有接口（FileDirManage 是空壳），
//             数据是空的（见 FolderProvider 里的 TODO），所以这一页
//             现在只会显示空状态。后端约定：文件夹由 songs.filePath
//             的父目录现算，path 就是它的唯一标识。
// ════════════════════════════════════════════════════════════════

/// 文件夹列表的排序方式
enum FolderSortMode {
  byName('文件夹名称', 1),
  bySongCount('歌曲数量', 2);

  final String label;
  final int value;

  const FolderSortMode(this.label, this.value);
}

class FolderViewPage extends StatefulWidget {
  const FolderViewPage({super.key});

  @override
  State<FolderViewPage> createState() => _FolderViewPageState();
}

class _FolderViewPageState extends State<FolderViewPage> {
  /// 当前选中的排序方式
  FolderSortMode _selectedSort = FolderSortMode.byName;

  /// 是否升序（true = 升序，false = 降序）
  bool _isAscending = true;

  @override
  void initState() {
    super.initState();

    final frontCache = AppCache.frontCacheRef;
    final sortValue = frontCache.getInt('folderSortWays') ?? 1;
    final isAscending = frontCache.getBool('folderIsAscending') ?? true;
    _selectedSort = FolderSortMode.values.firstWhere(
      (v) => v.value == sortValue,
      orElse: () => FolderSortMode.byName,
    );
    _isAscending = isAscending;

    // 进页面的时候拉一遍文件夹列表（后端接口还没做，现在是空列表）
    WidgetsBinding.instance.addPostFrameCallback((_) {
      context.read<FolderProvider>().getAllFolders();
    });
  }

  // 切换升降序
  void _toggleAscending() {
    setState(() {
      _isAscending = !_isAscending;
    });

    AppCache.frontCacheRef.setBool('folderIsAscending', _isAscending);
  }

  /// 按当前排序方式排好（不动 FolderProvider 里那份原列表）
  List<FolderBasicInfo> _getSortedFolders(List<FolderBasicInfo> source) {
    int compare(FolderBasicInfo a, FolderBasicInfo b) =>
        switch (_selectedSort) {
          FolderSortMode.byName => AppCache.collator.compare(a.name, b.name),
          // 数量一样的时候再按名字排，保证顺序稳定
          FolderSortMode.bySongCount =>
            a.songCount == b.songCount
                ? AppCache.collator.compare(a.name, b.name)
                : a.songCount.compareTo(b.songCount),
        };

    final sorted = List<FolderBasicInfo>.of(source);
    sorted.sort((a, b) => _isAscending ? compare(a, b) : compare(b, a));
    return sorted;
  }

  /// 打开某个文件夹：先把这个文件夹装进 Provider，再往路由栈里压一条记录
  void _openFolder(FolderBasicInfo folder) {
    context.read<FolderProvider>().setNewFolder(folder.path);
    context.read<PagesRouteController>().push(
      PagesRouteTable.singleFolder(folder),
    );
  }

  /// 没有文件夹时的提示
  Widget _buildEmptyHint(AppTheme theme) {
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(TablerIcons.folder, size: 64, color: theme.colorEdge),
          const SizedBox(height: 15),
          Text('还没有任何文件夹', style: theme.midTextStyle),
          // TODO(后端): 文件夹接口做完之后，这句提示要改掉
          const SizedBox(height: 5),
          Text('文件夹后端接口还没做，做完这里会按歌曲路径归类', style: theme.littleTextStyle),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    // watch 放在最前面：文件夹列表变了、主题变了都要重画
    final theme = context.watch<AppTheme>();
    final folders = _getSortedFolders(context.watch<FolderProvider>().folders);

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
              Text('文件夹', style: theme.bigTextStyle),
              const SizedBox(width: 15),
              Text('共 ${folders.length} 个', style: theme.littleTextStyle),
            ],
          ),
        ),

        // ── 第 2 行：工具栏 ──
        SizedBox(
          height: 80,
          child: Row(
            children: [
              // TODO: 这一行左边还空着，后面可以放「文件夹搜索框」
              //       或者「只看某一层」之类的筛选项。

              // 弹性空间（把排序控件推到最右边）
              const Spacer(),

              // ── 排序方式下拉框 ──
              SizedBox(
                width: 200,
                child: CustomComboBox<FolderSortMode>(
                  value: _selectedSort,
                  items: FolderSortMode.values,
                  itemLabel: (each) => each.label,
                  onChanged: (mode) async {
                    if (mode == null) return;
                    setState(() {
                      _selectedSort = mode;
                    });
                    final frontCache = AppCache.frontCacheRef;
                    frontCache.setInt('folderSortWays', mode.value);
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

        // ── 第 3 部分：文件夹网格（虚拟滚动，填充剩余空间） ──
        Expanded(
          child: folders.isEmpty
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
                      itemCount: folders.length,
                      padding: EdgeInsets.only(
                        right: 10, // 给滚动条留点位置
                        bottom: hasCurrentSong ? 90 : 0, // 有歌在播时别被 PlayBar 遮住
                      ),
                      itemBuilder: (context, index) {
                        final folder = folders[index];

                        return _FolderCard(
                          key: ValueKey(folder.path),
                          folder: folder,
                          onTap: () => _openFolder(folder),
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
// _FolderCard — 单个文件夹卡片
//
// 自上而下：封面/文件夹图标（正方形）→ 文件夹名 →「共 N 首」
// 文件夹没有独立封面，hash 为空（这个文件夹里的歌都没封面）就画个文件夹图标。
//
// TODO: 卡片还没有右键菜单（播放 / 添加到播放列表 / 用资源管理器打开）。
//       工程里已经引了 flutter_root_context_menu，用法参考
//       widgets/PlayControl/more_play_info.dart。
// ════════════════════════════════════════════════════════════════
class _FolderCard extends StatefulWidget {
  final FolderBasicInfo folder;

  /// 点卡片本体：打开这个文件夹
  final VoidCallback onTap;

  const _FolderCard({super.key, required this.folder, required this.onTap});

  @override
  State<_FolderCard> createState() => _FolderCardState();
}

class _FolderCardState extends State<_FolderCard> {
  /// 鼠标是否悬浮在卡片上
  bool _isHovered = false;

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final folder = widget.folder;
    final hash = folder.hash;

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
              // ── 封面 / 文件夹图标（正方形，居中） ──
              Expanded(
                child: Center(
                  child: AspectRatio(
                    aspectRatio: 1,
                    child: (hash == null || hash.isEmpty)
                        ? Center(
                            child: Icon(
                              TablerIcons.folderFilled,
                              size: 56,
                              color: theme.colorEdge,
                            ),
                          )
                        : AlbumCoverImage(hash: hash),
                  ),
                ),
              ),

              const SizedBox(height: 8),

              // ── 文件夹名（单行省略；tooltip 给完整路径） ──
              Tooltip(
                message: folder.path,
                child: Text(
                  folder.name,
                  style: theme.midTextStyle,
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
              ),

              const SizedBox(height: 2),

              // ── 歌曲数量 ──
              Text('共 ${folder.songCount} 首', style: theme.littleTextStyle),
            ],
          ),
        ),
      ),
    );
  }
}
