/// ════════════════════════════════════════════════════════════════
/// app.dart — 应用根组件
///
/// 对应原 Vue 项目 App.vue
///
/// 职责：
/// - 整体布局：左侧导航栏 + 右侧主内容区 + 底部播放栏
/// - 页面切换逻辑（pageId → Widget 映射）
/// - 播放栏显示/隐藏动画
/// - 应用初始化（加载歌曲列表、恢复播放状态）
///
/// Dart/Flutter 关键概念：
/// - `StatefulWidget`：有内部状态的 Widget，通过 `setState()` 重建
/// - `initState()`：State 生命周期方法，在 Widget 创建后仅执行一次
///   对应 Vue 的 `onMounted()` / React 的 `useEffect([], [])`
/// - `context.watch<T>()`：监听 Provider，Provider 变化时自动重建
/// - `AnimatedSlide`：Flutter 内置的滑动动画 Widget
///
/// 布局结构（对应 Vue App.vue 的 template）：
/// ```
/// Column
/// ├── Expanded
/// │   └── Row
/// │       ├── LeftColumn (固定 220px)
/// │       └── Expanded (主内容区 — 动态页面)
/// └── AnimatedSlide + AnimatedOpacity (底部播放栏)
/// ```
/// ════════════════════════════════════════════════════════════════

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'providers/song_provider.dart';
import 'providers/playback_provider.dart';
import 'theme/app_theme.dart';
import 'widgets/left_column.dart';
import 'widgets/play_bar.dart';
import 'widgets/info_window.dart';
import 'pages/all_music_page.dart';

/// App — 根组件（StatefulWidget）
///
/// 为什么用 StatefulWidget 而不是 StatelessWidget？
/// 因为需要：
/// 1. `initState()` — 应用初始化（加载数据）
/// 2. `setState()` — 页面切换时更新 UI
/// 3. 维护 `_currentPageId` 等可变状态
class App extends StatefulWidget {
  const App({super.key});

  /// 创建 State 对象
  ///
  /// Dart 语法：
  /// - `createState()` 是 StatefulWidget 的 abstract 方法，必须覆写
  /// - `=>` 箭头函数语法，等价于 `{ return _AppState(); }`
  /// - Flutter 框架在 Widget 挂载时调用此方法创建 State 对象
  @override
  State<App> createState() => _AppState();
}

/// _AppState — App 的内部状态类
///
/// Dart 语法说明：
/// - `_` 前缀：库私有（library-private），外部文件无法访问
/// - `class _AppState extends State<App>`：泛型参数 `<App>` 表示
///   这个 State 绑定的 Widget 类型
/// - `State<App>` 是 Flutter 的泛型类：
///   State<T> 中的 T 指代对应的 StatefulWidget 子类
class _AppState extends State<App> {
  // ════════════════════════════════════════════════════════════════
  // 页面切换相关字段
  // ════════════════════════════════════════════════════════════════

  /// 当前选中的页面 ID（默认 0 = AllMusic）
  ///
  /// 对应原 Vue: const currentPageId = ref<number | null>(0)
  int? _currentPageId = 0;

  // ════════════════════════════════════════════════════════════════
  // 页面 ID → Widget 映射
  //
  /// 对应原 Vue pageIdToComponent:
  ///   0 → AllMusic    1 → MyLike       2 → RecentPlay
  ///   3 → Artist      4 → Album       5 → Playlist
  ///   6 → Genre       7 → AIAssistant  8 → Effects
  ///   9 → Equalizer   10 → SpeakerArray 11 → Settings
  /// ════════════════════════════════════════════════════════════════

  /// 根据 pageId 返回对应的页面 Widget
  ///
  /// 对应原 Vue: function resolvedComponent(id)
  ///
  /// Dart 语法：
  /// - `Widget?` 返回类型表示可能返回 null
  /// - `switch` 表达式（Dart 3.x 特性）：有返回值的 switch
  /// - `_` 在 switch 中表示 default（默认分支）
  Widget? _resolvePage(int? id) {
    if (id == null) return null;

    // Dart 3 switch 表达式：每个 case 用 `=>` 返回一个值
    return switch (id) {
      0 => const AllMusicPage(), // 所有音乐
      // 其他页面尚未开发，返回 null → 显示占位符
      _ => null,
    };
  }

  // ════════════════════════════════════════════════════════════════
  // 事件处理
  // ════════════════════════════════════════════════════════════════

  /// 处理左侧导航栏选中事件
  ///
  /// 对应原 Vue: function handlePageChange(id)
  void _handlePageChange(int id) {
    // `setState()` 是 StatefulWidget 的核心方法
    // 调用它会触发 build() 方法重新执行 → UI 更新
    // 等价于 Vue 中修改 ref 值触发模板重渲染
    setState(() {
      _currentPageId = id;
      //setState：局部刷新。只会调用当前 State 对象的 build 方法，只重绘这一个 Widget 及其子树。其他页面、其他组件完全不受影响。
      // notifyListeners()：全局广播。所有通过 Provider.of<T>(context) 或 Consumer<T> 监听该 ChangeNotifier 的 Widget，全部会收到通知并重绘。
    });
  }

  // ════════════════════════════════════════════════════════════════
  // 生命周期
  // ════════════════════════════════════════════════════════════════

  /// initState — Widget 初始化时执行一次
  ///
  /// 对应 Vue 的 onMounted() 钩子
  ///
  /// Dart 语法：
  /// - `@override` 覆写父类方法
  /// - `super.initState()` 必须先调用父类的 initState（框架要求）
  @override
  void initState() {
    super.initState();

    // 在下一帧加载数据（等 Provider 完成初始化后）
    // `WidgetsBinding.instance.addPostFrameCallback` 是 Flutter 的回调机制，
    // 它在当前帧渲染完成后执行回调
    // 这是因为在 initState 中 context 还不能安全访问 Provider
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _loadInitialData();
    });
  }

  /// 加载初始数据：歌曲列表 + 恢复播放状态
  ///
  /// 对应原 Vue onMounted 中的:
  ///   songStore.getAllSongs();
  ///
  /// Dart 语法：
  /// - `void _loadInitialData()` 私有方法
  /// - `context.read<T>()` 获取 Provider 但不监听变化
  ///   适合在事件处理/初始化中调用方法
  ///   注意：read 不会触发 widget 重建（和 watch 的区别）
  void _loadInitialData() {
    // 加载所有歌曲
    context.read<SongProvider>().getAllSongs();

    // 恢复上次播放的歌曲
    context.read<PlaybackProvider>().restoreCurrentSongId();
  }

  // ════════════════════════════════════════════════════════════════
  // build — 渲染 UI
  // ════════════════════════════════════════════════════════════════

  @override
  Widget build(BuildContext context) {
    // `context.watch<T>()` 监听 Provider，值变化时自动重建本 widget
    // 等价于 Vue 中在模板里使用 store 的响应式属性
    final playbackProvider = context.watch<PlaybackProvider>();
    final hasCurrentSong = playbackProvider.currentSongId != null;

    // 计算要显示的页面 Widget
    final pageWidget = _resolvePage(_currentPageId);

    // Scaffold — Material Design 布局脚手架
    // 提供 appBar, body, bottomNavigationBar 等标准布局区域
    return Scaffold(
      // `body` 是 Scaffold 的主要内容区域
      // `SafeArea` 避免内容被系统状态栏/底部指示条遮挡
      body: Column(
        children: [
          // --- 主内容区域（占满剩余空间） ---
          Expanded(
            // `Expanded` 让子 widget 在主轴上填充剩余空间
            // 等价于 Vue CSS 的 flex: 1
            child: Row(
              children: [
                // 左侧导航栏（固定 220px 宽）
                // AppTheme.leftColumnWidth = 220.0
                SizedBox(
                  width: AppTheme.leftColumnWidth,
                  child: LeftColumn(
                    // `onSelectionChanged` 是 callback 参数：
                    // 父组件传入一个函数，子组件在选中变化时调用它
                    onSelectionChanged: _handlePageChange,
                  ),
                ),

                // 右侧主内容区
                // Expanded 让主内容区占据剩余的所有宽度
                Expanded(
                  child: Container(
                    // `color` 设置背景色（对应 CSS background-color）
                    color: AppTheme.colorMain,
                    padding: const EdgeInsets.symmetric(
                      horizontal: 20,
                      vertical: 15,
                    ),
                    // `EdgeInsets.symmetric` 对称边距：
                    // horizontal = 左右，vertical = 上下

                    // 条件渲染：
                    // 如果 pageWidget 不为 null，显示页面
                    // 否则显示占位符
                    child: pageWidget != null
                        ? pageWidget
                        : const _PlaceholderContent(),
                  ),
                ),
              ],
            ),
          ),

          // --- 底部播放栏（带动画） ---
          // 对应 Vue: <Transition name="slide-up">
          //          <playBar v-if="playBackStore.currentSongId !== null">
          //
          // AnimatedSlide + AnimatedOpacity 组合实现滑动淡入淡出
          if (hasCurrentSong)
            const PlayBar()
          else
            const SizedBox.shrink(), // 不占用任何空间
        ],
      ),

      // InfoWindow 全局覆盖层（悬浮在所有内容上方）
      // 使用 Stack + Positioned 实现类似 Vue Teleport to body 的效果
      // 这里在 Scaffold 的 body 层级不适用 overlay，
      // 实际 overlay 逻辑在 InfoWindow 内部用 Overlay widget 实现
    );
  }
}

/// _PlaceholderContent — 页面占位符（未开发页面的fallback）
///
/// 对应原 Vue:
///   <div v-else class="placeholder">
///     <p>主内容区域</p>
///     <p class="hint">（选择左侧导航以查看页面）</p>
///   </div>
///
/// Dart 语法：
/// - `class _PlaceholderContent extends StatelessWidget` 私有无状态组件
/// - `Center` widget 将子元素水平和垂直居中
/// - `Column` 垂直排列子元素
/// - `Text` 显示文本
/// - `TextStyle` 定义文本样式
class _PlaceholderContent extends StatelessWidget {
  const _PlaceholderContent();

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Column(
        // `mainAxisAlignment` 控制主轴（垂直）对齐方式
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Text('主内容区域', style: AppTheme.midTextStyle),
          const SizedBox(height: 8), // 间距
          Text('（选择左侧导航以查看页面）', style: AppTheme.littleTextStyle),
        ],
      ),
    );
  }
}

/// ════════════════════════════════════════════════════════════════
/// 关键概念对比 Vue → Flutter:
///
/// | Vue 概念            | Flutter 等价                          |
/// |---------------------|---------------------------------------|
/// | ref()               | 可变字段 + setState()                 |
/// | watch()             | context.watch<T>()                    |
/// | onMounted()         | initState()                           |
/// | v-if                | if (condition) widget                  |
/// | v-else              | else widget                           |
/// | :class="{...}"       | 三元表达式 / 条件判断                 |
/// | <Transition>        | AnimatedSlide / AnimatedOpacity       |
/// | <component :is="">  | 动态 widget 变量                      |
/// | <Teleport to="body">| Overlay widget                        |
/// | props               | 构造函数参数                           |
/// | emit                | callback 参数 (VoidCallback / Function)|
/// ════════════════════════════════════════════════════════════════
