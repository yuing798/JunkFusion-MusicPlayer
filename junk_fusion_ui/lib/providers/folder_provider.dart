import 'dart:convert';

import 'package:ffi/ffi.dart' as ffi;
import 'package:flutter/foundation.dart';
import 'package:junk_fusion_ui/Macro/FileMacro.dart';
import 'package:junk_fusion_ui/Macro/coordinatorMacro.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/model/song_basic_info.dart';
import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/utils/app_cache.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';

// ════════════════════════════════════════════════════════════════
// folder_provider.dart — 文件夹视图（左侧树 + 右侧列表）的数据源
//
// 后端接口（见 Coordinator/libExport.h）：
//   getAllRootDirView()
//     → [ "D:\\Music\\A", "D:\\Music\\B" ]           应用里注册过的根目录（数组）
//   getChildDirAndSongIdList(parentDir)
//     → { "songIdList": [id...], "dirArr": ["子目录完整路径"...] }
//       注意：这个接口会顺手触发后端重新扫描该目录、把新歌入库，所以点一次
//       目录可能不是纯读操作（见 dllManager::openNewSongFolder）
//   getSingleImportSongs()
//     → [songId, ...]                                单独导入（不是整文件夹导入）的歌
//
// ── 数据怎么存 ────────────────────────────────────────────────
//   前端不缓存歌曲元数据，只缓存「结构」和「id」：
//     _rootPaths       根目录（后端顺序）
//     _childDirs       path -> 子目录完整路径（后端顺序，前端不排序）
//     _songIds         path -> 这个目录里的 songId（后端顺序）
//   取歌的时候回 SongProvider 按 id 拿最新的 SongBasicInfo。
//
// ── 树的展开状态 ──────────────────────────────────────────────
//   展开状态存在 _expandedPaths（路径集合），由左侧树在展开/折叠时回写，
//   同时落盘到 SharedPreferences（folderTreeExpandedPaths）。下次打开文件夹段
//   时由 [initFolderTree] 从浅到深把上次展开过的目录重新拉一遍，这样树能直接
//   恢复到上次的样子，不用用户再一层层点开。
//
// ── 懒加载 ────────────────────────────────────────────────────
//   子目录是点开（或第一次展开）才向后端要的。左侧树会给还没拉过的文件夹
//   先挂一个「读取中…」的占位子节点，所以展开箭头一开始就有；点箭头就会拉
//   子目录，拉回来之后占位节点自动消失（见 folder_tree_sidebar.dart）。
// ════════════════════════════════════════════════════════════════

/// 文件夹视图的数据源
class FolderProvider extends ChangeNotifier {
  final SongProvider songProvider;

  FolderProvider({required this.songProvider}) {
    // 歌曲元数据变了（喜欢、播放次数、导入完成），右侧列表要跟着重算一次
    songProvider.addListener(_onSongsChanged);
  }

  /// 虚拟节点：单独导入的歌曲（不是磁盘上的文件夹，id 和路径不会冲突）
  static const String singleImportNodeId = '__single_import__';

  /// 虚拟节点的显示名
  static const String singleImportTitle = '单独导入';

  /// SharedPreferences：上次展开的目录路径
  static const String expandedPathsCacheKey = 'folderTreeExpandedPaths';

  /// SharedPreferences：上次打开的目录
  static const String selectedPathCacheKey = 'folderTreeSelectedPath';

  /// 根目录（后端 getAllRootDirView 的顺序）
  List<String> _rootPaths = [];
  List<String> get rootPaths => _rootPaths;

  /// 已拉过的子目录：父路径 -> 子目录完整路径（后端顺序）
  final Map<String, List<String>> _childDirs = {};

  /// 已拉过的目录里的 songId（后端顺序）
  final Map<String, List<int>> _songIds = {};

  /// 目前树上存在的目录（根目录 + 已经拉出来的子目录）
  final Set<String> _knownPaths = {};

  /// 左侧树展开的目录路径
  Set<String> _expandedPaths = {};

  /// 当前打开的目录（null = 还没选；可以是虚拟节点）
  String? _curPath;

  /// 单独导入的歌曲 id（打开虚拟节点时拉的）
  List<int> _singleImportSongIds = [];

  /// 当前列表里要显示的歌曲（已经从 SongProvider 解析成对象）
  List<SongBasicInfo> _curSongs = [];

  /// 结构版本号：根目录 / 子目录 / 展开状态一变就 +1
  ///
  /// 左侧树靠它判断「要不要重建 TreeData」，避免歌曲信息一变就把整棵树重建一遍
  int _structureVersion = 0;
  int get structureVersion => _structureVersion;

  /// 每次用户主动打开某个目录 +1；左侧树靠它判断「要不要自动展开并高亮」
  ///
  /// 和 currentPath 分开记：用户手动折叠当前目录时，不能又被这里强制展开
  int _openToken = 0;
  int get openToken => _openToken;

  bool _inited = false;

  /// 初始化过了吗（根目录拉过、上次的树状态恢复过）
  ///
  /// 左侧树第一次出现时用它判断要不要跑 [initFolderTree]（只跑一次）
  bool get isInitialized => _inited;

  // ════════════════════════════════════════════════════════════
  // 对外状态
  // ════════════════════════════════════════════════════════════

  /// 已经选了目录吗
  bool get hasSelection => _curPath != null;

  /// 当前目录的完整路径（虚拟节点是 [singleImportNodeId]）
  String? get currentPath => _curPath;

  /// 当前打开的是不是「单独导入」那个虚拟节点
  bool get isSingleImportOpen => _curPath == singleImportNodeId;

  /// 当前目录的显示名（标题 / 面包屑用）
  String get currentTitle {
    final path = _curPath;
    if (path == null) return '文件夹';
    if (path == singleImportNodeId) return singleImportTitle;
    return baseNameOf(path);
  }

  /// 当前目录下的子文件夹（后端顺序）
  List<String> get subDirsInFolder {
    final path = _curPath;
    if (path == null || path == singleImportNodeId) return const [];
    return _childDirs[path] ?? const [];
  }

  /// 当前目录下的歌曲（后端顺序，已经解析成 SongBasicInfo）
  List<SongBasicInfo> get songsInFolder => _curSongs;

  /// 某个目录的子文件夹；没拉过就是空
  List<String> childDirsOf(String path) => _childDirs[path] ?? const [];

  /// 某个目录的子目录拉过了吗（拉过但为空也算拉过）
  bool isChildrenLoaded(String path) => _childDirs.containsKey(path);

  /// 左侧树展开的目录（给树初始化用）
  Set<String> get expandedPaths => Set.unmodifiable(_expandedPaths);

  // ════════════════════════════════════════════════════════════
  // 初始化 / 恢复上次的树
  // ════════════════════════════════════════════════════════════

  /// 第一次打开「文件夹」这一段时调一次
  ///
  /// 由左侧树（widgets/nav/folder_tree_sidebar.dart）在第一次出现时触发，
  /// 不在应用启动时跑：恢复展开状态要按目录重新问后端，而每次后端都会顺手
  /// 重扫目录，放在启动阶段会白卡主界面。
  ///
  /// 只拉根目录是必须的；上次展开过的目录会按从浅到深的顺序重新拉一遍，
  /// 这样树能恢复到上次的样子。
  ///
  /// TODO(性能): 展开的目录很多时，这里会串行发起很多次后端调用，而每次
  ///   后端还会顺手重扫目录。如果打开文件夹页明显变慢，需要后端加一个
  ///   「按路径批量返回子目录」的接口，前端再一次性拿回来。
  ///
  /// TODO: 现在是「父目录折叠时，子目录的展开状态还留着」（展开集合只增不减），
  ///   所以恢复时会把已经折叠的子目录也重新拉一遍。想省掉这部分，可以在
  ///   折叠某个目录时把它后代的展开状态一起清掉。
  Future<void> initFolderTree() async {
    if (_inited) return;
    _inited = true;

    _loadRootDirs();

    final savedExpanded =
        (AppCache.frontCacheRef.getStringList(expandedPathsCacheKey) ??
                const <String>[])
            .toList()
          // 从浅到深：父目录先拉，不然子目录认不出来
          ..sort((a, b) => _depthOf(a).compareTo(_depthOf(b)));

    final restoredExpanded = <String>{};
    for (final path in savedExpanded) {
      // 上次的目录已经被删了 / 父目录没恢复成功，就跳过
      if (!_isKnownPath(path)) continue;
      _fetchChildrenInto(path);
      restoredExpanded.add(path);
    }
    // 顺手把已经不存在了的路径从缓存里清掉
    setExpandedPaths(restoredExpanded);

    final savedSelected = AppCache.frontCacheRef.getString(selectedPathCacheKey);
    if (savedSelected == singleImportNodeId) {
      _fetchSingleImportSongIds();
      _curPath = singleImportNodeId;
    } else if (savedSelected != null && _isKnownPath(savedSelected)) {
      _fetchChildrenInto(savedSelected);
      _curPath = savedSelected;
    }

    _ensureSongsForCurrentFolder();
    _refreshCurrentSongs();
    _structureVersion++;
    notifyListeners();
  }

  // ════════════════════════════════════════════════════════════
  // 打开目录（左侧树单击 / 右侧列表双击）
  // ════════════════════════════════════════════════════════════

  /// 打开某个目录：右侧列表立刻换成它的内容
  ///
  /// 无论之前有没有拉过都重新问后端一次 —— 「单击 = 刷新」，
  /// 顺便把后端重扫出来的新歌带回来。
  void openFolder(String path) {
    if (path == singleImportNodeId) {
      openSingleImportFolder();
      return;
    }

    _fetchChildrenInto(path);
    _curPath = path;
    _ensureSongsForCurrentFolder();
    _refreshCurrentSongs();
    _openToken++;
    _structureVersion++;
    AppCache.frontCacheRef.setString(selectedPathCacheKey, path);
    notifyListeners();
  }

  /// 打开「单独导入」虚拟节点
  ///
  /// 后端 getSingleImportSongs() 返回的是 isSingleFile=1 的 songId 数组；
  /// 这些歌的音频文件已经被后端拷到 <缓存目录>/songs 下，filePath 记的就是
  /// 缓存路径（见 SongImport::getCachedFileFor）。
  void openSingleImportFolder() {
    _fetchSingleImportSongIds();
    _curPath = singleImportNodeId;
    _ensureSongsForCurrentFolder();
    _refreshCurrentSongs();
    _openToken++;
    AppCache.frontCacheRef.setString(selectedPathCacheKey, singleImportNodeId);
    notifyListeners();
  }

  /// 只把某个目录的子目录拉回来（点展开箭头时用）
  ///
  /// 不动 _curPath，所以右侧正在看的列表不会被刷掉 —— 这正是「点箭头只展
  /// 开树、不刷新右侧」这条交互要的效果。
  void loadFolderChildren(String path) {
    if (path == singleImportNodeId) return;
    if (_childDirs.containsKey(path)) return;

    _fetchChildrenInto(path);
    _structureVersion++;
    notifyListeners();
  }

  // ════════════════════════════════════════════════════════════
  // 导入文件夹
  // ════════════════════════════════════════════════════════════

  /// 这个路径是不是已经注册过的根目录（重复导入时前端先拦一下）
  bool isRootPath(String path) {
    final cleaned = _trimTrailingSeparator(path);
    return _rootPaths.any((root) => _trimTrailingSeparator(root) == cleaned);
  }

  /// 重新拉一遍根目录（导入完文件夹之后调）
  void reloadRootDirs() {
    _loadRootDirs();
    _structureVersion++;
    notifyListeners();
  }

  /// 把一个文件夹导入后端，并刷新根目录
  ///
  /// 这里只负责「导入 + 让树认识它」；打开由调用方再调 [openFolder]。
  /// 导入过程中每首歌都会走后端的 onAddOrUpdateSong，前端 SongProvider 的缓存
  /// 会自动补齐，所以不需要再整表重拉。
  ///
  /// FIXME: importNewSongFolder 是同步的：它会把整个文件夹递归扫一遍，每个
  ///   文件都跑 ffmpeg + taglib 再写库，大文件夹会把界面卡住。最好由后端开
  ///   线程导入、导完再回调前端（顺带把 onDirHasRegister 也注册给前端，
  ///   现在这个回调根本没设，重复导入是静默的）。
  void importFolder(String path) {
    final dirPtr = UtilFunction.string2cPtr(path);
    try {
      bindings.importNewSongFolder(dirPtr);
    } finally {
      ffi.malloc.free(dirPtr);
    }

    reloadRootDirs();
  }

  // ════════════════════════════════════════════════════════════
  // 展开状态（左侧树回写，同时落盘）
  // ════════════════════════════════════════════════════════════

  /// 左侧树的展开状态变了
  void setExpandedPaths(Iterable<String> paths) {
    final next = paths.where((path) => path.isNotEmpty).toSet();
    if (setEquals(next, _expandedPaths)) return;

    _expandedPaths = next;
    AppCache.frontCacheRef.setStringList(
      expandedPathsCacheKey,
      next.toList(),
    );
    notifyListeners();
  }

  // ════════════════════════════════════════════════════════════
  // 和 SongProvider 同步
  // ════════════════════════════════════════════════════════════

  void _onSongsChanged() {
    _refreshCurrentSongs();
    notifyListeners();
  }

  // ════════════════════════════════════════════════════════════
  // 后端调用
  // ════════════════════════════════════════════════════════════

  void _loadRootDirs() {
    var roots = <String>[];
    try {
      final raw = UtilFunction.cPtr2String(bindings.getAllRootDirView());
      final decoded = jsonDecode(raw);
      if (decoded is List) {
        roots = decoded.map((each) => each.toString()).toList();
      }
    } catch (error) {
      print('[FolderProvider] getAllRootDirView 失败：$error');
    }

    _rootPaths = roots;
    _knownPaths.addAll(roots);
  }

  /// 拉某个目录的子目录 + songId，写进缓存（不发通知，调用方自己决定）
  ///
  /// 解析失败时**不覆盖**原来缓存的内容（刷新失败就保持上次的样子，
  /// 别把已经展开的树刷成空的）
  void _fetchChildrenInto(String path) {
    var dirs = <String>[];
    var ids = <int>[];
    var parsed = false;

    final pathPtr = UtilFunction.string2cPtr(path);
    try {
      final raw = UtilFunction.cPtr2String(
        bindings.getChildDirAndSongIdList(pathPtr),
      );
      final decoded = jsonDecode(raw);
      if (decoded is Map) {
        final dirsRaw = decoded[FileMacro.dirArr] as List<dynamic>?;
        final idsRaw = decoded[CoordinatorMacro.songIdList] as List<dynamic>?;
        if (dirsRaw != null) {
          // 后端顺序，前端不排序
          dirs = dirsRaw.map((each) => each.toString()).toList();
        }
        if (idsRaw != null) {
          ids = idsRaw.map((each) => (each as num).toInt()).toList();
        }
        parsed = true;
      }
    } catch (error) {
      // TODO: 失败时只打日志，界面上没有提示 —— 树里那个「读取中…」占位会一直
      //   留着。等有统一的错误弹窗（DialogUtil / 后端 registerErrorSendCallback）
      //   再接一下。
      print('[FolderProvider] getChildDirAndSongIdList 失败：$path / $error');
    } finally {
      // 传进去的 C 字符串要自己释放（回来的那个字符串 cPtr2String 已经释放了）
      ffi.malloc.free(pathPtr);
    }

    if (!parsed) return;

    _childDirs[path] = dirs;
    _songIds[path] = ids;
    _knownPaths.addAll(dirs);
  }

  void _fetchSingleImportSongIds() {
    var ids = <int>[];
    try {
      final raw = UtilFunction.cPtr2String(bindings.getSingleImportSongs());
      final decoded = jsonDecode(raw);
      if (decoded is List) {
        ids = decoded.map((each) => (each as num).toInt()).toList();
      }
    } catch (error) {
      print('[FolderProvider] getSingleImportSongs 失败：$error');
    }
    _singleImportSongIds = ids;
  }

  // ════════════════════════════════════════════════════════════
  // 内部工具
  // ════════════════════════════════════════════════════════════

  /// 打开目录前先把缺的歌曲基本信息补上
  ///
  /// 后端返回的只有 songId；SongProvider 现在是「id -> 基本信息」的缓存，
  /// 没有的就去后端要一份（见 SongProvider.ensureSongsLoaded）。
  /// 补完 SongProvider 会 notify，_onSongsChanged 里会重新解析一次列表。
  void _ensureSongsForCurrentFolder() {
    final path = _curPath;
    if (path == null) return;

    final ids = path == singleImportNodeId
        ? _singleImportSongIds
        : (_songIds[path] ?? const <int>[]);

    final missing = ids
        .where((id) => !songProvider.idToInfoMap.containsKey(id))
        .toList();
    if (missing.isEmpty) return;

    songProvider.ensureSongsLoaded(missing);
  }

  /// 把当前目录的 songId 解析成 SongBasicInfo（按后端顺序）
  void _refreshCurrentSongs() {
    final path = _curPath;
    if (path == null) {
      _curSongs = [];
      return;
    }

    final ids = path == singleImportNodeId
        ? _singleImportSongIds
        : (_songIds[path] ?? const <int>[]);

    // 用列表而不是 set：输出顺序必须和 ids（= 后端顺序）一致
    final resolved = <SongBasicInfo>[];
    for (final id in ids) {
      final song = songProvider.idToInfoMap[id];
      if (song == null) continue; // 还没补到的先跳过，补到之后会自己刷新
      resolved.add(song);
    }
    _curSongs = resolved;
  }

  /// 这个路径现在能不能在树上出现
  bool _isKnownPath(String path) {
    if (path.isEmpty) return false;
    if (path == singleImportNodeId) return true;
    return _knownPaths.contains(path);
  }

  /// 取路径最后一段当显示名（同时认 Windows 的 \ 和 /）
  static String baseNameOf(String path) {
    final normalized = _trimTrailingSeparator(path);
    if (normalized.isEmpty) return path;
    final index = normalized.lastIndexOf(RegExp(r'[\\/]'));
    if (index < 0) return normalized;
    final name = normalized.substring(index + 1);
    return name.isEmpty ? normalized : name;
  }

  /// 去掉路径结尾的分隔符（"D:\Music\" -> "D:\Music"）
  ///
  /// FilePicker 偶尔会带回来结尾分隔符，juce 那边存的是规范化的路径，
  /// 不统一的话树上会冒出两个指向同一目录的节点。
  static String trimPath(String path) => _trimTrailingSeparator(path);

  /// 去掉结尾的分隔符（内部共用）
  static String _trimTrailingSeparator(String path) {
    var normalized = path.trim();
    while (normalized.length > 1 &&
        (normalized.endsWith('\\') || normalized.endsWith('/'))) {
      normalized = normalized.substring(0, normalized.length - 1);
    }
    return normalized;
  }

  /// 路径层级（用来把恢复顺序从浅到深排）
  static int _depthOf(String path) {
    return path
        .split(RegExp(r'[\\/]+'))
        .where((segment) => segment.isNotEmpty)
        .length;
  }
}
