import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/widgets/PlayControl/song_play_page.dart';
import 'package:junk_fusion_ui/widgets/title_bar.dart';
import 'package:provider/provider.dart';
import 'theme/app_theme.dart';
import 'widgets/left_column.dart';
import 'widgets/PlayControl/play_bar.dart';
import 'pages/all_music_page.dart';

class App extends StatefulWidget {
  const App({super.key});

  @override
  State<App> createState() => _AppState();
}

// _AppState — App 的内部状态类

class _AppState extends State<App> {
  // 当前选中的页面 ID（默认 0 = AllMusic）
  int? _currentPageId = 0;

  // ════════════════════════════════════════════════════════════════
  // 页面 ID → Widget 映射
  //
  // 对应原 Vue pageIdToComponent:
  //   0 → AllMusic    1 → MyLike       2 → RecentPlay
  //   3 → Artist      4 → Album       5 → Playlist
  //   6 → Genre       7 → AIAssistant  8 → Effects
  //   9 → Equalizer   10 → SpeakerArray 11 → Settings
  // ════════════════════════════════════════════════════════════════

  // 根据 pageId 返回对应的页面 Widget
  Widget? _resolvePage(int? id) {
    if (id == null) return null;

    // Dart 3 switch 表达式：每个 case 用 `=>` 返回一个值
    return switch (id) {
      0 => const AllMusicPage(), // 所有音乐
      // 其他页面尚未开发，返回 null → 显示占位符
      _ => null,
    };
  }

  // 处理左侧导航栏选中事件
  void _handlePageChange(int id) {
    setState(() {
      _currentPageId = id;
    });
  }

  @override
  Widget build(BuildContext context) {
    // 计算要显示的页面 Widget
    final pageWidget = _resolvePage(_currentPageId);

    return Scaffold(
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
                SizedBox(
                  child: LeftColumn(onSelectionChanged: _handlePageChange),
                ),

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
                            padding: const EdgeInsets.symmetric(horizontal: 10),
                            child: pageWidget ?? const SizedBox(),
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
    );
  }
}
