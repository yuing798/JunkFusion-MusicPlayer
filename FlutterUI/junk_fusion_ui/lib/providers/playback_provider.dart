/// ════════════════════════════════════════════════════════════════
/// playback_provider.dart — 播放状态管理
///
/// 对应原 Vue 项目 store/playBackStore.ts (Pinia store)
///
/// Dart/Flutter 状态管理核心概念：
///
/// `ChangeNotifier` 是 Flutter 提供的最基础的状态管理类，来自
/// `package:flutter/foundation.dart`。它的工作原理：
/// 1. 持有可变状态（字段）
/// 2. 当状态改变时调用 `notifyListeners()` 通知监听者
/// 3. 监听者（通常是 Consumer widget / context.watch）自动重建 UI
///
/// 这等价于 Vue 的响应式系统：
///   - Vue:    `ref()` + `watch()` + 模板自动追踪
///   - Flutter: `ChangeNotifier` + `notifyListeners()` + Consumer widget
///
/// `provider` 包（第三方）让 ChangeNotifier 能通过 widget tree 传递，
/// 它的作用类似 Vue 的 `provide/inject` + Pinia store 的组合。
///
/// Dart 语法说明：
/// - `extends ChangeNotifier` 表示 PlaybackProvider 继承自 ChangeNotifier
/// - `super` 关键字引用父类
/// - `void` 表示方法没有返回值
/// - `int?` 中的 `?` 表示该变量可以为 null
/// - `late` 关键字：延迟初始化，变量在使用前必须被赋值
///   用于非 final 的非空变量
/// ════════════════════════════════════════════════════════════════

// `import` 语句：引入所需库
// `package:flutter/foundation.dart` 包含 ChangeNotifier 等基础类
import 'package:flutter/foundation.dart';

/// PlaybackProvider — 播放状态监听器
///
/// 职责：
/// - 管理当前播放歌曲 ID、播放/暂停状态、播放进度
/// - 管理播放模式（顺序/列表循环/单曲循环/随机）
/// - 持久化 currentSongId（原 Vue 用 localStorage）
///
/// 使用方式（在 widget 中）：
/// ```dart
/// final playback = context.watch<PlaybackProvider>();
/// print(playback.currentSongId);
/// ```
///
/// Dart 语法说明：
/// - `class A extends B` 表示 A 继承 B（单继承）
/// - `@override` 注解表示覆写父类方法（非必须但推荐加上）
class PlaybackProvider extends ChangeNotifier {
  // ════════════════════════════════════════════════════════════════
  // 状态字段
  //
  // Dart 中字段默认是 public 的，没有 public/private 关键字
  // 下划线 `_` 开头表示库私有（仅当前 .dart 文件内可见）
  // ════════════════════════════════════════════════════════════════

  /// 当前正在播放的歌曲 ID，null 表示未播放任何歌曲
  ///
  /// 对应原 Vue: currentSongId: null as number | null
  int? _currentSongId = null;

  /// 对外的只读 getter（这不创建新变量，只是提供读取访问）
  ///
  /// 语法说明：
  /// - `int? get currentSongId` 是 getter（获取器），调用时像字段一样使用
  /// - flutter 的 convention：私有字段 _xxx 配公开 getter xxx
  int? get currentSongId => _currentSongId;

  /// 是否正在播放（true = 播放中，false = 暂停）
  bool _isPlaying = false;
  bool get isPlaying => _isPlaying;

  /// 当前播放进度时间戳（null 表示无进度信息）
  int? _currentTimeStamp = null;
  int? get currentTimeStamp => _currentTimeStamp;

  /// 播放模式枚举
  /// 0 = 顺序播放，1 = 列表循环，2 = 单曲循环，3 = 随机播放
  ///
  /// 原 Vue 使用 number 0-3，这里用 int 保持一致
  int _playMode = 0;
  int get playMode => _playMode;

  /// 当前播放值（用于进度条等）
  int _currentValue = 0;
  int get currentValue => _currentValue;

  // ════════════════════════════════════════════════════════════════
  // 操作方法（对应原 Vue Pinia actions）
  // ════════════════════════════════════════════════════════════════

  /// 设置当前播放的歌曲 ID，并持久化存储
  ///
  /// 对应原 Vue: setCurrentSongId(songId: number)
  ///
  /// Dart 语法：
  /// - `void` 返回类型表示不返回任何值
  /// - `notifyListeners()` 通知所有监听者状态已变更 → UI 自动重建
  ///   这是整个 ChangeNotifier 模式的核心！
  void setCurrentSongId(int songId) {
    _currentSongId = songId;

    // TODO: 桥接层 - 持久化到本地存储
    // 原 Vue 代码: localStorage.setItem("playbar_currentSongId", String(songId));
    // Flutter 中可用 shared_preferences 包实现

    notifyListeners(); // ← 通知 UI 刷新
  }

  /// 从持久化存储恢复当前播放歌曲 ID
  ///
  /// 对应原 Vue: restoreCurrentSongId()
  ///
  /// Dart 语法：
  /// - 这个方法目前用 TODO 占位，因为桥接层尚未实现
  void restoreCurrentSongId() {
    // TODO: 桥接层 - 从本地存储恢复播放状态
    // 原 Vue 代码:
    //   const songId = localStorage.getItem("playbar_currentSongId");
    //   if (songId) this.currentSongId = Number(songId);
    //
    // Flutter 实现思路（用 shared_preferences）:
    //   final prefs = await SharedPreferences.getInstance();
    //   final songId = prefs.getInt('playbar_currentSongId');
    //   if (songId != null) _currentSongId = songId;
    //   notifyListeners();

    // 当前为空实现，不做任何恢复操作
  }

  /// 切换播放/暂停状态
  ///
  /// 对应原 Vue playPauseChange()
  void togglePlayPause() {
    _isPlaying = !_isPlaying; // `!` 是逻辑非运算符
    notifyListeners();
  }

  /// 切换播放模式（循环 0 → 1 → 2 → 3 → 0 → ...）
  ///
  /// 对应原 Vue playmodeChange():
  ///   playBackStore.playMode++;
  ///   if (playBackStore.playMode === 4) playBackStore.playMode -= 4;
  void cyclePlayMode() {
    _playMode = (_playMode + 1) % 4; // `%` 是取模运算符，确保值始终在 0-3 之间
    notifyListeners();
  }

  /// 获取播放模式的文本描述
  ///
  /// 对应原 Vue 中不同 mode 显示不同图标的逻辑
  String get playModeLabel {
    // Dart 的 switch 表达式（switch expression）：
    // 和 switch 语句不同，表达式有返回值，可直接赋值
    return switch (_playMode) {
      0 => '顺序播放',
      1 => '列表循环',
      2 => '单曲循环',
      3 => '随机播放',
      _ => '未知', // `_` 在 switch 中代表 default 分支
    };
  }

  /// 重置所有播放状态
  void reset() {
    _currentSongId = null;
    _isPlaying = false;
    _currentTimeStamp = null;
    _playMode = 0;
    _currentValue = 0;
    notifyListeners();
  }
}

/// ════════════════════════════════════════════════════════════════
/// ChangeNotifier + Provider 核心模式说明：
///
/// 1. 创建 Provider:
///    ```dart
///    ChangeNotifierProvider(
///      create: (_) => PlaybackProvider(),
///      child: MyApp(),
///    )
///    ```
///
/// 2. 在 Widget 中读取状态（3 种方式）：
///    a) `context.watch<PlaybackProvider>()` — 监听变化，自动重建
///    b) `context.read<PlaybackProvider>()` — 仅读取一次，不监听
///    c) `Consumer<PlaybackProvider>(builder: (_, p, __) => Text('${p.currentSongId}'))`
///
/// 3. 修改状态：
///    ```dart
///    context.read<PlaybackProvider>().setCurrentSongId(123);
///    ```
///
/// 对比 Vue Pinia:
///   Vue:    const store = usePlayBackStore(); store.setCurrentSongId(123)
///   Flutter: context.read<PlaybackProvider>().setCurrentSongId(123)
/// ════════════════════════════════════════════════════════════════
