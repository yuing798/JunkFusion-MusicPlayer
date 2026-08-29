import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/utils/utils.dart';
import 'package:provider/provider.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:smooth_scroll_multiplatform/smooth_scroll_multiplatform.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';
import '../model/song_info.dart';
import '../providers/song_provider.dart';
import '../providers/playback_provider.dart';
import '../theme/app_theme.dart';
import '../widgets/each_song.dart';
import '../widgets/combo_box.dart';

enum SortMode {
  byAddTime('添加时间', 0),
  byName('歌曲名称', 1),
  byPlayTimes('播放次数', 2);

  // 枚举字段
  final String label;
  final int value;

  const SortMode(this.label, this.value);
}

// AllMusicPage — "所有音乐" 页面
class AllMusicPage extends StatefulWidget {
  const AllMusicPage({super.key});

  @override
  State<AllMusicPage> createState() => _AllMusicPageState();
}

class _AllMusicPageState extends State<AllMusicPage> {
  // 当前选中的排序方式
  SortMode _selectedSort = SortMode.byAddTime;

  // 是否升序（true = 升序，false = 降序）
  bool _isAscending = true;

  // 是否正在导入歌曲（控制按钮禁用状态）
  bool _isImporting = false;

  // 滚动控制器（用于 ListView）
  final ScrollController _scrollController = ScrollController();

  @override
  void initState() {
    super.initState();

    _delayInit();
  }

  Future<void> _delayInit() async {
    // 读取数据（注意：如果获取不到，设置默认值）
    final frontCache = AppCache.frontCacheRef;
    final isAscending = frontCache.getBool("isAscending") ?? true; // 默认升序
    final sortValue = frontCache.getInt("sortWays") ?? 0; // 默认值需与 SortMode 对应
    final sortMode = SortMode.values.firstWhere(
      (v) => v.value == sortValue,
      orElse: () => SortMode.byAddTime,
    );

    if (!mounted) return;

    setState(() {
      _isAscending = isAscending;
      _selectedSort = sortMode;
    });
  }

  @override
  void dispose() {
    _scrollController.dispose();
    super.dispose();
  }

  // 切换升降序
  void _toggleAscending() async {
    setState(() {
      _isAscending = !_isAscending;
    });

    final frontCache = AppCache.frontCacheRef;
    await frontCache.setBool("isAscending", _isAscending);
  }

  //导入歌曲
  void _songImport(BuildContext context) async {
    _isImporting = true;
    await context.read<SongProvider>().songsImport();
    _isImporting = false;
  }

  // 获取排序后的歌曲列表
  List<SongInfo> _getSortedSongs(BuildContext context) {
    // `List.of(songs)` 创建副本（不修改原列表）
    //List<T>.of 是 Dart 中 List 类的一个工厂构造函数，
    //它的核心作用是：从一个现有的可迭代对象（Iterable）中，创建一个指定类型 T 的、新的独立列表（副本）。
    final copy = List<SongInfo>.of(context.read<SongProvider>().songs);

    switch (_selectedSort) {
      case SortMode.byAddTime:
        // 按 songId 排序（id 越大 = 越新添加）
        copy.sort(
          (a, b) => _isAscending
              ? a.songId.compareTo(b.songId)
              : b.songId.compareTo(a.songId),
        );
      // `compareTo` 是 Comparable 接口的方法，返回 -1/0/1

      case SortMode.byName:
        // 按歌曲名排序（使用字符串的 compareTo）
        copy.sort(
          (a, b) => _isAscending
              ? a.title.compareTo(b.title)
              : b.title.compareTo(a.title),
        );

      case SortMode.byPlayTimes:
        // 按播放次数排序
        copy.sort(
          (a, b) => _isAscending
              ? a.playNum.compareTo(b.playNum)
              : b.playNum.compareTo(a.playNum),
        );
    }

    return copy;
  }

  @override
  Widget build(BuildContext context) {
    final sortedSongs = _getSortedSongs(context);
    context.watch<SongProvider>();
    final theme = context.watch<AppTheme>();

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // ── 第 1 行：标题行 ──
        SizedBox(
          height: 50,
          child: Row(
            children: [
              Text('所有音乐', style: theme.bigTextStyle),
              const SizedBox(width: 15),
              Selector<SongProvider, int>(
                builder: (_, value, _) {
                  return Text('共 $value 首', style: theme.littleTextStyle);
                },
                selector: (_, songProvi) => songProvi.songCount,
              ),
            ],
          ),
        ),

        // ── 第 2 行：工具栏 ──
        SizedBox(
          height: 80,
          child: Row(
            children: [
              // ── 左侧：导入按钮 ──
              // 对应原 Vue .left-column
              SizedBox(
                height: 40,
                child: TextButton(
                  onPressed: _isImporting
                      ? null
                      : () {
                          _songImport(context);
                        },
                  style: TextButton.styleFrom(
                    backgroundColor: theme.colorHover,
                    foregroundColor: theme.colorTextMain,
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.zero, //  直角矩形
                    ),
                  ),
                  child: Text(
                    _isImporting ? '导入中...' : '导入文件',
                    style: theme.midTextStyle,
                  ),
                ),
              ),

              // 弹性空间（把右侧控件推到最右边）
              const Spacer(),

              // ── 右侧：排序下拉 + 升降序按钮 ──
              // 对应原 Vue .right-column
              Row(
                mainAxisSize: MainAxisSize.min, //根据子组件确定主轴长度
                children: [
                  // ── 排序方式下拉框 ──
                  SizedBox(
                    width: 200,
                    child: ComboBox<SortMode>(
                      value: _selectedSort,
                      items: SortMode.values,
                      itemBuilder: (mode) =>
                          Text(mode.label, style: theme.comboTextStyle),
                      onChanged: (mode) async {
                        setState(() {
                          _selectedSort = mode;
                        });
                        final frontCache = AppCache.frontCacheRef;
                        frontCache.setInt("sortWays", mode.value);
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
            ],
          ),
        ),

        // ── 第 3 部分：歌曲列表（虚拟滚动，填充剩余空间） ──
        Expanded(
          child: sortedSongs.isEmpty
              ? const SizedBox.shrink()
              : DynMouseScroll(
                  builder: (context_, controler_, physics_) {
                    return ListView.builder(
                      //ListView.builder是虚拟滚动的，而ListView是全量创建的
                      controller: controler_,
                      physics: physics_,
                      // `itemCount` 等于列表长度
                      itemCount: context.read<SongProvider>().songCount,
                      // `itemExtent` 固定每个 item 高度（性能优化）
                      // 对应原 Vue 虚拟滚动的 estimateSize: () => 80
                      itemExtent: 80,
                      // itemBuilder 构建每个 item
                      padding: EdgeInsets.only(
                        bottom:
                            context_.select<PlaybackProvider, bool>(
                              (provider) => provider.currentSong != null,
                            )
                            ? 90
                            : 0, //有歌曲的话应该padding，否则会被PlayBar给遮住
                      ),
                      // `(context, index) => Widget`
                      itemBuilder: (context, index) {
                        final song = sortedSongs[index];
                        return EachSong(
                          key: ValueKey(song.songId),
                          // `ValueKey` 基于值的唯一 Key，帮助 Flutter 识别
                          // 列表项的身份（diff 算法优化）
                          song: song,
                          onSongPlay: () {
                            context.read<PlaybackProvider>().setPlayList(
                              sortedSongs,
                            );
                          },
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
