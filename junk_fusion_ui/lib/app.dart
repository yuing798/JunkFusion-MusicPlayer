import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/widgets/PlayControl/song_play_page.dart';
import 'package:junk_fusion_ui/widgets/layout/three_column_layout.dart';
import 'package:junk_fusion_ui/widgets/pages/pages_route.dart';
import 'package:junk_fusion_ui/widgets/title_bar.dart';
import 'package:provider/provider.dart';
import 'theme/app_theme.dart';
import 'widgets/PlayControl/play_bar.dart';

class App extends StatefulWidget {
  const App({super.key});

  @override
  State<App> createState() => _AppState();
}

// _AppState — App 的内部状态类
//
// 窗口结构（自外向内）：
//   Stack
//     ├─ 顶栏（通栏：拖窗口 + 最小化/最大化/关闭）
//     └─ 三段式（图标栏 | 中间栏 | 主内容）   ← ThreeColumnLayout
//     ├─ 底部播放栏（浮层，横跨整个窗口）
//     └─ 全屏播放页（浮层，现在还是占位）
//
// 三段式的宽度、拖拽都在 widgets/layout/ 里，这个文件只管把状态 provide 好。

class _AppState extends State<App> {
  // 页面路由（页面栈 + 左侧图标栏高亮 + 中间栏显示哪一段）
  //
  // 具体有哪些页面、怎么跳转全在 widgets/pages/pages_route.dart 里，
  // 这里只负责拿着这一份状态，让图标栏 / 中间栏 / 内容区都能读到
  final PagesRouteController _pagesRoute = PagesRouteController();

  @override
  void dispose() {
    _pagesRoute.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    // 路由状态要同时给 IconRail（图标栏）、SidebarHost（中间栏）和
    // PagesRouteView（内容区）用，所以 provide 在它们上面
    return ChangeNotifierProvider<PagesRouteController>.value(
      value: _pagesRoute,
      child: Scaffold(
        body: Stack(
          children: [
            // 同步播放状态到 Windows 任务栏（无可见 UI）
            // const WindowsTaskbarController(),

            // ── 顶栏 + 三段式（图标栏 | 中间栏 | 主内容） ──
            Positioned.fill(
              child: ColoredBox(
                // 整个窗口的底色；图标栏和中间栏自己再涂 colorNav
                color: context.watch<AppTheme>().colorMain,
                child: Column(
                  children: [
                    // 顶栏通栏：窗口拖拽区 + 右侧三个窗口按钮
                    const TitleBar(),

                    Expanded(
                      child: ThreeColumnLayout(
                        // 右侧主内容：当前页面 + 左上角的返回按钮
                        child: const Padding(
                          padding: EdgeInsets.symmetric(horizontal: 10),
                          child: PagesRouteView(),
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ),

            // ── 底部播放栏（浮在最上面，横跨整个窗口） ──
            Positioned(
              left: 0,
              right: 0,
              bottom: 0,
              child: const PlayBar(),
            ),

            // ── 全屏播放页（浮层，避开 50px 的标题栏，方便拖窗口/关闭） ──
            Positioned(
              left: 0,
              right: 0,
              top: 50,
              bottom: 0,
              child: const SongPlayPage(),
            ),
          ],
        ),
      ),
    );
  }
}
