/// EQ 的参数模型（前端侧）
///
/// 字段名与后端 `Model/EqInfo.cpp` 里的 wire key 一一对应，改一边必须改另一边：
///   idx / on / type / freqHz / gainDb / q / shelfSlope / cutSlope
///
/// 为什么这里不用 freezed：这些是纯值对象，字段少、只需要 fromJson/toJson/copyWith，
/// 手写更直接，也不用先跑一轮 build_runner 才能编译。
/// （以后要统一成 freezed 的话，字段名保持不变直接换就行）
library;

/// 一条 band 的滤波器形状，顺序与后端 BandType 枚举无关（用字符串通信）
enum EqBandType {
  bell,
  lowPass,
  highPass,
  notch,
  bandPass,
  allPass,
  lowShelf,
  highShelf,
  tile,
  flatTile,
}

extension EqBandTypeX on EqBandType {
  /// 和后端通信用的名字（EqInfo.cpp 里的 typeName/typeFromName）
  String get wireName => switch (this) {
    EqBandType.bell => 'bell',
    EqBandType.lowPass => 'lowPass',
    EqBandType.highPass => 'highPass',
    EqBandType.notch => 'notch',
    EqBandType.bandPass => 'bandPass',
    EqBandType.allPass => 'allPass',
    EqBandType.lowShelf => 'lowShelf',
    EqBandType.highShelf => 'highShelf',
    EqBandType.tile => 'tile',
    EqBandType.flatTile => 'flatTile',
  };

  /// 下拉框里显示的中文名
  String get displayName => switch (this) {
    EqBandType.bell => '钟型',
    EqBandType.lowPass => '低通',
    EqBandType.highPass => '高通',
    EqBandType.notch => '陷波',
    EqBandType.bandPass => '带通',
    EqBandType.allPass => '全通',
    EqBandType.lowShelf => '低架',
    EqBandType.highShelf => '高架',
    EqBandType.tile => '双边搁架',
    EqBandType.flatTile => '整体倾斜',
  };

  /// 这个类型用不用增益（低通/高通/陷波/带通/全通不用）
  bool get usesGain => switch (this) {
    EqBandType.bell ||
    EqBandType.lowShelf ||
    EqBandType.highShelf ||
    EqBandType.tile ||
    EqBandType.flatTile => true,
    _ => false,
  };

  /// 这个类型用不用 Q（架式/倾斜不用，它们用斜率）
  bool get usesQ => switch (this) {
    EqBandType.lowShelf ||
    EqBandType.highShelf ||
    EqBandType.tile ||
    EqBandType.flatTile => false,
    _ => true,
  };

  /// 这个类型用不用斜率（只有架式/倾斜用）
  bool get usesSlope => !usesQ;

  /// 这个类型用不用"切"的斜率（12/24 dB per oct）
  bool get usesCutSlope =>
      this == EqBandType.lowPass || this == EqBandType.highPass;

  static EqBandType parse(String? name) {
    for (final type in EqBandType.values) {
      if (type.wireName == name) return type;
    }
    return EqBandType.bell; // 认不出来当钟型（和后端兜底一致）
  }
}

/// 一条 band 的全部参数（超集，和实际用哪个字段取决于 type）
class EqBandParam {
  const EqBandParam({
    required this.idx,
    required this.enabled,
    required this.type,
    required this.freqHz,
    required this.gainDb,
    required this.q,
    required this.shelfSlope,
    required this.cutSlopeDbPerOct,
  });

  final int idx;
  final bool enabled;
  final EqBandType type;
  final double freqHz;
  final double gainDb;
  final double q;
  final double shelfSlope;
  final int cutSlopeDbPerOct;

  /// 8 条 band 的初始值：条条关着、频率均分在 20Hz~20kHz 上
  static EqBandParam defaultForIndex(int index) {
    const defaultFreqs = [80.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 16000.0];
    return EqBandParam(
      idx: index,
      enabled: false,
      type: EqBandType.bell,
      freqHz: defaultFreqs[index.clamp(0, defaultFreqs.length - 1)],
      gainDb: 0,
      q: 1.0,
      shelfSlope: 1.0,
      cutSlopeDbPerOct: 12,
    );
  }

  /// 当前这条 band 真正有意义的"横向形状值"（Q 或斜率），给滚轮调参用
  double get shapeValue => type.usesQ ? q : shelfSlope;

  /// 形状值的滑杆范围（Q 用 0.1~10，斜率用 0.1~1.0）
  double get shapeMin => 0.1;
  double get shapeMax => type.usesQ ? 10.0 : 1.0;

  EqBandParam copyWith({
    bool? enabled,
    EqBandType? type,
    double? freqHz,
    double? gainDb,
    double? q,
    double? shelfSlope,
    int? cutSlopeDbPerOct,
  }) {
    return EqBandParam(
      idx: idx,
      enabled: enabled ?? this.enabled,
      type: type ?? this.type,
      freqHz: freqHz ?? this.freqHz,
      gainDb: gainDb ?? this.gainDb,
      q: q ?? this.q,
      shelfSlope: shelfSlope ?? this.shelfSlope,
      cutSlopeDbPerOct: cutSlopeDbPerOct ?? this.cutSlopeDbPerOct,
    );
  }

  /// 按「横轴形状值」改 Q 或斜率（两个字段都写，避免切换类型后值丢失）
  EqBandParam withShapeValue(double value) {
    return copyWith(
      q: type.usesQ ? value : q,
      shelfSlope: type.usesQ ? shelfSlope : value,
    );
  }

  Map<String, dynamic> toJson() => {
    'idx': idx,
    'on': enabled,
    'type': type.wireName,
    'freqHz': freqHz,
    'gainDb': gainDb,
    'q': q,
    'shelfSlope': shelfSlope,
    'cutSlope': cutSlopeDbPerOct,
  };

  factory EqBandParam.fromJson(Map<String, dynamic> json) {
    return EqBandParam(
      idx: (json['idx'] as num?)?.toInt() ?? 0,
      enabled: json['on'] as bool? ?? false,
      type: EqBandTypeX.parse(json['type'] as String?),
      freqHz: (json['freqHz'] as num?)?.toDouble() ?? 1000.0,
      gainDb: (json['gainDb'] as num?)?.toDouble() ?? 0.0,
      q: (json['q'] as num?)?.toDouble() ?? 0.70710678,
      shelfSlope: (json['shelfSlope'] as num?)?.toDouble() ?? 1.0,
      cutSlopeDbPerOct: (json['cutSlope'] as num?)?.toInt() ?? 12,
    );
  }
}

/// 整机 EQ 参数（8 条 band + 旁通 + 输出增益）
class EqAllParam {
  const EqAllParam({
    required this.enabled,
    required this.masterGainDb,
    required this.bands,
  });

  final bool enabled;
  final double masterGainDb;
  final List<EqBandParam> bands;

  static const int bandCount = 8;

  /// 出厂状态：开着、0dB、8 条都关着
  factory EqAllParam.defaults() {
    return EqAllParam(
      enabled: true,
      masterGainDb: 0,
      bands: List.generate(bandCount, EqBandParam.defaultForIndex),
    );
  }

  EqAllParam copyWith({
    bool? enabled,
    double? masterGainDb,
    List<EqBandParam>? bands,
  }) {
    return EqAllParam(
      enabled: enabled ?? this.enabled,
      masterGainDb: masterGainDb ?? this.masterGainDb,
      bands: bands ?? this.bands,
    );
  }

  /// 换掉第 [index] 条 band
  EqAllParam withBand(int index, EqBandParam band) {
    final next = List<EqBandParam>.from(bands);
    next[index] = band;
    return copyWith(bands: next);
  }

  Map<String, dynamic> toJson() => {
    'on': enabled,
    'gainDb': masterGainDb,
    'bands': [for (final band in bands) band.toJson()],
  };

  factory EqAllParam.fromJson(Map<String, dynamic> json) {
    final rawBands = json['bands'];
    final bands = <EqBandParam>[];
    if (rawBands is List) {
      for (final item in rawBands) {
        if (item is Map) {
          bands.add(EqBandParam.fromJson(item.cast<String, dynamic>()));
        }
      }
    }
    // 后端给几条就用几条，缺的补默认值，保证长度恒为 8
    while (bands.length < bandCount) {
      bands.add(EqBandParam.defaultForIndex(bands.length));
    }

    return EqAllParam(
      enabled: json['on'] as bool? ?? true,
      masterGainDb: (json['gainDb'] as num?)?.toDouble() ?? 0.0,
      bands: bands.take(bandCount).toList(growable: false),
    );
  }
}

/// 预设列表里的一项（不含参数体，参数体要单独按 id 拉）
class EqPresetItem {
  const EqPresetItem({
    required this.id,
    required this.title,
    required this.createTime,
  });

  final int id;
  final String title;
  final String createTime;

  factory EqPresetItem.fromJson(Map<String, dynamic> json) {
    return EqPresetItem(
      id: (json['id'] as num?)?.toInt() ?? -1,
      title: json['title'] as String? ?? '未命名',
      createTime: json['createTime'] as String? ?? '',
    );
  }
}
