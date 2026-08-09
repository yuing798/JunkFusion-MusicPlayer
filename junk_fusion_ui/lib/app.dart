// ════════════════════════════════════════════════════════════════
// app.dart — 应用根组件
//
// 布局结构（对应 Vue App.vue 的 template）：
// ```
// Column
// ├── Expanded
// │   └── Row
// │       ├── LeftColumn (固定 220px)
// │       └── Expanded (主内容区 — 动态页面)
// └── AnimatedSlide + AnimatedOpacity (底部播放栏)
// ```
// ════════════════════════════════════════════════════════════════

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/widgets/title_bar.dart';
import 'package:provider/provider.dart';
import 'providers/song_provider.dart';
import 'providers/playback_provider.dart';
import 'theme/app_theme.dart';
import 'widgets/left_column.dart';
import 'widgets/play_bar.dart';
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
      //setState：局部刷新。只会调用当前 State 对象的 build 方法，只重绘这一个 Widget 及其子树。其他页面、其他组件完全不受影响。
      // notifyListeners()：全局广播。所有通过 Provider.of<T>(context) 或 Consumer<T> 监听该 ChangeNotifier 的 Widget，全部会收到通知并重绘。
    });
  }

  @override
  Widget build(BuildContext context) {
    // 计算要显示的页面 Widget
    final pageWidget = _resolvePage(_currentPageId);

    // 只有当表达式的值（即 bool 结果）发生改变时才会重建
    final hasCurrentSong = context.select<PlaybackProvider, bool>(
      (provider) => provider.currentSongId != null,
    );

    return Column(
      children: [
        // --- 主内容区域（占满剩余空间） ---
        Expanded(
          child: Row(
            children: [
              SizedBox(
                child: LeftColumn(onSelectionChanged: _handlePageChange),
              ),

              // 右侧主内容区
              // Expanded 让主内容区占据剩余的所有宽度
              Expanded(
                child: Column(
                  children: [
                    const TitleBar(),
                    pageWidget ?? Expanded(child: const SizedBox()),
                  ],
                ),
              ),
            ],
          ),
        ),

        // --- 底部播放栏（带动画） ---
        // AnimatedSlide + AnimatedOpacity 组合实现滑动淡入淡出
        if (hasCurrentSong)
          const PlayBar()
        else
          const SizedBox.shrink(), // 不占用任何空间
      ],
    );
  }
}
