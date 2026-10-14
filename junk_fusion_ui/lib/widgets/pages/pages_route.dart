import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/album_basic_info.dart';
import 'package:junk_fusion_ui/model/album_detail_info.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/widgets/pages/album_view_page.dart';
import 'package:junk_fusion_ui/widgets/pages/all_music_page.dart';
import 'package:junk_fusion_ui/widgets/pages/single_album_page.dart';
import 'package:provider/provider.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

// ════════════════════════════════════════════════════════════════
// pages_route.dart — 页面路由
//
// 这里管三件事：
//   1. PagesRouteTable —— 路由表：哪个 navId 对应哪个页面。页面只在
//      这里 new，别的地方不要再直接 new 页面；
//   2. PagesRouteController —— 页面栈：左侧导航栏点了什么、现在在哪一页、
//      还能不能返回，全归它管（ChangeNotifier，App 里 provide 一份）；
//   3. PagesRouteView —— 把当前这一页画出来，并在左上角补一个返回按钮
//      （栈底 = 最开始那一页时按钮禁用）。
//
// 左侧导航栏（widgets/left_column.dart）不再自己存「选中项」，而是读
// PagesRouteController.currentNavId，所以点左侧按钮 / 点返回按钮之后，
// 高亮都会自动同步。
//
// 注意：这里不是 Flutter 的 Navigator，只是一个自己维护的页面栈 ——
// 页面都画在主内容区里，左侧导航栏和底部播放条不参与路由。
// ════════════════════════════════════════════════════════════════

/// 一个路由项 = 一页
class PagesRouteEntry {
  /// 这一页对应左侧导航栏的哪个按钮；null = 左侧没有对应的按钮
  final int? navId;

  /// 页面名字（占位页、tooltip 用得到）
  final String title;

  /// 造页面
  final WidgetBuilder builder;

  const PagesRouteEntry({
    this.navId,
    required this.title,
    required this.builder,
  });
}

// ════════════════════════════════════════════════════════════════
// 路由表
//
// navId 和 widgets/left_column.dart 里 _NavButton 的 id 一一对应：
//   0 所有音乐   1 我喜欢     2 最近播放
//   3 作者       4 专辑       5 歌单      6 风格
//   7 AI助手     8 效果器     9 均衡器    10 音箱阵列   11 设置
// 目前只有 0 / 4 做完了页面，其它的先走占位页。
// ════════════════════════════════════════════════════════════════
abstract class PagesRouteTable {
  /// 左侧导航栏「所有音乐」的 id
  static const int allMusicId = 0;

  /// 左侧导航栏「专辑」的 id
  static const int albumViewId = 4;

  /// 所有音乐
  static const PagesRouteEntry allMusic = PagesRouteEntry(
    navId: allMusicId,
    title: '所有音乐',
    builder: _buildAllMusic,
  );

  /// 专辑（网格视图）
  static const PagesRouteEntry albumView = PagesRouteEntry(
    navId: albumViewId,
    title: '专辑',
    builder: _buildAlbumView,
  );

  /// 单张专辑：要把专辑本身带进去，所以用函数现造一个路由项
  ///
  /// 用法（比如专辑页点卡片进单张专辑）：
  ///   context.read<PagesRouteController>().push(
  ///     PagesRouteTable.singleAlbum(album),
  ///   );
  ///
  /// TODO: album_view_page.dart 现在还是自己在页面内部切到 SingleAlbumPage
  ///       （本页状态 _openedAlbum），等那边改完再改成走这条路由。
  static PagesRouteEntry singleAlbum(AlbumBasicInfo album) {
    return PagesRouteEntry(
      // 单张专辑属于「专辑」这一栏，左侧按钮继续高亮专辑
      navId: albumViewId,
      title: album.title,
      builder: (context) => SingleAlbumPage(),
    );
  }

  /// 还没开发的页面：占位
  ///
  /// TODO: 页面写完之后，在 byNavId() 里给它加一条真的路由项
  static PagesRouteEntry placeholder({
    required int navId,
    required String title,
  }) {
    return PagesRouteEntry(
      navId: navId,
      title: title,
      builder: (context) => _PlaceholderPage(title: title),
    );
  }

  /// 左侧导航栏的 id → 路由项；返回 null 表示这一页还没写
  static PagesRouteEntry? byNavId(int navId) {
    if (navId == allMusicId) return allMusic;
    if (navId == albumViewId) return albumView;
    return null;
  }
}

Widget _buildAllMusic(BuildContext context) => const AllMusicPage();

Widget _buildAlbumView(BuildContext context) => const AlbumViewPage();

// ════════════════════════════════════════════════════════════════
// 页面栈
//
// 栈底（index 0）是应用打开时的第一页，删不掉；canGoBack == false 就表示
// 已经退到最开始那一页了。
// ════════════════════════════════════════════════════════════════
class PagesRouteController extends ChangeNotifier {
  /// 页面栈
  final List<PagesRouteEntry> _stack = <PagesRouteEntry>[
    PagesRouteTable.allMusic,
  ];

  /// 现在正在显示的这一页
  PagesRouteEntry get current => _stack.last;

  /// 左侧导航栏该高亮哪个按钮（null = 一个都不高亮）
  int? get currentNavId => current.navId;

  /// 还有上一页吗；false = 已经在最开始那一页，返回按钮要禁用
  bool get canGoBack => _stack.length > 1;

  /// 页面栈的层数
  int get stackDepth => _stack.length;

  /// 左侧导航栏点了某个按钮
  ///
  /// [title] 是这个按钮上的文字，只有占位页用得到（路由表里还没这一页的时候）
  void openNav(int navId, {required String title}) {
    // 点的就是现在这一栏 → 不重复入栈
    if (navId == currentNavId) return;

    _push(
      PagesRouteTable.byNavId(navId) ??
          PagesRouteTable.placeholder(navId: navId, title: title),
    );
  }

  /// 进一页：页面内部要跳转的时候用（比如专辑页点开单张专辑）
  void push(PagesRouteEntry entry) => _push(entry);

  /// 返回上一页；已经在栈底就什么都不做（返回按钮那边也会是禁用态）
  void goBack() {
    if (!canGoBack) return;

    _stack.removeLast();
    notifyListeners();
  }

  void _push(PagesRouteEntry entry) {
    _stack.add(entry);
    notifyListeners();
  }
}

// ════════════════════════════════════════════════════════════════
// 内容区：当前这一页 + 左上角的返回按钮
// ════════════════════════════════════════════════════════════════
class PagesRouteView extends StatelessWidget {
  const PagesRouteView({super.key});

  @override
  Widget build(BuildContext context) {
    // watch：换页了、能/不能返回了，都要重画
    final pagesRoute = context.watch<PagesRouteController>();
    final theme = context.watch<AppTheme>();

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // ── 左上角：返回上一页 ──
        SizedBox(
          height: 40,
          child: Row(
            children: [
              IconButton(
                icon: const Icon(TablerIcons.arrowLeft, size: 32),
                // onPressed = null 就是禁用态（IconButton 自己会画成灰的）
                onPressed: pagesRoute.canGoBack ? pagesRoute.goBack : null,
                tooltip: pagesRoute.canGoBack ? '返回上一页' : '已经是第一页了',
                color: theme.colorTextMain,
                disabledColor: theme.colorEdge,
                // 默认的 IconButton 自带 8 的 padding + 40x40 最小尺寸，
                // 图标 32 会撑出这 40 高的行，这里把它压回 40x40
                style: IconButton.styleFrom(
                  padding: EdgeInsets.zero,
                  minimumSize: const Size(40, 40),
                  tapTargetSize: MaterialTapTargetSize.shrinkWrap,
                ),
              ),
            ],
          ),
        ),

        // ── 当前这一页 ──
        Expanded(child: Builder(builder: pagesRoute.current.builder)),
      ],
    );
  }
}

// ════════════════════════════════════════════════════════════════
// _PlaceholderPage — 还没开发的页面
// ════════════════════════════════════════════════════════════════
class _PlaceholderPage extends StatelessWidget {
  final String title;

  const _PlaceholderPage({required this.title});

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();

    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Text(title, style: theme.bigTextStyle),
          const SizedBox(height: 8),
          // TODO: 这个页面还没写；写完在 PagesRouteTable.byNavId() 里
          //       换成真的页面就行
          Text('这一页还没写', style: theme.littleTextStyle),
        ],
      ),
    );
  }
}
