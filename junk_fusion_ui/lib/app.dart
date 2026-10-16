import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/widgets/PlayControl/song_play_page.dart';
import 'package:junk_fusion_ui/widgets/pages/pages_route.dart';
import 'package:junk_fusion_ui/widgets/title_bar.dart';
import 'package:provider/provider.dart';
import 'theme/app_theme.dart';
import 'widgets/left_column.dart';
import 'widgets/PlayControl/play_bar.dart';

class App extends StatefulWidget {
  const App({super.key});

  @override
  State<App> createState() => _AppState();
}

// _AppState — App 的内部状态类

class _AppState extends State<App> {
  // 页面路由（页面栈 + 左侧导航栏高亮）
  //
  // 具体有哪些页面、怎么跳转全在 widgets/pages/pages_route.dart 里，
  // 这里只负责拿着这一份状态，让左侧导航栏和内容区都能读到
  final PagesRouteController _pagesRoute = PagesRouteController();

  @override
  void dispose() {
    _pagesRoute.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    // 路由状态要同时给 LeftColumn（左侧导航栏）和 PagesRouteView（内容区）用，
    // 所以 provide 在它们上面
    return ChangeNotifierProvider<PagesRouteController>.value(
      value: _pagesRoute,
      child: Scaffold(
        body: Stack(
          children: [
            // 同步播放状态到 Windows 任务栏（无可见 UI）
            // const WindowsTaskbarController(),

            // --- 主内容区域（占满剩余空间，底部留 90px 给 PlayBar） ---
            // Positioned 替代了原来的 Expanded：Stack 的子元素不支持 Expanded
            Positioned(
              top: 0,
              left: 0,
              right: 0,
              bottom: 0, // 顶满整个 Stack
              child: Row(
                children: [
                  const SizedBox(child: LeftColumn()),

                  // 右侧主内容区
                  // Expanded 让主内容区占据剩余的所有宽度
                  Expanded(
                    child: ColoredBox(
                      //能够单独设置颜色的组件
                      color: context.watch<AppTheme>().colorMain,
                      child: Column(
                        children: [
                          const TitleBar(),
                          Expanded(
                            child: Padding(
                              padding: const EdgeInsets.symmetric(
                                horizontal: 10,
                              ),
                              // 当前页面 + 左上角的返回按钮都在这里面
                              child: const PagesRouteView(),
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
                ],
              ),
            ),

            Positioned(
              left: 0,
              right: 0,
              bottom: 0,
              // height: 90,
              child: const PlayBar(),
            ),

            Positioned(
              left: 0,
              right: 0,
              bottom: 0,
              top: 0,
              child: const SongPlayPage(),
            ),
          ],
        ),
      ),
    );
  }
}
