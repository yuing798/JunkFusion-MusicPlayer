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
//     ├─ Column
//     │    ├─ 顶栏（通栏：拖窗口 + 最小化/最大化/关闭）
//     │    ├─ 三段式（图标栏 | 中间栏 | 主内容）   ← ThreeColumnLayout（Expanded）
//     │    └─ 底部播放栏                            ← 常驻，占布局空间
//     └─ 全屏播放页（浮层，含独立主题、窗口顶栏和完整播放栏）
//
// 播放栏不在浮层里，所以各页面不需要为它留底部空白；
// 唯一的浮层是全屏播放页。
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

            // ── 顶栏 + 三段式 + 常驻播放栏 ──
            //
            // 播放栏现在是 Column 的一部分（不再是浮层）：
            //   · 内容区永远不会被它盖住，所以各页面不用再算"底部留白"；
            //   · 窗口缩放时它跟着一起缩，不会出现"栏浮在内容上"的错觉。
            // 真正的浮层只剩全屏播放页（它要盖住顶栏，仍然留在 Stack 里）。
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

                    // 底部播放栏：常驻，属于布局而不是浮层
                    const PlayBar(),
                  ],
                ),
              ),
            ),

            // ── 全屏播放页：背景覆盖顶栏，页内复用窗口控制和完整播放栏 ──
            Positioned.fill(child: const SongPlayPage()),
          ],
        ),
      ),
    );
  }
}
