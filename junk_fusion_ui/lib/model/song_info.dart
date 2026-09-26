import 'package:freezed_annotation/freezed_annotation.dart';

part 'song_info.freezed.dart';
part 'song_info.g.dart'; // 用于 JSON

// SongInfo — 单首歌曲的完整元数据
@freezed
// abstract（抽象类）在这里的作用
// 含义：标记该类不能被直接实例化（不能 new SongInfo()）。
// 为什么这里要用？ 在 freezed 的设计中，SongInfo 只是一个“接口定义”（定义有哪些属性）。真正的实现类（比如 _SongInfo）是隐藏的、由生成器自动创建的。
abstract class SongInfo with _$SongInfo {
  //with _$SongInfo 是 Dart 中 “混入（Mixin）” 的语法，它的核心作用是：
  //将自动生成的 _$SongInfo 类里的所有具体方法（copyWith、==、hashCode 等）“混入”到你手写的 SongInfo 类中，让它们合二为一。
  // 3. 核心：只写这一个工厂构造函数
  //factory 是做什么的？（构造函数控制器）
  // 普通的构造函数（ClassName(...)）必须返回该类的一个新实例。
  // factory 关键字不强制创建新对象，它的职责是 “决定返回什么”。它可以在函数体里返回一个缓存的对象、
  // 返回一个子类对象，或者（在这里）将请求转发给另一个构造函数。
  const factory SongInfo({
    required int songId,
    required double duration,
    required String title,

    required List<String> artists,
    String? album,
    String? albumArtist,
    String? genre,
    int? trackNumber,
    int? discNumber,
    int? year,
    String? composer,

    required int bitRate,
    required int sampleRate,
    required String channelLayout,
    required int bitDepth,

    String? codecName,
    String? aiGenre,
    int? bpm,
    String? key,

    required bool isMyLike,

    String? comment,
    required int playNum,
    String? hash,
  }) = _SongInfo;

  // 4. 关键：从 JSON 创建对象的工厂方法（用于接收 C++ 数据）
  //你调用 SongInfo.fromJson(myMap)。
  // 这个工厂方法直接重定向给顶层的 _$SongInfoFromJson 函数（这个函数是 build_runner 自动写在 song_info.g.dart 文件里的）。
  // _$SongInfoFromJson 会读取 myMap 里的键值对（比如 'songId'、'title'），按照你定义的字段类型，帮你 new 出一个完整的 SongInfo 对象。

  factory SongInfo.fromJson(Map<String, dynamic> json) =>
      _$SongInfoFromJson(json);
}

//dart run build_runner build --delete-conflicting-outputs
