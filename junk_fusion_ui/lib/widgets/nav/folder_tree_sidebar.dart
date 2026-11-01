import 'package:flutter/material.dart';
import 'package:flutter_directory_tree/flutter_directory_tree.dart';
import 'package:junk_fusion_ui/providers/folder_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:provider/provider.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

// ════════════════════════════════════════════════════════════════
// folder_tree_sidebar.dart — 三段式中间栏里的文件夹树
//
// 交互（和后端接口的分工见 providers/folder_provider.dart）：
//   单击节点名字 / 图标
//       → FolderProvider.openFolder(path)：右侧列表立刻换成这个目录的内容
//       → 树自动展开这个节点，并高亮它（reveal + select）
//   单击展开箭头（> / v）
//       → 只切换树的展开状态，**不刷新右侧列表**
//       → 如果这个目录还没拉过子目录，顺手把子目录拉回来（只动树）
//   右侧列表双击子文件夹
//       → 那边调 openFolder，这边靠 openToken 变化自动展开并高亮
//
// 树用 flutter_directory_tree 这个依赖包：
//   DirectoryTreeController 管展开 / 选中 / 扁平化；DirectoryTreeView 负责
//   虚拟滚动 + 缩进 + 箭头；节点长什么样由我们自己的 nodeBuilder 决定
//   （TreeNodeTile：选中/hover 底色、单击回调）。
//
// 节点 id 直接用目录的完整路径（虚拟节点用 FolderProvider.singleImportNodeId），
// 这样「展开集合 = 路径集合 = 落盘的集合」，不用来回映射。
//
// 状态保存：
//   展开状态回写 FolderProvider.setExpandedPaths → 落盘到 SharedPreferences
//   （键见 FolderProvider.expandedPathsCacheKey），下次打开文件夹段时恢复；
//   当前打开的目录由 FolderProvider 自己存。
// ════════════════════════════════════════════════════════════════

/// 中间栏的文件夹树
class FolderTreeSidebar extends StatefulWidget {
  const FolderTreeSidebar({super.key});

  @override
  State<FolderTreeSidebar> createState() => _FolderTreeSidebarState();
}

class _FolderTreeSidebarState extends State<FolderTreeSidebar> {
  /// TreeData 的隐藏根（DirectoryTreeController 要求有一个 root 节点）
  static const String _rootNodeId = 'root';

  /// 渲染起点：它的子节点（= 各根目录）会以 depth 0 画出来
  static const String _containerNodeId = 'tree_root';

  /// 占位子节点的 id 前缀（见 _buildTreeData 里的注释）
  static const String _loadingIdPrefix = '__loading__:';

  /// 行高（要和下面 DirectoryTreeThemeData 的 rowHeight 一致，
  /// revealAndScroll 靠它算滚动位置）
  static const double _rowHeight = 34;

  static String _loadingNodeIdOf(String path) => '$_loadingIdPrefix$path';

  static bool _isLoadingNodeId(String id) => id.startsWith(_loadingIdPrefix);

  /// 树的滚动控制器（revealAndScroll 要用）
  final ScrollController _treeScrollController = ScrollController();

  FolderProvider? _provider;
  DirectoryTreeController? _treeController;
  ExpansionController? _expansions;

  /// 这一次 reveal 不用滚动（在树里单击时节点本来就是可见的，
  /// 强行滚到顶部反而会让用户眼花）
  bool _skipScrollOnce = false;

  /// 上一次重建 TreeData 时的结构版本（只比结构，歌曲信息变化不重建树）
  int _lastStructureVersion = -1;

  /// 上一次自动展开 + 高亮时对应的 openToken / 路径
  int _lastOpenToken = -1;
  String? _lastRevealedPath;

  /// 防止「回写展开状态 → provider 发通知 → 再回写」这种循环
  bool _isWritingExpansions = false;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();

    // read 而不是 watch：provider 变化走 _onProviderChanged，不重跑整个 build
    final provider = context.read<FolderProvider>();
    if (identical(provider, _provider)) return;

    _provider?.removeListener(_onProviderChanged);
    _provider = provider;
    provider.addListener(_onProviderChanged);

    _createController(provider);

    // 文件夹段第一次出现时才初始化（拉根目录 + 恢复上次展开的树）。
    // 必须等这一帧画完：initFolderTree 会 notifyListeners，在 build 过程中发
    // 通知会把正在构建的兄弟节点标脏，Flutter 会直接报错。
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted || !identical(provider, _provider)) return;
      if (!provider.isInitialized) {
        provider.initFolderTree();
      }
    });
  }

  @override
  void dispose() {
    _provider?.removeListener(_onProviderChanged);
    _disposeController();
    _treeScrollController.dispose();
    super.dispose();
  }

  // ════════════════════════════════════════════════════════════
  // 控制器
  // ════════════════════════════════════════════════════════════

  void _createController(FolderProvider provider) {
    _disposeController();

    _expansions = ExpansionController(
      // tree_root 必须展开，否则根目录一个都看不见
      initiallyExpanded: {...provider.expandedPaths, _containerNodeId},
    );

    _treeController = DirectoryTreeController(
      data: _buildTreeData(provider),
      expansions: _expansions,
    );

    _expansions!.addListener(_onExpansionsChanged);
    _lastStructureVersion = provider.structureVersion;
    _lastOpenToken = -1;
    _lastRevealedPath = null;

    // 同理放到这一帧画完之后：reveal 会改展开状态，展开状态又会回写 provider，
    // 在 didChangeDependencies（属于 build 阶段）里做会触发 markNeedsBuild 断言
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (mounted) _revealCurrentFolder();
    });
  }

  void _disposeController() {
    _expansions?.removeListener(_onExpansionsChanged);
    _treeController?.dispose();
    _treeController = null;
    // 传进去的 ExpansionController 归我们管，directory_tree 不会替我们释放
    _expansions?.dispose();
    _expansions = null;
  }

  /// provider 状态变了
  void _onProviderChanged() {
    final provider = _provider;
    final controller = _treeController;
    if (provider == null || controller == null) return;

    if (provider.structureVersion != _lastStructureVersion) {
      _lastStructureVersion = provider.structureVersion;
      // rebuild 期间会 retainWhere 掉已经不存在的展开项，那也会触发
      // _onExpansionsChanged；这里先拦住，别在重建过程中回写 provider
      _isWritingExpansions = true;
      try {
        // rebuild 会尽量保住原来的展开 / 选中状态（不在新树里的 id 自动丢掉）
        controller.rebuild(_buildTreeData(provider));
      } finally {
        _isWritingExpansions = false;
      }
      _syncExpansionsFromProvider(provider);
    }

    _revealCurrentFolder();
  }

  /// 展开状态变了 → 回写 provider（provider 负责落盘）
  void _onExpansionsChanged() {
    if (_isWritingExpansions) return;

    final provider = _provider;
    final expansions = _expansions;
    if (provider == null || expansions == null) return;

    _isWritingExpansions = true;
    try {
      provider.setExpandedPaths(expansions.expandedIds);
    } finally {
      _isWritingExpansions = false;
    }
  }

  /// 把 provider 里已经记着、但树还没展开的目录补展开
  ///
  /// 主要用在两种时候：启动时 provider 恢复得比树晚；树重建后新出现的节点。
  void _syncExpansionsFromProvider(FolderProvider provider) {
    final expansions = _expansions;
    if (expansions == null) return;

    final missing = provider.expandedPaths
        .where((path) => !expansions.isExpanded(path))
        .toList();
    if (missing.isEmpty) return;

    _isWritingExpansions = true;
    try {
      expansions.expandAll(missing);
    } finally {
      _isWritingExpansions = false;
    }
  }

  /// 自动展开 + 高亮当前打开的目录
  ///
  /// 只在「用户打开了一个目录」（openToken 变了）或者「上次恢复的选中项还没
  /// 高亮」时做；用户自己点箭头折叠当前目录，这里不会又给它强行展开。
  void _revealCurrentFolder() {
    final provider = _provider;
    final controller = _treeController;
    if (provider == null || controller == null) return;

    final path = provider.currentPath;
    if (path == null) {
      _lastRevealedPath = null;
      _lastOpenToken = provider.openToken;
      return;
    }

    final isNewOpen = provider.openToken != _lastOpenToken;
    if (!isNewOpen && path == _lastRevealedPath) return;

    _lastOpenToken = provider.openToken;
    _lastRevealedPath = path;

    // 树里单击进来的：节点本来就在视野里，展开 + 高亮就行，别把它滚到顶部
    if (_skipScrollOnce) {
      _skipScrollOnce = false;
      controller.reveal(nodeId: path, select: true);
      return;
    }

    // 右侧列表双击 / 恢复上次打开的位置：把树滚到那个节点
    controller.revealAndScroll(
      nodeId: path,
      select: true,
      scrollController: _treeScrollController,
      rowExtent: _rowHeight,
    );
  }

  // ════════════════════════════════════════════════════════════
  // 树数据
  // ════════════════════════════════════════════════════════════

  /// 用 provider 现有的结构拼一颗树
  ///
  /// 顺序完全按后端给的顺序来（根目录、每个目录的子目录都不排序）。
  TreeData _buildTreeData(FolderProvider provider) {
    final nodes = <String, TreeNode>{};

    // 递归建：先建子节点，再把自己装进去（TreeNode.childIds 要在构造时给全）
    String addFolder(String path, String parentId) {
      final childIds = <String>[];
      for (final childPath in provider.childDirsOf(path)) {
        childIds.add(addFolder(childPath, path));
      }

      // 还没拉过子目录的文件夹：塞一个占位子节点，这样它的展开箭头会先出现，
      // 用户可以直接点箭头看树，而不用先把右侧列表刷掉。
      // 点开箭头时 _onExpanderTapped 会把真正的子目录拉回来，占位节点随即消失。
      if (!provider.isChildrenLoaded(path)) {
        final loadingId = _loadingNodeIdOf(path);
        nodes[loadingId] = TreeNode(
          id: loadingId,
          name: '读取中…',
          type: NodeType.file,
          parentId: path,
          virtualPath: '/tree/$path/__loading__',
        );
        childIds.add(loadingId);
      }

      nodes[path] = TreeNode(
        id: path,
        name: FolderProvider.baseNameOf(path),
        type: NodeType.folder,
        parentId: parentId,
        virtualPath: '/tree/$path',
        sourcePath: path,
        childIds: childIds,
      );
      return path;
    }

    final rootChildIds = <String>[];
    for (final rootPath in provider.rootPaths) {
      rootChildIds.add(addFolder(rootPath, _containerNodeId));
    }

    // 虚拟节点：单独导入的歌曲（不是磁盘目录）
    rootChildIds.add(FolderProvider.singleImportNodeId);
    nodes[FolderProvider.singleImportNodeId] = TreeNode(
      id: FolderProvider.singleImportNodeId,
      name: FolderProvider.singleImportTitle,
      type: NodeType.folder,
      parentId: _containerNodeId,
      virtualPath: '/tree/single_import',
      childIds: const [],
    );

    nodes[_containerNodeId] = TreeNode(
      id: _containerNodeId,
      name: '文件夹',
      type: NodeType.folder,
      parentId: _rootNodeId,
      virtualPath: '/tree_root',
      childIds: rootChildIds,
    );

    nodes[_rootNodeId] = TreeNode(
      id: _rootNodeId,
      name: 'root',
      type: NodeType.root,
      parentId: '',
      virtualPath: '/',
      childIds: const [_containerNodeId],
    );

    return TreeData(
      nodes: nodes,
      rootId: _rootNodeId,
      visibleRootId: _containerNodeId,
      // 不画 tree_root 那一行，根目录直接以 depth 0 出现
      omitContainerRowAtRoot: true,
    );
  }

  // ════════════════════════════════════════════════════════════
  // 交互
  // ════════════════════════════════════════════════════════════

  /// 单击节点：打开目录（右侧刷新）
  void _openNode(VisibleNode node) {
    final provider = _provider;
    final controller = _treeController;
    if (provider == null || controller == null) return;
    // 占位子节点不是真正的目录
    if (_isLoadingNodeId(node.id)) return;

    // 树里单击 → 不用滚动（见 _revealCurrentFolder）
    _skipScrollOnce = true;

    // 先高亮，别等后端回来才有反馈
    controller.selectOnly(node.id);

    if (node.id == FolderProvider.singleImportNodeId) {
      provider.openSingleImportFolder();
    } else {
      provider.openFolder(node.id);
    }
  }

  /// 点展开箭头
  ///
  /// [wasExpanded] 是点击之前的状态：false = 这次是「展开」。展开一个还没拉过
  /// 子目录的节点时，顺手拉一下子目录；但不动当前打开的目录，右侧列表不刷新。
  void _onExpanderTapped(VisibleNode node, bool wasExpanded) {
    final provider = _provider;
    if (provider == null) return;
    if (wasExpanded) return; // 正在折叠，不需要数据
    if (node.id == FolderProvider.singleImportNodeId) return;
    if (provider.isChildrenLoaded(node.id)) return;

    provider.loadFolderChildren(node.id);
  }

  // ════════════════════════════════════════════════════════════
  // 构建
  // ════════════════════════════════════════════════════════════

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final controller = _treeController;

    return Container(
      color: theme.colorNav,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          // ── 标题行（和内容页顶部那行对齐） ──
          SizedBox(
            height: 50,
            child: Padding(
              padding: const EdgeInsets.symmetric(horizontal: 12),
              child: Align(
                alignment: Alignment.centerLeft,
                child: Text(
                  '文件夹',
                  style: theme.midTextStyle.copyWith(fontWeight: FontWeight.bold),
                ),
              ),
            ),
          ),

          // ── 标题和树之间的细线 ──
          Container(height: 1, color: theme.colorEdge.withAlpha(120)),

          // ── 文件夹树 ──
          Expanded(
            child: controller == null
                ? const SizedBox.shrink()
                : DirectoryTreeTheme(
                    data: DirectoryTreeThemeData(
                      rowHeight: _rowHeight,
                      indent: 14,
                      hoverColor: theme.colorHover.withAlpha(90),
                      selectionColor: theme.colorHover,
                      focusColor: theme.colorHover,
                      animationDuration: const Duration(milliseconds: 120),
                      roundedCorners: false,
                    ),
                    child: DirectoryTreeView(
                      controller: controller,
                      scrollController: _treeScrollController,
                      // 底部留出播放条的高度，最后一个节点别被盖住。
                      // 顶部不留 padding：revealAndScroll 是按 index * 行高
                      // 算位置的，加了顶部 padding 会差几个像素
                      padding: const EdgeInsets.only(bottom: 100),
                      expanderSize: 20,
                      expanderGap: 4,
                      showScrollbar: true,
                      nodeBuilder: (context, node, state) =>
                          _buildNode(theme, node, state),
                      expanderBuilder: (context, node, isExpanded, onPressed) =>
                          _buildExpander(theme, node, isExpanded, onPressed),
                    ),
                  ),
          ),
        ],
      ),
    );
  }

  /// 一行节点：文件夹图标 + 名字（单击打开）
  Widget _buildNode(AppTheme theme, VisibleNode node, NodeVisualState state) {
    // 占位子节点：正常情况下根本来不及画（拉数据是同步的），兜底画成灰字
    if (_isLoadingNodeId(node.id)) {
      return Align(
        alignment: Alignment.centerLeft,
        child: Text(
          '读取中…',
          style: theme.littleTextStyle.copyWith(color: theme.colorEdge),
        ),
      );
    }

    final isVirtual = node.id == FolderProvider.singleImportNodeId;

    return TreeNodeTile(
      node: node,
      state: state,
      leading: Padding(
        padding: const EdgeInsets.only(left: 2, right: 6),
        child: Icon(
          isVirtual
              ? TablerIcons.fileMusic
              : (state.isExpanded ? TablerIcons.folderOpen : TablerIcons.folder),
          size: 18,
          color: isVirtual ? theme.colorTextSecond : theme.colorTextMain,
        ),
      ),
      title: Tooltip(
        message: isVirtual ? '单独导入的歌曲' : node.id,
        waitDuration: const Duration(milliseconds: 600),
        child: Text(
          node.name,
          style: theme.midTextStyle.copyWith(fontSize: 15),
          maxLines: 1,
          overflow: TextOverflow.ellipsis,
        ),
      ),
      onTap: () => _openNode(node),
    );
  }

  /// 展开 / 折叠箭头：只动树，不刷新右侧
  ///
  /// theme 从外层 build 传进来，不在 itemBuilder 里 context.watch
  /// （itemBuilder 是在 layout 阶段跑的，那里拿 InheritedWidget 不稳）
  Widget _buildExpander(
    AppTheme theme,
    VisibleNode node,
    bool isExpanded,
    VoidCallback onPressed,
  ) {
    return GestureDetector(
      behavior: HitTestBehavior.opaque,
      onTap: () {
        onPressed();
        _onExpanderTapped(node, isExpanded);
      },
      child: Icon(
        isExpanded ? TablerIcons.chevronDown : TablerIcons.chevronRight,
        size: 16,
        color: theme.colorTextSecond,
      ),
    );
  }
}
