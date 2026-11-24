/// 插件库（后端 `plugins` 表）的前端模型
///
/// ── 数据从哪来 ────────────────────────────────────────────────────────
///   全走 FFI：`getAllPluginsView()` → `{"plugins":[...]}`，
///   字段名和后端 `PluginHost::PluginCatalogManage::Row` 一一对应，改一边必须改另一边：
///   filePath / identifier / name / vendor / category / type /
///   numInputs / numOutputs / scanStatus / scanError
///
/// ── 为什么不用 freezed ────────────────────────────────────────────────
///   和 `model/plugin_chain.dart` 同一个理由：这只是后端 JSON 的只读投影，
///   只需要 fromJson + 一个改 type 的 copyWith，手写更直接，
///   也不用为了编译先跑一轮 build_runner。
///
/// ── type 的三个值是谁定的 ─────────────────────────────────────────────
///   默认值由后端按扫描规则（PluginHost/PluginCategory.cpp）算出来，
///   但**可以人工改**：元数据整条错掉的插件会落进 other，用户能在设置页改掉它，
///   而且重扫不会覆盖（后端 upsert 里 type 那一条是特意留着的）。
///   所以前端的 PluginType 和后端的 `PluginHost::Category` 必须字面量一致。
library;

/// 插件分类，字面量和后端 `PluginHost::Category` / 数据库 `plugins.type` 一致
enum PluginType {
  /// 音源（合成器、采样器）
  instrument,

  /// 效果器
  effect,

  /// 兜底桶：元数据不可信的插件落这里，等用户手动改
  other,
}

extension PluginTypeX on PluginType {
  /// 和后端通信 / 写库用的名字
  String get wireName => switch (this) {
    PluginType.instrument => 'instrument',
    PluginType.effect => 'effect',
    PluginType.other => 'other',
  };

  /// 表格里显示的中文名
  String get displayName => switch (this) {
    PluginType.instrument => '音源',
    PluginType.effect => '效果器',
    PluginType.other => '其他',
  };
}

/// 后端字符串 → 枚举；认不出来一律当 other（不抛异常，UI 不该因为脏数据崩）
PluginType pluginTypeFromWire(String? wire) => switch (wire) {
  'instrument' => PluginType.instrument,
  'effect' => PluginType.effect,
  _ => PluginType.other,
};

/// `plugins` 表的一行
class PluginItem {
  /// `.vst3` 模块的完整路径（Windows 上通常是 bundle 目录）
  final String filePath;

  /// `PluginDescription::createIdentifierString()`，将来做状态/预设要拿它当键。
  ///
  /// 扫描失败的占位行没有它（空串），所以失败行**不能**改 type
  /// —— 后端 `setPluginType` 是按 identifier 定位的。
  final String identifier;

  final String name;
  final String vendor;

  /// 插件自己报的原始分类（"Fx|EQ" / "Instrument|Synth"），只用来展示
  final String category;

  final PluginType type;

  final int numInputs;
  final int numOutputs;

  /// "ok" / "failed"
  final String scanStatus;

  /// 扫描失败的原因（成功时为空）
  final String scanError;

  const PluginItem({
    required this.filePath,
    required this.identifier,
    required this.name,
    required this.vendor,
    required this.category,
    required this.type,
    required this.numInputs,
    required this.numOutputs,
    required this.scanStatus,
    required this.scanError,
  });

  /// 扫描失败的行
  bool get isFailed => scanStatus != 'ok';

  /// 能不能改类型：失败行没有 identifier，改了也定位不到
  bool get canEditType => !isFailed && identifier.isNotEmpty;

  /// 进/出通道，失败行不显示
  String get channelText => '$numInputs / $numOutputs';

  factory PluginItem.fromJson(Map<String, dynamic> json) {
    return PluginItem(
      filePath: _asString(json['filePath']),
      identifier: _asString(json['identifier']),
      name: _asString(json['name']),
      vendor: _asString(json['vendor']),
      category: _asString(json['category']),
      type: pluginTypeFromWire(json['type'] as String?),
      numInputs: _asInt(json['numInputs']),
      numOutputs: _asInt(json['numOutputs']),
      scanStatus: _asString(json['scanStatus']),
      scanError: _asString(json['scanError']),
    );
  }

  /// 只改 type（前端唯一会改的字段）
  PluginItem copyWithType(PluginType newType) {
    return PluginItem(
      filePath: filePath,
      identifier: identifier,
      name: name,
      vendor: vendor,
      category: category,
      type: newType,
      numInputs: numInputs,
      numOutputs: numOutputs,
      scanStatus: scanStatus,
      scanError: scanError,
    );
  }

  /// 缺字段/类型不对时给安全默认值，别让一行脏数据把整张表带崩
  static String _asString(Object? value) => value is String ? value : '';

  static int _asInt(Object? value) {
    if (value is int) return value;
    if (value is num) return value.toInt();
    return 0;
  }
}

/// `pluginDirs` 表的一行（设置页以后做「扫描目录」用，现在只读来展示）
class PluginDirItem {
  final String path;

  /// 系统默认目录（`C:\Program Files\Common Files\VST3`），不允许删除
  final bool isBuiltin;

  const PluginDirItem({required this.path, required this.isBuiltin});

  factory PluginDirItem.fromJson(Map<String, dynamic> json) {
    return PluginDirItem(
      path: json['path'] is String ? json['path'] as String : '',
      isBuiltin: json['isBuiltin'] == true,
    );
  }
}

/// 后台扫描任务的状态（`getPluginScanState()` → `{"scan":{...}}`）
class PluginScanState {
  /// "idle" / "running" / "done" / "failed"
  final String state;
  final int total;
  final int processed;
  final int failed;

  /// 给用户看的一句话（"扫描完成：成功 11 个模块，失败 0 个"）
  final String message;

  const PluginScanState({
    required this.state,
    required this.total,
    required this.processed,
    required this.failed,
    required this.message,
  });

  const PluginScanState.idle()
    : state = 'idle',
      total = 0,
      processed = 0,
      failed = 0,
      message = '';

  bool get isRunning => state == 'running';

  /// 给按钮/进度条用的一行文案
  String get progressText {
    if (state == 'running') {
      return total > 0 ? '正在扫描 $processed / $total' : '正在查找插件…';
    }
    return message;
  }

  factory PluginScanState.fromJson(Map<String, dynamic>? json) {
    if (json == null) return const PluginScanState.idle();

    int asInt(Object? v) => v is int ? v : (v is num ? v.toInt() : 0);

    return PluginScanState(
      state: json['state'] is String ? json['state'] as String : 'idle',
      total: asInt(json['total']),
      processed: asInt(json['processed']),
      failed: asInt(json['failed']),
      message: json['message'] is String ? json['message'] as String : '',
    );
  }
}
