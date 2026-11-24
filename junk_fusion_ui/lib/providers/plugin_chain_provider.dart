/// 全局效果器链的数据层（Provider）
///
/// ── 职责 ──────────────────────────────────────────────────────────────
///   1. 持有"当前全局链"（0~8 个插件的有序列表）与"音频进程回报的状态"；
///   2. 把链下发给音频进程（`setPluginChain`），旁通开关单独走轻量指令；
///   3. 链预设的增删改查（预设 = 一条完整的链，含每个插件的 state）。
///
/// ── 全局链 vs 预设 ────────────────────────────────────────────────────
///   · 全局链：用户正在编辑的那条，会真的插在交叉淡化之后出声；
///   · 预设：存下来的链，可以贴到全局（现在）或贴到某个分轨（以后）。
///   两者用同一份数据结构 [ChainSlot]，所以"存为预设"和"载入预设"都只是搬运。
///
/// ── 顺序 / 脏标记 ─────────────────────────────────────────────────────
///   增删、更换插件、拖动排序都**只改前端**并标脏，点「应用」才重建：
///   重建要重新实例化插件（实测 CHOWTapeModel 1.86 秒），拖动一次就重建的话没法用。
///   重建期间音频照常放旧链，只是"新链生效前那一段还是旧声音"。
///   旁通是纯开关，立即生效。
///
/// ── 用法 ─────────────────────────────────────────────────────────────
/// ```dart
/// chain.ensureInitialized();
/// chain.addSlot(pluginItem);        // 追加
/// chain.moveSlot(0, 2);             // 拖动排序（前端）
/// chain.toggleSlotBypass(0);        // 立即生效
/// chain.apply();                    // 重建整链
/// chain.applyPreset(id);            // 载入预设并立即下发
/// chain.saveChainAsPreset('母带');  // 存当前链
/// chain.updatePresetChain(id, '名字', slots);  // 双击预设改完保存
/// ```
library;

import 'dart:async';
import 'dart:convert';
import 'dart:ffi';

import 'package:ffi/ffi.dart' show malloc;
import 'package:flutter/foundation.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/model/plugin_chain.dart';
import 'package:junk_fusion_ui/model/plugin_info.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';

class PluginChainProvider extends ChangeNotifier {
  /// 一条链最多几个插件：和后端 `PluginChain::kSlotCount` 必须一致
  static const int maxSlots = 8;

  // ── 全局链 ────────────────────────────────────────────────────────
  List<ChainSlot> _slots = const <ChainSlot>[];
  bool _bypassAll = false;
  bool _dirty = false;

  // ── 运行状态（音频进程回传）────────────────────────────────────────
  ChainStatus _status = const ChainStatus.none();

  // ── 预设 ──────────────────────────────────────────────────────────
  List<ChainPresetItem> _presets = const <ChainPresetItem>[];

  bool _initialized = false;
  String? _lastError;

  Timer? _pollTimer;
  int _pollTries = 0;

  // ── 只读 ──────────────────────────────────────────────────────────
  List<ChainSlot> get slots => _slots;
  ChainStatus get status => _status;
  List<ChainPresetItem> get presets => _presets;
  bool get bypassAll => _bypassAll;
  bool get isDirty => _dirty;
  bool get isFull => _slots.length >= maxSlots;
  int get length => _slots.length;
  String? get lastError => _lastError;

  int get totalLatency => _status.latency;
  bool get anyLoaded => _status.anyLoaded;

  ChainSlotStatus statusOf(int position) =>
      _status.slots[position] ?? const ChainSlotStatus.none();

  void clearError() {
    _lastError = null;
  }

  // ════════════════════════════════════════════════════════════════
  // 初始化 / 刷新
  // ════════════════════════════════════════════════════════════════
  void ensureInitialized() {
    if (_initialized) return;
    _initialized = true;
    refreshPresets();
    refreshState();
    _restoreSavedChain();
  }

  /// 启动时把"上次应用的链"恢复回来
  ///
  /// 存在 pluginChainState 表里（Coordinator 在每次 apply 成功后写），
  /// 所以这里只负责读回来 + 重新下发一次。音频进程可能还没起来，
  /// 所以第一次下发失败会再补一次（总共两次，不做无限重试——
  /// 无限重试遇到坏插件就变成崩溃循环了）。
  void _restoreSavedChain() {
    final envelope = _readJsonObject(bindings.getSavedPluginChain());
    final raw = envelope?['chain'];
    if (raw is! Map) return;

    final parsed = _parseChain(raw.cast<String, dynamic>());
    if (parsed.slots.isEmpty) return;

    loadChain(parsed.slots, bypassAll: parsed.bypassAll);
    _dirty = true;
    _applyRestoredChain(hasRetry: true);
  }

  Future<void> _applyRestoredChain({required bool hasRetry}) async {
    // 给音频进程一点启动时间
    await Future<void>.delayed(const Duration(milliseconds: 800));
    if (_slots.isEmpty) return;

    apply();

    if (!hasRetry) return;
    await Future<void>.delayed(const Duration(seconds: 4));
    if (_status.anyLoaded || _slots.isEmpty) return;

    // 第二次也是最后一次
    apply();
  }

  void refreshState() {
    final envelope = _readJsonObject(bindings.getPluginChainState());
    final raw = envelope?['chain'];
    _status = raw is Map
        ? ChainStatus.fromJson(raw.cast<String, dynamic>())
        : const ChainStatus.none();
    notifyListeners();
  }

  void refreshPresets() {
    final envelope = _readJsonObject(bindings.getAllPluginChainPresets());
    final raw = envelope?['presets'];
    _presets = raw is List
        ? <ChainPresetItem>[
            for (final item in raw)
              if (item is Map)
                ChainPresetItem.fromJson(item.cast<String, dynamic>()),
          ]
        : const <ChainPresetItem>[];
    notifyListeners();
  }

  // ════════════════════════════════════════════════════════════════
  // 编辑全局链（都只改前端 + 标脏）
  // ════════════════════════════════════════════════════════════════
  /// 追加一个插件；满了返回 false
  bool addSlot(PluginItem plugin) {
    if (isFull) return false;
    if (plugin.identifier.isEmpty) {
      _lastError = '这个插件没有 identifier，无法加载（重新扫一遍试试）';
      notifyListeners();
      return false;
    }

    _slots = [
      ..._slots,
      ChainSlot.create(
        index: _slots.length,
        filePath: plugin.filePath,
        identifier: plugin.identifier,
        name: plugin.name,
      ),
    ];
    _dirty = true;
    notifyListeners();
    return true;
  }

  void removeSlot(int position) {
    if (position < 0 || position >= _slots.length) return;
    _slots = [..._slots]..removeAt(position);
    _renumber();
    _dirty = true;
    notifyListeners();
  }

  /// 拖动排序（前端）
  ///
  /// 入参与 `ReorderableListView.onReorder` 一致（newIndex 是"插入到哪个下标之前"，
  /// 往下拖时要减 1）。
  void moveSlot(int oldIndex, int newIndex) {
    if (oldIndex < 0 || oldIndex >= _slots.length) return;
    var target = newIndex;
    if (target > oldIndex) target -= 1;
    if (target < 0) target = 0;
    if (target >= _slots.length) target = _slots.length - 1;
    if (target == oldIndex) return;

    final next = [..._slots];
    final moved = next.removeAt(oldIndex);
    next.insert(target, moved);
    _slots = next;
    _renumber();
    _dirty = true;
    notifyListeners();
  }

  /// 整链替换（载入预设 / 对话框里"保存并应用"）
  void loadChain(List<ChainSlot> chain, {bool? bypassAll}) {
    _slots = [...chain];
    _renumber();
    if (bypassAll != null) _bypassAll = bypassAll;
    _dirty = true;
    notifyListeners();
  }

  /// 某个条目的旁通：立即生效，不需要点应用
  void toggleSlotBypass(int position) {
    if (position < 0 || position >= _slots.length) return;

    final next = [..._slots];
    final bypass = !next[position].bypass;
    next[position] = next[position].copyWith(bypass: bypass);
    _slots = next;

    bindings.setPluginChainSlotBypass(position, bypass ? 1 : 0);
    notifyListeners();
  }

  /// 整链旁通：立即生效
  void toggleBypassAll() {
    _bypassAll = !_bypassAll;
    bindings.setPluginChainBypassAll(_bypassAll ? 1 : 0);
    notifyListeners();
  }

  /// 打开某个条目的插件编辑器（窗口开在音频进程里）
  void openEditor(int position) {
    if (position < 0 || position >= _slots.length) return;
    bindings.openPluginSlotEditor(position);
  }

  // ════════════════════════════════════════════════════════════════
  // 下发
  // ════════════════════════════════════════════════════════════════
  Map<String, dynamic> _configJson() {
    return {
      'bypassAll': _bypassAll,
      'slots': [for (final slot in _slots) slot.toJson()],
    };
  }

  /// 下发整链；返回 false 时原因在 [lastError]
  bool apply() {
    final result = _withCString(
      jsonEncode(_configJson()),
      (ptr) => _readJsonObject(bindings.setPluginChain(ptr)),
    );

    if (result?['ok'] != true) {
      _lastError = result?['error'] is String
          ? result!['error'] as String
          : '下发效果器链失败';
      notifyListeners();
      return false;
    }

    // 黑名单挡掉的插件：用 warning 通知，别让人以为"没生效"
    final dropped = result?['dropped'];
    if (dropped is List && dropped.isNotEmpty) {
      _lastError = '有 ${dropped.length} 个插件在黑名单里，没有下发（设置页可强制启用）';
    }

    _dirty = false;
    _startPolling();
    notifyListeners();
    return true;
  }

  void _startPolling() {
    _pollTimer?.cancel();
    _pollTries = 0;
    _pollTimer = Timer.periodic(const Duration(milliseconds: 300), (timer) {
      _pollTries++;
      refreshState();
      // 加载完（anyLoaded）或者超时 3 秒就停
      final stillBusy = !_status.anyLoaded && _pollTries < 10;
      if (!stillBusy) {
        timer.cancel();
        _pollTimer = null;
      }
    });
  }

  // ════════════════════════════════════════════════════════════════
  // 预设
  // ════════════════════════════════════════════════════════════════
  /// 载入预设并立即下发
  bool applyPreset(int id) {
    final parsed = _readPresetChain(id);
    if (parsed == null) {
      _lastError = '预设读不出来，可能数据损坏';
      notifyListeners();
      return false;
    }

    loadChain(parsed.slots, bypassAll: parsed.bypassAll);
    return apply();
  }

  /// 读一个预设的配置体（不应用）
  ({List<ChainSlot> slots, bool bypassAll})? _readPresetChain(int id) {
    final envelope = _readJsonObject(bindings.getPluginChainPreset(id));
    final raw = envelope?['chain'];
    if (raw is! Map) return null;
    return _parseChain(raw.cast<String, dynamic>());
  }

  /// 把一个预设的链读出来（双击预设打开对话框时用）
  ({List<ChainSlot> slots, bool bypassAll})? readPresetChain(int id) =>
      _readPresetChain(id);

  /// 存当前链为新预设；返回新 id（-1 = 失败）
  int saveCurrentChainAsPreset(String title) =>
      saveChainAsPreset(title, _slots);

  /// 把任意一条链存为新预设
  int saveChainAsPreset(String title, List<ChainSlot> chain) {
    if (title.trim().isEmpty || chain.isEmpty) return -1;

    final id = _withCString(
      title.trim(),
      (titlePtr) => _withCString(_jsonOf(chain), (jsonPtr) {
        return bindings.savePluginChainPreset(titlePtr, jsonPtr);
      }),
    );

    if (id > 0) refreshPresets();
    return id;
  }

  /// 覆盖已有预设（双击预设 → 改名 / 改链 → 保存）
  bool updatePresetChain(int id, String title, List<ChainSlot> chain) {
    if (title.trim().isEmpty) return false;

    final result = _withCString(title.trim(), (titlePtr) {
      return _withCString(_jsonOf(chain), (jsonPtr) {
        return _readJsonObject(
          bindings.updatePluginChainPreset(id, titlePtr, jsonPtr),
        );
      });
    });

    final ok = result?['ok'] == true;
    if (ok) refreshPresets();
    return ok;
  }

  void deletePreset(int id) {
    bindings.deletePluginChainPreset(id);
    refreshPresets();
  }

  void renamePreset(int id, String title) {
    if (title.trim().isEmpty) return;
    _withCString(title.trim(), (ptr) {
      bindings.renamePluginChainPreset(id, ptr);
      return 0;
    });
    refreshPresets();
  }

  @override
  void dispose() {
    _pollTimer?.cancel();
    _pollTimer = null;
    super.dispose();
  }

  // ════════════════════════════════════════════════════════════════
  // 内部
  // ════════════════════════════════════════════════════════════════

  /// 把顺序号按当前位置重排（去掉空洞之后 index 就是位置）
  void _renumber() {
    _slots = [
      for (var position = 0; position < _slots.length; position++)
        _slots[position].copyWith(index: position),
    ];
  }

  String _jsonOf(List<ChainSlot> chain) {
    final renumbered = [
      for (var position = 0; position < chain.length; position++)
        chain[position].copyWith(index: position),
    ];
    return jsonEncode({
      'bypassAll': false,
      'slots': [for (final slot in renumbered) slot.toJson()],
    });
  }

  ({List<ChainSlot> slots, bool bypassAll}) _parseChain(
    Map<String, dynamic> json,
  ) {
    final slots = <ChainSlot>[];
    final rawSlots = json['slots'];
    if (rawSlots is List) {
      for (final item in rawSlots) {
        if (item is! Map) continue;
        final slot = ChainSlot.fromJson(item.cast<String, dynamic>());
        if (slot.isEmpty) continue; // 老数据里可能有空槽，忽略
        slots.add(slot.copyWith(index: slots.length));
      }
    }
    return (slots: slots, bypassAll: json['bypassAll'] == true);
  }

  T _withCString<T>(String text, T Function(Pointer<Char>) action) {
    final ptr = UtilFunction.string2cPtr(text);
    try {
      return action(ptr);
    } finally {
      malloc.free(ptr);
    }
  }

  Map<String, dynamic>? _readJsonObject(Pointer<Char> ptr) {
    if (ptr == nullptr) return null;

    final text = UtilFunction.cPtr2String(ptr);
    if (text.isEmpty) return null;

    try {
      final decoded = jsonDecode(text);
      return decoded is Map<String, dynamic> ? decoded : null;
    } catch (error) {
      _lastError = '解析后端返回失败: $error';
      return null;
    }
  }
}
