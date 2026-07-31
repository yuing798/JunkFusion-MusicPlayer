import 'package:flutter/foundation.dart';
import 'package:freezed_annotation/freezed_annotation.dart';
import 'package:json_annotation/json_annotation.dart';

// part 指令是 Dart 中“将一个库（Library）的源代码拆分成多个物理文件”的机制。
// 用一句话说透：part 告诉 Dart 编译器：“把 song_info.freezed.dart 这个文件里的代码，
//原封不动地粘贴到 song_info.dart 文件的内部，作为它的一部分来编译。”
// 它和 C++ 的 #include 极为相似，但目的完全不同——#include 是为了复用代码，
//而 part 是为了解决代码生成器（如 freezed）必须访问文件私有成员（_ 开头的成员）的问题。
// ════════════════════════════════════════════════════════════════
// song_info.dart — SongInfo 数据模型
//
//
// Dart 语法说明：
// - `String?` 类型后面的 `?` 表示可空（nullable），可以为 null
// - `super` 调用父类（这里没有显式父类，等价于继承自 Object）
// - `factory` 构造函数不一定要创建新实例，可以返回缓存/子类型实例
// - `fromJson` 是约定俗成的工厂构造函数名，用于从 JSON/Map 反序列化
// - `Map<String, dynamic>` 是键为 String、值为任意类型的映射
//   （dynamic 表示"禁用类型检查"，相当于 TypeScript 的 any）
//
// Immutable（不可变）模式：
// 这个类所有字段都是 final，创建后不可修改。要"修改"时，
// 需要用 copyWith() 创建一个新对象。这种模式叫"不可变数据"，
// Flutter 中广泛使用，利于性能优化和状态追踪。
// ════════════════════════════════════════════════════════════════

// SongInfo — 单首歌曲的完整元数据
class SongInfo {
  // 数据库主键，自增 ID（C++ 端为 int64_t）
  final int songId;
  //final必须使用构造函数传参

  // 歌曲时长（秒）
  final int duration;

  // 歌曲标题
  final String title;

  // 艺术家名称（可空，对应 C++ std::optional）
  final String? artist;

  // 专辑名称（可空）
  final String? album;

  // 专辑艺术家（可空）
  final String? albumArtist;

  // 体裁（可空）
  final String? genre;

  // 轨道号（可空）
  final int? trackNumber;

  // 碟片号（可空）
  final int? discNumber;

  // 发行年份（可空）
  final int? year;

  // 作曲者（可空）
  final String? composer;

  // 比特率（kbps，C++ 端为 int64_t）
  final int bitRate;

  // 采样率（Hz）
  final int sampleRate;

  // 通道布局描述字符串
  final String channelLayout;

  // 位深
  final int bitDepth;

  // 编码器名称（可空）
  final String? codecName;

  // AI 分析体裁（可空）
  final String? aiGenre;

  // 节拍数 BPM（可空）
  final int? bpm;

  // 调性，如 "C major", "A minor"（可空）
  final String? key;

  // 是否已添加到"我喜欢"列表
  final bool isMyLike;

  // 用户备注（可空）
  final String? comment;

  // 已播放次数
  final int playNum;

  SongInfo({
    required this.songId,
    required this.duration,
    required this.title,
    this.artist,
    this.album,
    this.albumArtist,
    this.genre,
    this.trackNumber,
    this.discNumber,
    this.year,
    this.composer,
    required this.bitRate,
    required this.sampleRate,
    required this.channelLayout,
    required this.bitDepth,
    this.codecName,
    this.aiGenre,
    this.bpm,
    this.key,
    required this.isMyLike,
    this.comment,
    required this.playNum,
  });
}

// ════════════════════════════════════════════════════════════════
// song_provider.dart — 歌曲列表状态管理
//
// 对应原 Vue 项目 store/songStore.ts (Pinia store: 'allSongs')
//
// Dart/Flutter 关键概念：
// - `ChangeNotifier` 是 Flutter 基础库提供的可监听对象
// - `notifyListeners()` 触发所有依赖此 provider 的 widget 重建
// - `async` / `await` 用于处理异步操作（网络请求、原生调用等）
// - `Future<T>` 表示一个将在未来完成的异步操作，T 是返回值类型
// ════════════════════════════════════════════════════════════════
class SongProvider extends ChangeNotifier {
  List<SongInfo> _songs = [];

  // 对外暴露的只读 getter
  //
  // Dart 的 getter 语法：`返回类型 get 名称 => 表达式`
  // 调用时像属性一样：`provider.songs`，不是 `provider.songs()`
  //外界可以通过 对象.songs 来获取 _songs 的值，但不能直接对这个属性赋值（因为没有配套的 set）
  List<SongInfo> get songs => _songs;

  // 按 ID 查找歌曲
  //
  // 对应原 Vue: getSongInfo(songId: number): SongInfo | undefined
  //
  // Dart 语法说明：
  // - `SongInfo?` 返回类型（注意 `?`），表示可能返回 null
  // - `_songs.where(...)` 过滤列表，返回 lazy iterable
  // - `.firstOrNull` 是 Dart 3.x 新增的便捷方法，
  //   返回第一个匹配项，无匹配返回 null
  // - `(s) => s.songId == songId` 是 lambda/匿名函数
  // 按 ID 查找歌曲
  SongInfo? getSongInfo(int songId) {
    // `where()` 返回符合条件的所有元素（惰性求值）
    // `(s) => s.songId == songId` 是匿名函数（arrow function）：
    //   s 是参数（列表中每个元素）
    //   => 后面是返回值（bool 表达式）
    // `firstOrNull` 取第一个结果，如果没有结果就返回 null
    return _songs.where((s) => s.songId == songId).firstOrNull;
  }

  // 获取所有歌曲（从后端数据库）
  //
  // 对应原 Vue: async getAllSongs() — 通过 JUCE 桥接调用 C++ 原生函数
  //
  // Dart 语法说明：
  // - `Future<void>` 异步方法，返回一个 Future（承诺未来完成），没有实际返回值
  // - `async` 标记该方法包含异步操作
  // - `await` 等待一个 Future 完成并取结果
  Future<void> getAllSongs() async {
    // TODO: 桥接层 - 调用原生函数获取所有歌曲
    // 原 Vue 代码:
    //   const result = await getNativeFunction(B_getAllSongs.name)();
    //   if (Array.isArray(result)) {
    //     this.songs = result as SongInfo[];
    //     return;
    //   } else if (typeof result === 'string' && result === B_getAllSongs.nothing) {
    //     return; // 空数据库
    //   }
    //
    // Flutter 中未来桥接层的实现大致是:
    //   final result = await NativeBridge.call('getAllSongs');
    //   if (result is List) {
    //     _songs = result.map((e) => SongInfo(...)).toList();
    //   }
    //   notifyListeners();

    // 当前为空实现，songs 保持初始空列表
  }

  // 切换"我喜欢"状态（乐观更新 + 回滚）
  //
  // 对应原 Vue: async toggleMyLike(songId: number)
  //
  // 乐观更新流程：
  // 1. 立即翻转 UI 中的 isMyLike 状态（用户立刻看到反馈）
  // 2. 异步调用后端接口
  // 3. 如果后端返回错误，恢复原始状态（回滚）
  //
  // Dart 语法说明：
  // - `Future<void>` 异步方法
  // - `try { ... } catch (e) { ... }` 异常处理，等价于 JS 的 try/catch
  // - `where(...)` 返回 Iterable，`.firstOrNull` 配合使用高效
  Future<void> toggleMyLike(int songId) async {
    // 找到目标歌曲在列表中的索引
    final index = _songs.indexWhere((s) => s.songId == songId);
    if (index == -1) return; // 没找到，直接返回

    // 保存原始状态，用于失败时回滚
    final originSong = _songs[index];

    // --- 乐观更新：立即翻转 UI 状态 ---
    _songs[index] = originSong.copyWith(isMyLike: !originSong.isMyLike);
    notifyListeners(); // 通知 UI 刷新（UI 立刻看到变化）

    // --- 异步调用后端 ---
    try {
      // TODO: 桥接层 - 调用原生函数切换喜欢状态
      // 原 Vue 代码:
      //   await getNativeFunction(B_toggleMyLike.name)(songId);
      //
      // Flutter 中未来桥接层的实现大致是:
      //   await NativeBridge.call('toggleMyLike', {'songId': songId});

      // 如果到这里还没抛出异常，说明后端操作成功
    } catch (e) {
      // --- 失败回滚：恢复原始状态 ---
      _songs[index] = originSong;

      // TODO: 桥接层 - 显示错误提示
      // 原 Vue 代码: showInfoWindow(error)
      // Flutter 中应该是: InfoWindow.show(e);

      notifyListeners(); // 通知 UI 刷新回原始状态
    }
  }

  // 直接设置歌曲列表（用于初始化或外部数据注入）
  //
  // Dart 语法：
  // - `void setSongs(...)` 设置私有字段并通知监听者
  void setSongs(List<SongInfo> newSongs) {
    _songs = newSongs;
    notifyListeners();
  }

  // 追加歌曲到列表末尾（用于导入等操作）
  //
  // Dart 语法：
  // - `addAll()` 把一个列表的所有元素追加到另一个列表末尾
  // - `...`（展开运算符）在 Dart 中不在列表字面量中使用，
  //   而是用 `addAll` 方法实现同样功能
  void addSongs(List<SongInfo> newSongs) {
    _songs.addAll(newSongs);
    notifyListeners();
  }

  // 将秒数格式化为 分:秒 或 时:分:秒 的可读字符串
  //可选：是否显示毫秒数
  static String formatDuration(double seconds, [bool needMs = false]) {
    if (seconds < 0) return '0:00';

    // 1. 分离整数部分（秒）和小数部分（毫秒）
    final totalSecs = seconds.floor(); // 向下取整，得到整秒
    final milliseconds = ((seconds - totalSecs) * 1000).round();

    // 2. 计算 时/分/秒
    final hours = totalSecs ~/ 3600;
    final minutes = (totalSecs % 3600) ~/ 60;
    final secs = totalSecs % 60;

    // 3. 拼接主要字符串
    String result;
    if (hours > 0) {
      result =
          '$hours:${minutes.toString().padLeft(2, '0')}:${secs.toString().padLeft(2, '0')}';
    } else {
      result = '$minutes:${secs.toString().padLeft(2, '0')}';
    }

    // 4. 追加毫秒（如果需要）
    if (needMs) {
      result = '$result.${milliseconds.toString().padLeft(3, '0')}';
    }
    return result;
  }
}

// ════════════════════════════════════════════════════════════════
// Provider 使用模式总结：
//
// ## 注入 Provider 到 Widget Tree（在 main.dart 中）：
// ```dart
// MultiProvider(
//   providers: [
//     ChangeNotifierProvider(create: (_) => SongProvider()),
//     ChangeNotifierProvider(create: (_) => PlaybackProvider()),
//   ],
//   child: const MyApp(),
// )
// ```
//
// ## 在 Widget 中使用：
//
// ### 方式 1: context.watch (推荐，自动监听)
// ```dart
// @override
// Widget build(BuildContext context) {
//   final songs = context.watch<SongProvider>().songs;
//   return ListView(...);
// }
// ```
//
// ### 方式 2: context.read (不监听，只调用方法)
// ```dart
// ElevatedButton(
//   onPressed: () {
//     context.read<SongProvider>().getAllSongs();
//   },
//   child: Text('刷新'),
// )
// ```
//
// ### 方式 3: Consumer widget (局部重建)
// ```dart
// Consumer<SongProvider>(
//   builder: (context, provider, child) {
//     return Text('共 ${provider.songs.length} 首');
//   },
// )
// ```
//
// ## 对比 Vue Pinia:
//
// | 操作        | Vue Pinia                      | Flutter Provider                       |
// |------------|-------------------------------|----------------------------------------|
// | 读取状态    | `store.songs`                  | `context.watch<SongProvider>().songs`  |
// | 修改状态    | `store.songs = [...]`         | provider.setSongs([...]) + notifyListeners |
// | 异步操作    | `async` actions               | `Future<void>` methods + async/await   |
// | 乐观更新    | 直接改 state + try/catch回滚   | 直接改 _songs + try/catch回滚          |
// ════════════════════════════════════════════════════════════════
