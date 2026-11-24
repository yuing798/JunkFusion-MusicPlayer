/// 全局效果器链的数据层（Provider）
///
/// ── 职责 ──────────────────────────────────────────────────────────────
///   1. 持有"8 个插槽的配置"（用户意图）与"音频进程回报的状态"（实际加载结果）；
///   2. 把配置下发给音频进程（`setPluginChain`），旁通开关单独走轻量指令；
///   3. 链预设的增删查（存的是配置体 JSON，含每个插件的 state）。
///
/// ── 为什么配置放前端 ──────────────────────────────────────────────────
///   和 EQ 一样是"编辑态"：用户在页面上挑插件、改旁通、点应用。
///   Coordinator 只在下发时留一份副本（用于重发/查询），权威永远是用户刚点的那一下。
///
/// ── 状态为什么要轮询 ──────────────────────────────────────────────────
///   实例化插件要几十~几百毫秒，音频进程加载完才回传 `pluginChainState`。
///   只在"应用之后"轮询 3 秒，平时不轮询（FFI 调用本身很便宜，但没必要）。
///
/// ── 用法 ─────────────────────────────────────────────────────────────
/// ```dart
/// context.read<PluginChainProvider>().ensureInitialized();
/// provider.loadSlot(0, pluginItem);       // 选插件
/// provider.toggleSlotBypass(0);           // 单槽旁通（立即生效）
/// provider.apply();                        // 下发整链
/// provider.savePreset('我的母带链');
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
  /// 槽位数：和后端 `PluginChain::kSlotCount` 必须一致
  static const int slotCount = 8;

  List<ChainSlot> _slots = List<ChainSlot>.generate(
    slotCount,
    (index) => ChainSlot.empty(index),
  );
  ChainStatus _status = const ChainStatus.none();
  List<ChainPresetItem> _presets = const <ChainPresetItem>[];

  bool _bypassAll = false;
  bool _initialized = false;

  /// 配置改过但还没应用（页面上给"应用"按钮加高亮）
  bool _dirty = false;

  String? _lastError;
  Timer? _pollTimer;
  int _pollTries = 0;

  // ── 只读状态 ──────────────────────────────────────────────────────
  List<ChainSlot> get slots => _slots;
  ChainStatus get status => _status;
  List<ChainPresetItem> get presets => _presets;
  bool get bypassAll => _bypassAll;
  bool get isDirty => _dirty;
  String? get lastError => _lastError;

  int get totalLatency => _status.latency;
  bool get anyLoaded => _status.anyLoaded;

  ChainSlotStatus statusOf(int slot) =>
      _status.slots[slot] ?? const ChainSlotStatus.none();

  void clearError() {
    _lastError = null;
  }

  // ── 初始化 ────────────────────────────────────────────────────────
  void ensureInitialized() {
    if (_initialized) return;
    _initialized = true;
    refreshPresets();
    refreshState();
  }

  // ── 读状态 ────────────────────────────────────────────────────────
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

  // ── 编辑 ──────────────────────────────────────────────────────────
  /// 给某个槽选一个插件（状态 blob 清空：新插件没有旧状态）
  void loadSlot(int slot, PluginItem plugin) {
    if (slot < 0 || slot >= slotCount) return;
    _slots[slot] = ChainSlot(
      index: slot,
      filePath: plugin.filePath,
      identifier: plugin.identifier,
      name: plugin.name,
      bypass: false,
      state: '',
    );
    _dirty = true;
    notifyListeners();
  }

  void clearSlot(int slot) {
    if (slot < 0 || slot >= slotCount) return;
    _slots[slot] = ChainSlot.empty(slot);
    _dirty = true;
    notifyListeners();
  }

  /// 单槽旁通：立即生效，不需要点"应用"
  void toggleSlotBypass(int slot) {
    if (slot < 0 || slot >= slotCount) return;
    final next = !_slots[slot].bypass;
    _slots[slot] = _slots[slot].copyWith(bypass: next);
    bindings.setPluginChainSlotBypass(slot, next ? 1 : 0);
    notifyListeners();
  }

  /// 整链旁通：立即生效
  void toggleBypassAll() {
    _bypassAll = !_bypassAll;
    bindings.setPluginChainBypassAll(_bypassAll ? 1 : 0);
    notifyListeners();
  }

  /// 打开某个槽的插件编辑器
  ///
  /// 窗口开在**音频进程**里（插件实例在那边，消息循环也在那边）。
  /// 音频进程重启后窗口会消失，再点一次即可。
  void openEditor(int slot) {
    if (slot < 0 || slot >= slotCount) return;
    bindings.openPluginSlotEditor(slot);
  }

  // ── 下发 ──────────────────────────────────────────────────────────
  Map<String, dynamic> _configJson() {
    return {
      'bypassAll': _bypassAll,
      'slots': [
        for (final slot in _slots)
          if (!slot.isEmpty) slot.toJson(),
      ],
    };
  }

  /// 下发整链；返回 false 时原因在 [lastError]
  bool apply() {
    final json = jsonEncode(_configJson());
    final result = _withCString(
      json,
      (ptr) => _readJsonObject(bindings.setPluginChain(ptr)),
    );

    if (result?['ok'] != true) {
      _lastError = result?['error'] is String
          ? result!['error'] as String
          : '下发效果器链失败';
      notifyListeners();
      return false;
    }

    // 黑名单挡掉的插件：告诉用户，别让人以为"没生效"
    final dropped = result?['dropped'];
    if (dropped is List && dropped.isNotEmpty) {
      _lastError = '有 ${dropped.length} 个插件在黑名单里，没有下发（可在设置页强制启用）';
    }

    _dirty = false;
    _startPolling();
    notifyListeners();
    return true;
  }

  void _startPolling() {
    _pollTimer?.cancel();
    _pollTries = 0;
    _pollTimer = Timer.periodic(const Duration(milliseconds: 300), (_) {
      _pollTries++;
      refreshState();
      final stillBusy = !_status.anyLoaded && _pollTries < 10;
      if (!stillBusy) {
        _pollTimer?.cancel();
        _pollTimer = null;
      }
    });
  }

  // ── 预设 ──────────────────────────────────────────────────────────
  /// 把当前 8 槽存成新预设；返回新 id（-1 = 失败）
  int savePreset(String title) {
    if (title.trim().isEmpty) return -1;

    final json = jsonEncode(_configJson());
    final id = _withCString(
      title.trim(),
      (titlePtr) => _withCString(json, (jsonPtr) {
        return bindings.savePluginChainPreset(titlePtr, jsonPtr);
      }),
    );

    if (id > 0) refreshPresets();
    return id;
  }

  /// 载入预设并立即下发
  bool applyPreset(int id) {
    final envelope = _readJsonObject(bindings.getPluginChainPreset(id));
    final raw = envelope?['chain'];
    if (raw is! Map) {
      _lastError = '预设读不出来，可能数据损坏';
      notifyListeners();
      return false;
    }

    final config = raw.cast<String, dynamic>();
    final parsed = List<ChainSlot>.generate(
      slotCount,
      (index) => ChainSlot.empty(index),
    );

    final rawSlots = config['slots'];
    if (rawSlots is List) {
      for (final item in rawSlots) {
        if (item is! Map) continue;
        final slot = ChainSlot.fromJson(item.cast<String, dynamic>());
        if (slot.index < 0 || slot.index >= slotCount) continue;
        parsed[slot.index] = slot;
      }
    }

    _slots = parsed;
    _bypassAll = config['bypassAll'] == true;
    _dirty = true;
    notifyListeners();

    return apply();
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

  // ── 内部：FFI 字符串转换 ──────────────────────────────────────────
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
