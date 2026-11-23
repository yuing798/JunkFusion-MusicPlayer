/// 全局效果器链的前端模型（8 个插槽）
///
/// ── 两份数据，别混 ────────────────────────────────────────────────────
///   1. **配置**（[ChainSlot]）：index / filePath / identifier / name / bypass / state，
///      是"用户想要什么"。权威副本在 Coordinator，前端改完通过 `setPluginChain` 推上去；
///      预设（pluginChainPresets.slots）存的也是这一坨。
///   2. **状态**（[ChainSlotStatus]）：loaded / latency / error，
///      是"音频进程实际加载成什么样"，由音频进程回传，前端只读。
///
///   `state` 是插件自己的 getStateInformation（base64），**前端只是个搬运工**：
///   选新插件时清空，切预设时原样带回去。不要在前端解析它。
///
/// ── 为什么不用 freezed ────────────────────────────────────────────────
///   和 model/plugin_info.dart 同一个理由：只是 JSON 的投影 + 几个 copyWith，
///   手写更直接，也不用为了编译先跑 build_runner。
library;

/// 一个插槽的**配置**（用户意图）
class ChainSlot {
  final int index;

  /// 空串 = 空槽
  final String filePath;
  final String identifier;
  final String name;
  final bool bypass;

  /// 插件状态 blob（base64，不透明）。选新插件时为空，切预设时原样保留。
  final String state;

  const ChainSlot({
    required this.index,
    required this.filePath,
    required this.identifier,
    required this.name,
    required this.bypass,
    required this.state,
  });

  const ChainSlot.empty(this.index)
    : filePath = '',
      identifier = '',
      name = '',
      bypass = false,
      state = '';

  bool get isEmpty => filePath.isEmpty;

  ChainSlot copyWith({bool? bypass, bool clearPlugin = false, String? state}) {
    if (clearPlugin) {
      return ChainSlot(
        index: index,
        filePath: '',
        identifier: '',
        name: '',
        bypass: false,
        state: '',
      );
    }
    return ChainSlot(
      index: index,
      filePath: filePath,
      identifier: identifier,
      name: name,
      bypass: bypass ?? this.bypass,
      state: state ?? this.state,
    );
  }

  factory ChainSlot.fromJson(Map<String, dynamic> json) {
    return ChainSlot(
      index: _asInt(json['index']),
      filePath: _asString(json['filePath']),
      identifier: _asString(json['identifier']),
      name: _asString(json['name']),
      bypass: json['bypass'] == true,
      state: _asString(json['state']),
    );
  }

  Map<String, dynamic> toJson() => {
    'index': index,
    'filePath': filePath,
    'identifier': identifier,
    'name': name,
    'bypass': bypass,
    'state': state,
  };

  static String _asString(Object? value) => value is String ? value : '';

  static int _asInt(Object? value) {
    if (value is int) return value;
    if (value is num) return value.toInt();
    return 0;
  }
}

/// 一个插槽的**运行状态**（音频进程回传，只读）
class ChainSlotStatus {
  final bool loaded;
  final bool bypass;
  final int latency;
  final String error;

  const ChainSlotStatus({
    required this.loaded,
    required this.bypass,
    required this.latency,
    required this.error,
  });

  const ChainSlotStatus.none()
    : loaded = false,
      bypass = false,
      latency = 0,
      error = '';

  factory ChainSlotStatus.fromJson(Map<String, dynamic> json) {
    return ChainSlotStatus(
      loaded: json['loaded'] == true,
      bypass: json['bypass'] == true,
      latency: json['latency'] is num ? (json['latency'] as num).toInt() : 0,
      error: json['error'] is String ? json['error'] as String : '',
    );
  }
}

/// 音频进程回报的整链状态
class ChainStatus {
  final int latency;
  final bool anyLoaded;
  final Map<int, ChainSlotStatus> slots;

  const ChainStatus({
    required this.latency,
    required this.anyLoaded,
    required this.slots,
  });

  const ChainStatus.none() : latency = 0, anyLoaded = false, slots = const {};

  /// 解析 `getPluginChainState()` 内层的 `chain`
  factory ChainStatus.fromJson(Map<String, dynamic>? json) {
    if (json == null) return const ChainStatus.none();

    final rawSlots = json['slots'];
    final map = <int, ChainSlotStatus>{};
    if (rawSlots is List) {
      for (final raw in rawSlots) {
        if (raw is! Map) continue;
        final item = raw.cast<String, dynamic>();
        final index = item['index'] is num
            ? (item['index'] as num).toInt()
            : -1;
        if (index < 0) continue;
        map[index] = ChainSlotStatus.fromJson(item);
      }
    }

    return ChainStatus(
      latency: json['latency'] is num ? (json['latency'] as num).toInt() : 0,
      anyLoaded: json['anyLoaded'] == true,
      slots: map,
    );
  }
}

/// 链预设（列表项，不含配置体）
class ChainPresetItem {
  final int id;
  final String title;
  final String createTime;

  const ChainPresetItem({
    required this.id,
    required this.title,
    required this.createTime,
  });

  factory ChainPresetItem.fromJson(Map<String, dynamic> json) {
    return ChainPresetItem(
      id: json['id'] is num ? (json['id'] as num).toInt() : -1,
      title: json['title'] is String ? json['title'] as String : '',
      createTime: json['createTime'] is String
          ? json['createTime'] as String
          : '',
    );
  }
}

/// 黑名单一行（系统观测）
class PluginBanItem {
  final String identifier;
  final String filePath;
  final String reason;
  final int failureCount;
  final String lastFailedAt;

  const PluginBanItem({
    required this.identifier,
    required this.filePath,
    required this.reason,
    required this.failureCount,
    required this.lastFailedAt,
  });

  factory PluginBanItem.fromJson(Map<String, dynamic> json) {
    return PluginBanItem(
      identifier: json['identifier'] is String
          ? json['identifier'] as String
          : '',
      filePath: json['filePath'] is String ? json['filePath'] as String : '',
      reason: json['reason'] is String ? json['reason'] as String : '',
      failureCount: json['failureCount'] is num
          ? (json['failureCount'] as num).toInt()
          : 0,
      lastFailedAt: json['lastFailedAt'] is String
          ? json['lastFailedAt'] as String
          : '',
    );
  }
}

/// 白名单一行（用户意图）
class PluginAllowItem {
  final String identifier;
  final String filePath;
  final String note;
  final String createdAt;

  const PluginAllowItem({
    required this.identifier,
    required this.filePath,
    required this.note,
    required this.createdAt,
  });

  factory PluginAllowItem.fromJson(Map<String, dynamic> json) {
    return PluginAllowItem(
      identifier: json['identifier'] is String
          ? json['identifier'] as String
          : '',
      filePath: json['filePath'] is String ? json['filePath'] as String : '',
      note: json['note'] is String ? json['note'] as String : '',
      createdAt: json['createdAt'] is String ? json['createdAt'] as String : '',
    );
  }
}
