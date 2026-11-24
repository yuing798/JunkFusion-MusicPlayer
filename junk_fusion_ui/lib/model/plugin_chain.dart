/// 全局效果器链的前端模型
///
/// ── 一条链 = 0~8 个插件的有序列表（不是 8 个固定槽位）──────────────
///   为什么去掉"空洞"：链在界面上是 `输入 → [卡] → [卡] → 输出` 这种箭头流，
///   中间空一格在视觉上没有意义（还会让人以为信号会跳过它）。
///   所以 `index` 只是"下发给后端时的顺序号"，由前端在每次增删/拖动后重新编号；
///   后端本来就是按下标顺序处理，不需要改。
///
/// ── 两份数据，别混 ────────────────────────────────────────────────────
///   1. **配置**（[ChainSlot]）：插件是谁、旁通没有、插件状态 blob，
///      是"用户想要什么"。权威副本在 Coordinator，前端改完通过 `setPluginChain` 推上去；
///      预设（pluginChainPresets.slots）存的也是这一坨。
///   2. **状态**（[ChainSlotStatus]）：loaded / latency / error，
///      是"音频进程实际加载成什么样"，由音频进程回传，前端只读。
///
///   `state` 是插件自己的 getStateInformation（base64），前端只是个搬运工：
///   换插件时清空，切预设时原样带回去，不要解析它。
///
/// ── uid 是什么 ────────────────────────────────────────────────────────
///   拖动排序的动画库要求 `isSameItem(a, b)` 能判断"是不是同一个条目"，
///   而配置本身是可变的（改旁通就换了一个对象），所以给每个条目一个前端自增的
///   `uid`，**只用于列表身份，不落库、不下发**。
library;

/// 前端自增的条目身份（只用来做列表动画，不落库）
int _nextSlotUid = 0;

int _allocateSlotUid() => _nextSlotUid++;

/// 链上的一个效果器（用户意图）
class ChainSlot {
  /// 列表身份（拖动动画用），不落库、不下发
  final int uid;

  /// 在链里的顺序号：**每次增删/拖动后由前端重新编号**，下发时用它
  final int index;

  /// 空串 = 这个条目没有插件（理论上不该出现，防御性判断）
  final String filePath;
  final String identifier;
  final String name;
  final bool bypass;

  /// 插件状态 blob（base64，不透明）
  final String state;

  ChainSlot({
    required this.uid,
    required this.index,
    required this.filePath,
    required this.identifier,
    required this.name,
    required this.bypass,
    required this.state,
  });

  /// 新建一个条目（分配新的 uid）
  factory ChainSlot.create({
    required int index,
    required String filePath,
    required String identifier,
    required String name,
    bool bypass = false,
    String state = '',
  }) {
    return ChainSlot(
      uid: _allocateSlotUid(),
      index: index,
      filePath: filePath,
      identifier: identifier,
      name: name,
      bypass: bypass,
      state: state,
    );
  }

  factory ChainSlot.fromJson(Map<String, dynamic> json) {
    return ChainSlot(
      uid: _allocateSlotUid(),
      index: _asInt(json['index']),
      filePath: _asString(json['filePath']),
      identifier: _asString(json['identifier']),
      name: _asString(json['name']),
      bypass: json['bypass'] == true,
      state: _asString(json['state']),
    );
  }

  bool get isEmpty => filePath.isEmpty;

  ChainSlot copyWith({int? index, bool? bypass, String? state}) {
    return ChainSlot(
      uid: uid,
      index: index ?? this.index,
      filePath: filePath,
      identifier: identifier,
      name: name,
      bypass: bypass ?? this.bypass,
      state: state ?? this.state,
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

  /// "TDR Nova → CHOWTapeModel" 这种链预览，后端在查列表时顺手拼好
  final String preview;

  const ChainPresetItem({
    required this.id,
    required this.title,
    required this.createTime,
    required this.preview,
  });

  factory ChainPresetItem.fromJson(Map<String, dynamic> json) {
    return ChainPresetItem(
      id: json['id'] is num ? (json['id'] as num).toInt() : -1,
      title: json['title'] is String ? json['title'] as String : '',
      createTime: json['createTime'] is String
          ? json['createTime'] as String
          : '',
      preview: json['preview'] is String ? json['preview'] as String : '',
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
