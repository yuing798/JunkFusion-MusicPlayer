/// ════════════════════════════════════════════════════════════════
/// all_music_page.dart — "所有音乐" 页面
///
/// 对应原 Vue 项目 components/pages/AllMusic.vue
///
/// 页面结构：
///   第 1 行：标题 "所有音乐" + "共 N 首"
///   第 2 行：工具栏（导入按钮 + 排序下拉 + 升降序切换）
///   剩余：虚拟滚动歌曲列表（ListView.builder）
///
/// 排序功能：
///   0 = 添加时间排序（songId 升/降）
///   1 = 歌曲名称排序（按拼音字母）
///   2 = 播放次数排序（playNum 升/降）
///
/// Dart 语法说明：
/// - `ListView.builder` 是 Flutter 内置的高性能列表组件
///   它只构建屏幕上可见的 item（类似 @tanstack/vue-virtual）
/// - `itemCount` 指定总 item 数
/// - `itemBuilder` 是构建每个 item 的回调函数
/// - `DropdownButton` 是 Flutter Material 下拉选择框
/// - `Enum` 定义枚举类型（更好的类型安全）
/// ════════════════════════════════════════════════════════════════

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/song_info.dart';
import '../providers/song_provider.dart';
import '../providers/playback_provider.dart';
import '../theme/app_theme.dart';
import '../widgets/each_song.dart';

/// SortMode — 排序方式枚举
///
/// Dart 语法：
/// - `enum` 定义枚举类型，比原始 int 更安全
/// - 每个枚举值可以有自己的 label（通过构造函数）
enum SortMode {
  /// 按添加时间排序（对应 C++ SortMode::ByAddTime）
  byAddTime('添加时间'),

  /// 按歌曲名称排序（对应 C++ SortMode::ByName）
  byName('歌曲名称'),

  /// 按播放次数排序（对应 C++ SortMode::ByPlayTimes）
  byPlayTimes('播放次数');

  /// 枚举字段
  final String label;

  /// 枚举构造函数（const）
  ///
  /// Dart 语法：
  /// - `const SortMode(this.label)` 是常量构造函数
  /// - 枚举的每个值在定义时调用构造函数
  const SortMode(this.label);
}

/// AllMusicPage — "所有音乐" 页面
///
/// 这是目前唯一实现的页面（pageId = 0）
class AllMusicPage extends StatefulWidget {
  const AllMusicPage({super.key});

  @override
  State<AllMusicPage> createState() => _AllMusicPageState();
}

class _AllMusicPageState extends State<AllMusicPage> {
  // ════════════════════════════════════════════════════════════════
  // 状态字段
  // ════════════════════════════════════════════════════════════════

  /// 当前选中的排序方式
  SortMode _selectedSort = SortMode.byAddTime;

  /// 是否升序（true = 升序，false = 降序）
  bool _isAscending = true;

  /// 是否正在导入歌曲（控制按钮禁用状态）
  bool _isImporting = false;

  /// 歌曲总数
  int _songCount = 0;

  /// 滚动控制器（用于 ListView）
  final ScrollController _scrollController = ScrollController();

  // ════════════════════════════════════════════════════════════════
  // 生命周期
  // ════════════════════════════════════════════════════════════════

  @override
  void initState() {
    super.initState();

    // TODO: 桥接层 - 从持久化存储恢复排序设置
    // 原 Vue 代码:
    //   isAscending.value = localStorage.getItem('AllMusic_isascending') !== 'false';
    //   const sortWaysLoad = localStorage.getItem('AllMusic_sortMode');
    //   if (sortWaysLoad !== null) selectedSort.value = Number(sortWaysLoad);

    // 延迟加载数据
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _refreshSongCount();
      context.read<PlaybackProvider>().restoreCurrentSongId();
    });
  }

  @override
  void dispose() {
    _scrollController.dispose();
    super.dispose();
  }

  // ════════════════════════════════════════════════════════════════
  // 操作方法
  // ════════════════════════════════════════════════════════════════

  /// 导入歌曲
  ///
  /// 对应原 Vue: songImport()
  Future<void> _songImport() async {
    if (_isImporting) return;

    setState(() {
      _isImporting = true;
    });

    try {
      // TODO: 桥接层 - 调用原生函数导入歌曲
      // 原 Vue 代码:
      //   const obj = await getNativeFunction(B_songImport.name)();
      //   if (obj && typeof obj === 'object') {
      //     const numImport = obj[B_songImport.numImport];
      //     const numSuccess = obj[B_songImport.numSuccess];
      //     songStore.songs.push(...obj[B_songImport.songs]);
      //     // 构建导入结果信息并显示
      //     showInfoWindow(windowText);
      //   }

      await _refreshSongCount();
    } catch (e) {
      // TODO: 桥接层 - 显示错误信息
      // showInfoWindow(e);
    } finally {
      if (mounted) {
        setState(() {
          _isImporting = false;
        });
      }
    }
  }

  /// 刷新歌曲总数
  ///
  /// 对应原 Vue: refreshSongCount()
  Future<void> _refreshSongCount() async {
    // TODO: 桥接层 - 调用原生函数获取歌曲总数
    // 原 Vue 代码:
    //   const obj = await getNativeFunction(B_getAllSongCount.name)();
    //   if (obj && typeof obj === 'object' && B_getAllSongCount.count in obj) {
    //     songCount.value = obj[B_getAllSongCount.count];
    //   }

    // 当前使用本地歌曲列表的长度
    if (mounted) {
      final songStore = context.read<SongProvider>();
      setState(() {
        _songCount = songStore.songs.length;
      });
    }
  }

  /// 切换升降序
  void _toggleAscending() {
    setState(() {
      _isAscending = !_isAscending;
    });

    // TODO: 桥接层 - 持久化排序方向
    // 原 Vue: localStorage.setItem('AllMusic_isascending', String(isAscending.value));
  }

  /// 获取排序后的歌曲列表
  ///
  /// 对应原 Vue: sortSongs computed
  List<SongInfo> _getSortedSongs(List<SongInfo> songs) {
    // `List.of(songs)` 创建副本（不修改原列表）
    final copy = List<SongInfo>.of(songs);

    switch (_selectedSort) {
      case SortMode.byAddTime:
        // 按 songId 排序（id 越大 = 越新添加）
        copy.sort(
            (a, b) => _isAscending ? a.songId.compareTo(b.songId) : b.songId.compareTo(a.songId));
        // `compareTo` 是 Comparable 接口的方法，返回 -1/0/1

      case SortMode.byName:
        // 按歌曲名排序（使用字符串的 compareTo）
        copy.sort((a, b) => _isAscending
            ? a.title.compareTo(b.title)
            : b.title.compareTo(a.title));

      case SortMode.byPlayTimes:
        // 按播放次数排序
        copy.sort((a, b) => _isAscending
            ? a.playNum.compareTo(b.playNum)
            : b.playNum.compareTo(a.playNum));
    }

    return copy;
  }

  // ════════════════════════════════════════════════════════════════
  // build
  // ════════════════════════════════════════════════════════════════

  @override
  Widget build(BuildContext context) {
    // 监听歌曲列表变化
    final songStore = context.watch<SongProvider>();
    final sortedSongs = _getSortedSongs(songStore.songs);

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // ── 第 1 行：标题行 ──
        _buildHeaderRow(),

        // ── 第 2 行：工具栏 ──
        _buildToolbarRow(),

        // ── 第 3 部分：歌曲列表（虚拟滚动，填充剩余空间） ──
        Expanded(
          child: sortedSongs.isEmpty
              ? const Center(
                  child: Text('暂无歌曲',
                      style: AppTheme.midTextStyle))
              : ListView.builder(
                  controller: _scrollController,
                  // `itemCount` 等于列表长度
                  itemCount: sortedSongs.length,
                  // `itemExtent` 固定每个 item 高度（性能优化）
                  // 对应原 Vue 虚拟滚动的 estimateSize: () => 80
                  itemExtent: 80,
                  // itemBuilder 构建每个 item
                  // `(context, index) => Widget`
                  itemBuilder: (context, index) {
                    final song = sortedSongs[index];
                    return EachSong(
                      key: ValueKey(song.songId),
                      // `ValueKey` 基于值的唯一 Key，帮助 Flutter 识别
                      // 列表项的身份（diff 算法优化）
                      song: song,
                    );
                  },
                ),
        ),
      ],
    );
  }

  /// 构建标题行："所有音乐" + "共 N 首"
  ///
  /// 对应原 Vue .header-row
  Widget _buildHeaderRow() {
    return SizedBox(
      height: 50,
      child: Row(
        children: [
          Text(
            '所有音乐',
            style: AppTheme.bigTextStyle,
          ),
          const SizedBox(width: 15),
          Text(
            '共 $_songCount 首', // `$_songCount` 字符串插值
            style: AppTheme.littleTextStyle,
          ),
        ],
      ),
    );
  }

  /// 构建工具栏行：导入按钮 + 排序下拉 + 升降序切换
  ///
  /// 对应原 Vue .toolbar-row
  Widget _buildToolbarRow() {
    return SizedBox(
      height: 80,
      child: Row(
        children: [
          // ── 左侧：导入按钮 ──
          // 对应原 Vue .left-column
          _buildImportButton(),

          // 弹性空间（把右侧控件推到最右边）
          const Spacer(),

          // ── 右侧：排序下拉 + 升降序按钮 ──
          // 对应原 Vue .right-column
          _buildSortControls(),
        ],
      ),
    );
  }

  /// 构建"导入文件/扫描文件夹"按钮
  ///
  /// 对应原 Vue .btn-import
  Widget _buildImportButton() {
    return Padding(
      padding: const EdgeInsets.all(10),
      child: SizedBox(
        height: 40,
        child: ElevatedButton(
          onPressed: _isImporting ? null : _songImport,
          style: ElevatedButton.styleFrom(
            backgroundColor: AppTheme.colorClicked,
            foregroundColor: AppTheme.colorTextMain,
            shape: RoundedRectangleBorder(
              borderRadius:
                  BorderRadius.circular(AppTheme.borderRadius),
            ),
            minimumSize: const Size(180, 40),
            textStyle: AppTheme.midTextStyle,
          ),
          child: Text(_isImporting ? '导入中...' : '导入文件/扫描文件夹'),
        ),
      ),
    );
  }

  /// 构建排序控件（下拉框 + 升降序切换）
  ///
  /// 对应原 Vue .right-column
  Widget _buildSortControls() {
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        // ── 排序方式下拉框 ──
        // 对应原 Vue <select v-model="selectedSort" class="combo-sort">
        SizedBox(
          width: 200,
          child: DropdownButton<SortMode>(
            value: _selectedSort,
            // `isExpanded: true` 让下拉按钮撑满容器宽度
            isExpanded: true,
            // `underline` 去掉底部下划线
            underline: const SizedBox(),
            // `icon` 自定义下拉箭头
            icon: const Icon(Icons.arrow_drop_down),
            style: AppTheme.midTextStyle,
            // `items` 定义下拉选项
            items: SortMode.values.map((mode) {
              return DropdownMenuItem(
                value: mode,
                child: Text(mode.label),
              );
            }).toList(),
            // `onChanged` 选中变化回调
            onChanged: (SortMode? newMode) {
              if (newMode != null) {
                setState(() {
                  _selectedSort = newMode;
                });
                // TODO: 桥接层 - 持久化排序方式
              }
            },
          ),
        ),

        const SizedBox(width: 8),

        // ── 升降序切换按钮 ──
        IconButton(
          icon: Icon(
            _isAscending
                ? Icons.arrow_upward
                : Icons.arrow_downward,
            size: 28,
          ),
          onPressed: _toggleAscending,
          tooltip: _isAscending ? '升序' : '降序',
          color: AppTheme.colorTextMain,
        ),
      ],
    );
  }
}

/// ════════════════════════════════════════════════════════════════
/// ListView.builder 虚拟滚动原理：
///
/// Flutter 的 ListView.builder 天然是"懒加载"的：
/// 它只构建当前在屏幕上可见（加少量预渲染）的 item。
///
/// 关键参数：
/// - `itemCount`：总 item 数量
/// - `itemExtent`：固定 item 高度（如果所有 item 等高的话）
///   设置后性能大幅提升，因为不需要测量每个 item 的高度
/// - `itemBuilder`：只在 item 出现在视口中时才调用
///
/// 这等价于 @tanstack/vue-virtual 的核心功能：
///   Vue:  useVirtualizer({ count, estimateSize, overscan })
///   Flutter: ListView.builder(itemCount, itemExtent)  // overscan 自动管理
/// ════════════════════════════════════════════════════════════════
