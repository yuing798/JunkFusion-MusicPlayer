/// ════════════════════════════════════════════════════════════════
/// song_info.dart — SongInfo 数据模型
///
/// 对应原 Vue 项目 store/songStore.ts 中的 SongInfo interface
///
/// Dart 语法说明：
/// - `class` 定义一个类，Dart 中没有 `interface` 关键字，
///   任何类都可以被其他类 implements（实现）或 extends（继承）
/// - `final` 字段：初始化后不可再修改（immutable）。Dart 的 final
///   类似于 TypeScript 的 readonly 或 Java 的 final
/// - `String?` 类型后面的 `?` 表示可空（nullable），可以为 null
/// - `int` 是整数类型（Dart 没有 int64 概念，int 是任意精度整数）
/// - `double` 是 64 位浮点数
/// - 构造函数参数用花括号 `{}` 包裹 → 命名参数（可选的、无序的）
/// - `required` 标记的命名参数是必填的
/// - `super` 调用父类（这里没有显式父类，等价于继承自 Object）
/// - `factory` 构造函数不一定要创建新实例，可以返回缓存/子类型实例
/// - `fromJson` 是约定俗成的工厂构造函数名，用于从 JSON/Map 反序列化
/// - `Map<String, dynamic>` 是键为 String、值为任意类型的映射
///   （dynamic 表示"禁用类型检查"，相当于 TypeScript 的 any）
///
/// Immutable（不可变）模式：
/// 这个类所有字段都是 final，创建后不可修改。要"修改"时，
/// 需要用 copyWith() 创建一个新对象。这种模式叫"不可变数据"，
/// Flutter 中广泛使用，利于性能优化和状态追踪。
/// ════════════════════════════════════════════════════════════════

/// SongInfo — 单首歌曲的完整元数据
///
/// Dart 的数据类写法：所有字段用 final，提供 copyWith 方法
/// 注意：Dart 没有 TypeScript 的 `interface`，直接用 class 即可
class SongInfo {
  // ════════════════════════════════════════════════════════════════
  // 字段声明
  //
  // 语法：`final 类型 字段名;`
  // 所有字段用 `final` 表示创建后不可变
  // `String?` 表示可为 null 的字符串
  // ════════════════════════════════════════════════════════════════

  /// 数据库主键，自增 ID（C++ 端为 int64_t）
  final int songId;

  /// 歌曲时长（秒）
  final int duration;

  /// 歌曲标题
  final String title;

  /// 艺术家名称（可空，对应 C++ std::optional）
  final String? artist;

  /// 专辑名称（可空）
  final String? album;

  /// 专辑艺术家（可空）
  final String? albumArtist;

  /// 体裁（可空）
  final String? genre;

  /// 轨道号（可空）
  final int? trackNumber;

  /// 碟片号（可空）
  final int? discNumber;

  /// 发行年份（可空）
  final int? year;

  /// 作曲者（可空）
  final String? composer;

  /// 比特率（kbps，C++ 端为 int64_t）
  final int bitRate;

  /// 采样率（Hz）
  final int sampleRate;

  /// 通道布局描述字符串
  final String channelLayout;

  /// 位深
  final int bitDepth;

  /// 编码器名称（可空）
  final String? codecName;

  /// AI 分析体裁（可空）
  final String? aiGenre;

  /// 节拍数 BPM（可空）
  final int? bpm;

  /// 调性，如 "C major", "A minor"（可空）
  final String? key;

  /// 是否已添加到"我喜欢"列表
  final bool isMyLike;

  /// 用户备注（可空）
  final String? comment;

  /// 已播放次数
  final int playNum;

  // ════════════════════════════════════════════════════════════════
  // 构造函数
  //
  // Dart 的构造函数语法：
  // - `const SongInfo({...})` 声明 const 构造函数，
  //   允许在编译期创建不可变实例
  // - `{ }` 包裹的是命名参数（named parameters）
  // - `required` 标记参数为必填，否则是可选的
  // - `this.fieldName` 是 Dart 的语法糖（syntactic sugar），
  //   直接把构造函数参数赋值给同名字段，无需写 `this.fieldName = fieldName`
  // ════════════════════════════════════════════════════════════════

  const SongInfo({
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

  // ════════════════════════════════════════════════════════════════
  // copyWith — 创建副本并选择性覆盖某些字段
  //
  // 对应 TypeScript 中的展开运算符: { ...originSong, isMyLike: true }
  //
  // Dart 语法说明：
  // - `SongInfo copyWith({...})` 返回一个新的 SongInfo 对象
  // - 参数中的 `类型? 参数名` 全部是可选的（没有 required）
  // - `??` 是 Dart 的空值合并运算符：
  //   `a ?? b` 意思是"如果 a 不为 null，用 a；否则用 b"
  //   等价于 TypeScript 的 `a ?? b`
  // ════════════════════════════════════════════════════════════════

  SongInfo copyWith({
    int? songId,
    int? duration,
    String? title,
    // `Object?` 是一种技巧：当你想让调用者能传入 null 来"清空"某个可空字段时，
    // 用 Object? 代替对应的类型?，然后手动判断
    Object? artist = _sentinel,
    Object? album = _sentinel,
    Object? albumArtist = _sentinel,
    Object? genre = _sentinel,
    Object? trackNumber = _sentinel,
    Object? discNumber = _sentinel,
    Object? year = _sentinel,
    Object? composer = _sentinel,
    int? bitRate,
    int? sampleRate,
    String? channelLayout,
    int? bitDepth,
    Object? codecName = _sentinel,
    Object? aiGenre = _sentinel,
    Object? bpm = _sentinel,
    Object? key = _sentinel,
    bool? isMyLike,
    Object? comment = _sentinel,
    int? playNum,
  }) {
    return SongInfo(
      songId: songId ?? this.songId,
      duration: duration ?? this.duration,
      title: title ?? this.title,
      artist: _valueOrNull(artist) ?? this.artist,
      album: _valueOrNull(album) ?? this.album,
      albumArtist: _valueOrNull(albumArtist) ?? this.albumArtist,
      genre: _valueOrNull(genre) ?? this.genre,
      trackNumber: _valueOrNull(trackNumber) ?? this.trackNumber,
      discNumber: _valueOrNull(discNumber) ?? this.discNumber,
      year: _valueOrNull(year) ?? this.year,
      composer: _valueOrNull(composer) ?? this.composer,
      bitRate: bitRate ?? this.bitRate,
      sampleRate: sampleRate ?? this.sampleRate,
      channelLayout: channelLayout ?? this.channelLayout,
      bitDepth: bitDepth ?? this.bitDepth,
      codecName: _valueOrNull(codecName) ?? this.codecName,
      aiGenre: _valueOrNull(aiGenre) ?? this.aiGenre,
      bpm: _valueOrNull(bpm) ?? this.bpm,
      key: _valueOrNull(key) ?? this.key,
      isMyLike: isMyLike ?? this.isMyLike,
      comment: _valueOrNull(comment) ?? this.comment,
      playNum: playNum ?? this.playNum,
    );
  }

  // ════════════════════════════════════════════════════════════════
  // 工具方法
  // ════════════════════════════════════════════════════════════════

  /// 将秒数格式化为 分:秒 或 时:分:秒 的可读字符串
  ///
  /// 例如：123 秒 → "2:03"，3661 秒 → "1:01:01"
  ///
  /// 这是实例方法（非 static），需要先有 SongInfo 对象才能调用：
  /// ```dart
  /// final song = SongInfo(duration: 123, ...);
  /// print(song.formatDuration()); // "2:03"
  /// ```
  ///
  /// Dart 语法说明：
  /// - 方法定义在 class 内部，自动属于该类
  /// - `String` 是返回类型
  /// - 没有 `function` 关键字，直接写方法名
  static String formatDuration(int seconds) {
    // 防御性检查：秒数为负或零时的默认处理
    if (seconds <= 0) return '0:00';

    // `~/` 是 Dart 的整除运算符（返回整数，丢弃余数）
    // 等价于 JavaScript 的 Math.floor(seconds / 3600)
    final hours = seconds ~/ 3600;
    final minutes = (seconds % 3600) ~/ 60;
    final secs = seconds % 60;

    if (hours > 0) {
      // `toString()` 把数字转为字符串
      // `padStart(2, '0')` 在左侧补 0 至指定长度
      return '$hours:${minutes.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}';
    }
    return '$minutes:${secs.toString().padStart(2, '0')}';
  }

  // ════════════════════════════════════════════════════════════════
  // 私有辅助
  //
  // Dart 中以下划线 `_` 开头的标识符是库私有的（library-private），
  // 只在当前 .dart 文件内可访问，等价于 Java 的 package-private
  // ════════════════════════════════════════════════════════════════

  /// 哨兵对象：用于区分"没传参数"和"明确传入 null"
  static final Object _sentinel = Object();

  /// 如果值是哨兵对象则返回 null，否则返回原值
  ///
  /// 泛型方法：`T?` 表示返回类型与输入类型相同且可空
  static T? _valueOrNull<T>(Object? value) {
    return identical(value, _sentinel) ? null : value as T?;
  }
}

/// ════════════════════════════════════════════════════════════════
/// Dart class 核心概念速查：
///
/// | 概念          | Dart                          | TypeScript            |
/// |--------------|-------------------------------|-----------------------|
/// | 定义类型      | `class Foo { }`               | `interface Foo { }`  |
/// | 不可变字段    | `final int x;`                | `readonly x: number`  |
/// | 可空字段      | `String? name;`               | `name?: string`       |
/// | 构造函数      | `Foo({required this.x});`     | `constructor(x: ...)`|
/// | 创建副本      | `foo.copyWith(x: 1)`          | `{...foo, x: 1}`     |
/// | 空值合并      | `a ?? b`                      | `a ?? b`             |
/// | 私有成员      | `_privateField`（下划线开头）  | `private field`      |
/// | 字符串插值    | `'Hello $name'`               | `` `Hello ${name}` ``|
/// | 整除          | `a ~/ b`                      | `Math.floor(a / b)`  |
/// ════════════════════════════════════════════════════════════════
