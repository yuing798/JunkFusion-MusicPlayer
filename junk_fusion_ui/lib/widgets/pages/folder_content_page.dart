import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';
import 'package:flutter_root_context_menu/flutter_root_context_menu.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/model/song_basic_info.dart';
import 'package:junk_fusion_ui/providers/folder_provider.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/app_cache.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';
import 'package:junk_fusion_ui/widgets/each_song.dart';
import 'package:junk_fusion_ui/widgets/popup_window.dart';
import 'package:provider/provider.dart';
import 'package:smooth_scroll_multiplatform/smooth_scroll_multiplatform.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

// ════════════════════════════════════════════════════════════════
// folder_content_page.dart — 三段式右侧主内容里的「文件夹」页
//
// 内容全部来自 FolderProvider（见 providers/folder_provider.dart）：
//   列表 = [当前目录的子文件夹] + [当前目录的歌曲]
//   两者都按后端返回的顺序，前端不排序。
//
// 布局（自上而下，最后一排按钮在没选目录时也在，方便一进来就导入）：
//   第 1 行（60px）：文件夹图标 + 名字 + 「路径 · N 个子文件夹 · M 首」
//   第 2 行（50px）：播放全部 / 添加到播放列表 …… 导入文件夹 / 单独导入歌曲
//   第 3 部分：列表（子文件夹行 + EachSong 歌曲行），没选目录时显示提示
//
// 交互：
//   双击子文件夹     → FolderProvider.openFolder(子目录路径)
//                      左侧树会自动定位、展开并高亮（由 openToken 驱动）
//   点歌曲封面       → 把这个文件夹的歌曲当成播放列表开始播
//   导入文件夹       → FilePicker 选目录 → 后端 importNewSongFolder →
//                      重拉根目录 + 重拉歌曲表 → 打开这个文件夹
//   单独导入歌曲     → SongProvider.songsImport() → 后端 importSongs（路径数组）
//                      导入完成后后端回调 → 新歌进 SongProvider
//
// 左侧的文件夹树在 widgets/nav/folder_tree_sidebar.dart，不在这个文件里。
// ════════════════════════════════════════════════════════════════

class FolderContentPage extends StatefulWidget {
  const FolderContentPage({super.key});

  @override
  State<StatefulWidget> createState() {
    return FolderContentPageState();
  }
}

/// 文件夹视图的右侧内容
class FolderContentPageState extends State<FolderContentPage> {
  final importBtnKey = GlobalKey();

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final folder = context.watch<FolderProvider>();

    // 有歌在播时底部 PlayBar 会浮上来，列表要留出位置
    final hasCurrentSong = context.select<PlaybackProvider, bool>(
      (provider) => provider.curBasicInfo != null,
    );

    final hasSelection = folder.hasSelection;
    final subDirs = hasSelection ? folder.subDirsInFolder : const <String>[];
    final songs = hasSelection ? folder.songsInFolder : const <SongBasicInfo>[];

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // ── 第 1 行：标题 ──
        _buildHeader(theme, folder, subDirs.length, songs.length),

        // ── 第 2 行：操作（导入按钮没选目录时也能用） ──
        _buildToolbar(context, songs),

        // ── 第 3 部分：子文件夹 + 歌曲 / 空状态 ──
        Expanded(
          child: hasSelection
              ? _buildList(
                  context,
                  theme,
                  folder,
                  subDirs,
                  songs,
                  hasCurrentSong,
                )
              : _buildNothingSelected(theme, folder),
        ),
      ],
    );
  }

  // ════════════════════════════════════════════════════════════
  // 各个块
  // ════════════════════════════════════════════════════════════

  Widget _buildHeader(
    AppTheme theme,
    FolderProvider folder,
    int subDirCount,
    int songCount,
  ) {
    final hasSelection = folder.hasSelection;
    final isVirtual = folder.isSingleImportOpen;

    final String title = hasSelection ? folder.currentTitle : '文件夹';

    final String subtitle;
    if (!folder.isInitialized) {
      subtitle = '正在读取文件夹…';
    } else if (!hasSelection) {
      subtitle = '共 ${folder.rootPaths.length} 个根目录 · 从左侧选一个文件夹';
    } else if (isVirtual) {
      subtitle = '单独导入的歌曲 · 共 $songCount 首';
    } else {
      subtitle =
          '${folder.currentPath ?? ''} · $subDirCount 个子文件夹 · $songCount 首';
    }

    final IconData icon = !hasSelection
        ? TablerIcons.folder
        : (isVirtual ? TablerIcons.fileMusic : TablerIcons.folderFilled);

    return SizedBox(
      height: 60,
      child: Row(
        children: [
          SizedBox(
            width: 50,
            height: 50,
            child: Center(child: Icon(icon, size: 40, color: theme.colorEdge)),
          ),
          const SizedBox(width: 10),
          Expanded(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  title,
                  style: theme.bigTextStyle,
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
                const SizedBox(height: 2),
                Text(
                  subtitle,
                  style: theme.littleTextStyle,
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  /// 工具栏：左边是播放相关的，右边是导入相关的
  Widget _buildToolbar(BuildContext context, List<SongBasicInfo> songs) {
    return SizedBox(
      height: 50,
      child: Row(
        children: [
          _FolderActionButton(
            icon: TablerIcons.playerPlayFilled,
            label: '播放全部',
            isPrimary: true,
            onTap: songs.isEmpty ? null : () => _playAll(context, songs),
          ),
          const SizedBox(width: 10),
          _FolderActionButton(
            icon: TablerIcons.playlistAdd,
            label: '添加到播放列表',
            onTap: songs.isEmpty
                ? null
                : () => _appendToPlayList(context, songs),
          ),

          // 导入按钮推到右边：和播放操作区分开
          const Spacer(),

          _FolderActionButton(
            icon: TablerIcons.refresh,
            label: "刷新",
            onTap: bindings.refreshForntend,
          ),
          const SizedBox(width: 5),
          _FolderActionButton(
            key: importBtnKey,
            icon: TablerIcons.fileImport,
            label: "导入",
            onTap: () {
              final importBtnCenter = UtilFunction.getWidgetCenter(
                importBtnKey,
              );
              if (importBtnCenter == null) return;

              return showRootContextMenu(
                context: context,
                position: Offset(
                  importBtnCenter.dx - 140,
                  importBtnCenter.dy + 30,
                ),
                useBarrier: true,
                items: [
                  ContextMenuItem.custom(
                    builder: (_) => _FolderActionButton(
                      icon: TablerIcons.folderPlus,
                      label: '导入文件夹',
                      onTap: () => _importFolder(context),
                    ),
                  ),
                  ContextMenuItem.custom(
                    builder: (_) => _FolderActionButton(
                      icon: TablerIcons.musicPlus,
                      label: '单独导入歌曲',
                      onTap: () => _importSongs(context),
                    ),
                  ),
                ],
              );
            },
          ),
        ],
      ),
    );
  }

  Widget _buildList(
    BuildContext context,
    AppTheme theme,
    FolderProvider folder,
    List<String> subDirs,
    List<SongBasicInfo> songs,
    bool hasCurrentSong,
  ) {
    if (subDirs.isEmpty && songs.isEmpty) {
      return Center(
        child: Text(
          folder.isSingleImportOpen ? '还没有单独导入的歌曲' : '这个文件夹是空的',
          style: theme.littleTextStyle,
        ),
      );
    }

    // key 绑当前目录：换文件夹时整块换掉，滚动位置回到顶部
    // （不换的话会停在上一个文件夹的位置，被 clamp 到新列表末尾）
    return KeyedSubtree(
      key: ValueKey('folder-list-${folder.currentPath}'),
      child: DynMouseScroll(
        builder: (context_, controller_, physics_) {
          return ListView.builder(
            controller: controller_,
            physics: physics_,
            // 子文件夹在前、歌曲在后，顺序都是后端给的
            itemCount: subDirs.length + songs.length,
            padding: EdgeInsets.only(
              right: 10, // 给滚动条留位置
              bottom: hasCurrentSong ? 90 : 0, // 有歌在播时别被 PlayBar 盖住
            ),
            itemBuilder: (context, index) {
              // ── 子文件夹 ──
              if (index < subDirs.length) {
                final path = subDirs[index];
                return _SubFolderTile(
                  key: ValueKey(path),
                  path: path,
                  onOpen: () => context.read<FolderProvider>().openFolder(path),
                );
              }

              // ── 歌曲 ──
              final song = songs[index - subDirs.length];
              return EachSong(
                key: ValueKey(song.songId),
                song: song,
                onSongPlay: () {
                  // 点封面播放时，把「整个文件夹」当成播放列表
                  final playback = context.read<PlaybackProvider>();
                  playback.setPlayList(folder.songsInFolder);
                  playback.saveState();
                },
              );
            },
          );
        },
      ),
    );
  }

  /// 一个文件夹都没打开时的提示
  Widget _buildNothingSelected(AppTheme theme, FolderProvider folder) {
    // 还没初始化完：标题行那边已经写着「正在读取文件夹…」了，这里先空着
    if (!folder.isInitialized) return const SizedBox.shrink();

    final hasRoots = folder.rootPaths.isNotEmpty;

    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(
            hasRoots ? TablerIcons.folder : TablerIcons.folderOff,
            size: 64,
            color: theme.colorEdge,
          ),
          const SizedBox(height: 15),
          Text(
            hasRoots ? '从左侧选择一个文件夹' : '还没有导入任何文件夹',
            style: theme.midTextStyle,
          ),
          const SizedBox(height: 5),
          Text(
            hasRoots ? '单击左侧的文件名就会在这里显示它的内容' : '导入文件夹之后，这里会显示它的内容',
            style: theme.littleTextStyle,
          ),
        ],
      ),
    );
  }

  // ════════════════════════════════════════════════════════════
  // 播放
  // ════════════════════════════════════════════════════════════

  /// 播放全部：按当前顺序把整个文件夹塞进播放列表，从第一首开始放
  void _playAll(BuildContext context, List<SongBasicInfo> songs) {
    if (songs.isEmpty) return;

    final playback = context.read<PlaybackProvider>();
    // 先设置播放列表再切歌：setNewSong 内部会把当前播放列表缓存下来
    playback.setPlayList(songs);
    playback.setNewSong(songs.first.songId);
  }

  /// 添加到播放列表：追加到现有播放列表后面（按 songId 去重）
  void _appendToPlayList(BuildContext context, List<SongBasicInfo> songs) {
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

  // ════════════════════════════════════════════════════════════
  // 导入
  // ════════════════════════════════════════════════════════════

  /// 导入整个文件夹
  ///
  /// 流程：选目录 → 后端 importNewSongFolder（同步递归导入）→ 重拉根目录
  /// → 直接打开这个文件夹。
  ///
  /// 导入过程中每首歌都会走后端的 onAddOrUpdateSong，SongProvider 的缓存
  /// 会自动补齐，不用整表重拉。
  Future<void> _importFolder(BuildContext context) async {
    // await 之前先把 provider 抓出来：等用户选完目录回来，context 可能已经失效
    final folder = context.read<FolderProvider>();

    final String? dirPath = await FilePicker.getDirectoryPath(
      dialogTitle: '选择要导入的文件夹',
      initialDirectory: AppCache.appDocDir,
      lockParentWindow: true,
    );
    if (dirPath == null || dirPath.isEmpty) return;
    // 统一去掉结尾分隔符：树上的节点 id 就是路径，带不带结尾的 \ 是两个 id
    final importPath = FolderProvider.trimPath(dirPath);

    // 前端先拦一遍重复导入（后端也会拦，但它的提示回调没注册到前端）
    if (folder.isRootPath(importPath)) {
      await DialogUtil.showInfoDialog('这个文件夹已经在文件树里了');
      return;
    }

    // 同步导入，大文件夹会卡一会儿（见 FolderProvider.importFolder 的 FIXME）
    folder.importFolder(importPath);
    // 导入完直接打开它（顺便让左侧树定位过去）。
    // 缺的歌曲基本信息由 FolderProvider 按需从后端补，不用整表重拉。
    folder.openFolder(importPath);

    await DialogUtil.showInfoDialog('文件夹已导入：$importPath');
  }

  /// 单独导入歌曲：和「所有音乐」页的导入文件是同一个流程
  ///
  /// 后端每导入/更新完一首会回调 registerOnAddOrUpdateSong，
  /// 新歌由 CppCallbackManager 塞进 SongProvider。
  Future<void> _importSongs(BuildContext context) async {
    final folder = context.read<FolderProvider>();
    final songProvider = context.read<SongProvider>();

    await songProvider.songsImport();

    // 正在看「单独导入」的话，重新拉一遍它的 songId 列表，
    // 新导入的歌才会出现在当前列表里
    if (folder.isSingleImportOpen) folder.openSingleImportFolder();
  }
}

// ════════════════════════════════════════════════════════════════
// _SubFolderTile — 右侧列表里的一个子文件夹行
//
// 双击打开（和左侧树的单击不同：右侧列表里的子文件夹需要双击，避免
// 单击就跳走、看错内容）。
// ════════════════════════════════════════════════════════════════
class _SubFolderTile extends StatefulWidget {
  /// 子文件夹的完整路径
  final String path;

  /// 双击打开
  final VoidCallback onOpen;

  const _SubFolderTile({super.key, required this.path, required this.onOpen});

  @override
  State<_SubFolderTile> createState() => _SubFolderTileState();
}

class _SubFolderTileState extends State<_SubFolderTile> {
  bool _isHovered = false;

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final name = FolderProvider.baseNameOf(widget.path);

    return MouseRegion(
      cursor: SystemMouseCursors.click,
      onEnter: (_) => setState(() => _isHovered = true),
      onExit: (_) => setState(() => _isHovered = false),
      child: GestureDetector(
        behavior: HitTestBehavior.opaque,
        onDoubleTap: widget.onOpen,
        child: Tooltip(
          message: '${widget.path}\n双击打开',
          waitDuration: const Duration(milliseconds: 500),
          child: Container(
            height: 48,
            padding: const EdgeInsets.symmetric(horizontal: 10),
            color: _isHovered ? theme.colorHover : theme.colorCell,
            child: Row(
              children: [
                Icon(
                  TablerIcons.folderFilled,
                  size: 22,
                  color: theme.colorEdge,
                ),
                const SizedBox(width: 10),
                Expanded(
                  child: Text(
                    name,
                    style: theme.midTextStyle,
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                ),
                Text(
                  '双击打开',
                  style: theme.littleTextStyle.copyWith(color: theme.colorEdge),
                ),
              ],
            ),
          ),
        ),
      ),
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
    super.key,
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
