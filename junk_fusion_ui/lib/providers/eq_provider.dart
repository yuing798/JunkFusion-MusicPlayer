/// EQ 页的数据层（Provider）
///
/// ── 职责 ──────────────────────────────────────────────────────────────
/// 1. 持有"当前 EQ 参数"的前端副本（真正权威的那份在后端 EqController）；
/// 2. 按"更新频率"选通道：
///      拖拽中      → updateBandLive（OSC/UDP，可丢帧，跟手优先）
///      松手/换类型 → setBand（TCP JSON，必达，把最终值定死）
///    **拖拽结束一定要 commitBand**，否则最后一帧可能被节流丢掉；
/// 3. 缓存后端算好的频响曲线（后端和 DSP 用同一份滤波数学，曲线就是实际响应）；
/// 4. 预设列表的增删改查。
///
/// ── 用法 ─────────────────────────────────────────────────────────────
///     `context.read<EqProvider>().ensureInitialized();`   // 进页面时一次
///     `context.watch<EqProvider>()`                       // 画 UI
///
/// ── FFI 从哪来 ────────────────────────────────────────────────────────
/// 直接用 ffigen 生成的 `bindings`（`npm run ffigen`），本文件不再维护
/// 第二份手写绑定。字符串/JSON 的来回转换都在本文件底部的私有方法里。
library;

import 'dart:convert';
import 'dart:ffi';

import 'package:ffi/ffi.dart' show malloc;
import 'package:flutter/foundation.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/model/eq_param.dart';
import 'package:junk_fusion_ui/utils/util_function.dart';

class EqProvider extends ChangeNotifier {
  /// 曲线采样点数：256 点在常见宽度下已经看不出折线
  static const int curvePoints = 256;

  /// 请求曲线的点数（后端会钳到 2~2048）
  static const int _minCurvePoints = 2;
  static const int _maxCurvePoints = 2048;

  EqAllParam _state = EqAllParam.defaults();
  List<EqPresetItem> _presets = const <EqPresetItem>[];
  List<double> _curve = const <double>[];
  int? _selectedBand;
  int? _activePresetId;
  bool _initialized = false;

  // ── 只读状态 ──────────────────────────────────────────────────────
  EqAllParam get state => _state;
  List<EqBandParam> get bands => _state.bands;
  List<EqPresetItem> get presets => _presets;

  /// 频响曲线（dB），长度 == curvePoints；为空表示后端还没就绪
  List<double> get curve => _curve;

  int? get selectedBand => _selectedBand;
  int? get activePresetId => _activePresetId;

  EqBandParam? get selectedBandParam {
    final index = _selectedBand;
    if (index == null || index < 0 || index >= _state.bands.length) return null;
    return _state.bands[index];
  }

  // ── 初始化 ────────────────────────────────────────────────────────
  /// 进页面时调一次：拉一次后端状态 + 预设列表
  void ensureInitialized() {
    if (_initialized) return;
    _initialized = true;

    final remote = _requestState();
    if (remote != null) _state = remote;

    _refreshCurve();
    refreshPresets();
    notifyListeners();
  }

  // ── 选中 ──────────────────────────────────────────────────────────
  void selectBand(int? index) {
    if (_selectedBand == index) return;
    _selectedBand = index;
    notifyListeners();
  }

  // ── 拖拽（高频，走 OSC） ──────────────────────────────────────────
  /// 拖拽中：改任意连续量，只更新本地 + 走 OSC，不发 JSON
  ///
  /// 曲线是后端算的，所以每拖一帧都会重取一次（一次 256 点的调用是 µs 级）。
  /// 松手时一定要调 [commitBand]，否则最后一帧可能被节流丢掉。
  void dragBandLive(
    int index, {
    double? freqHz,
    double? gainDb,
    double? q,
    double? shelfSlope,
  }) {
    _updateLocalBand(
      index,
      (band) => band.copyWith(
        freqHz: freqHz,
        gainDb: gainDb,
        q: q,
        shelfSlope: shelfSlope,
      ),
    );

    final band = _state.bands[index];
    bindings.updateEqBandLive(
      index,
      band.freqHz,
      band.gainDb,
      band.q,
      band.shelfSlope,
    );

    _refreshCurve();
    notifyListeners();
  }

  /// 滚轮调 Q / 斜率：这种离散小改动直接走 JSON 定死，不用 OSC
  void setBandShape(int index, double value) {
    _commitBandChange(index, (band) => band.withShapeValue(value));
  }

  /// 拖拽结束 / 换类型 / 开关：走 JSON 把最终值定死
  void commitBand(int index) {
    if (index < 0 || index >= _state.bands.length) return;
    _sendBand(_state.bands[index]);
  }

  /// 改一条 band 的任意字段并立即下发（走 JSON）
  void updateBand(int index, EqBandParam Function(EqBandParam band) change) {
    _commitBandChange(index, change);
  }

  void setBandEnabled(int index, bool enabled) {
    _commitBandChange(index, (band) => band.copyWith(enabled: enabled));
    // 打开一条空 band 时顺手选中它，用户马上能调
    if (enabled) _selectedBand = index;
    notifyListeners();
  }

  // ── 整机 ──────────────────────────────────────────────────────────
  void setEqEnabled(bool enabled) {
    _state = _state.copyWith(enabled: enabled);
    _sendMaster();
    notifyListeners();
  }

  /// 输出增益：拖拽中只更新本地（[commit] = false），松手时再下发
  void setMasterGain(double db, {bool commit = false}) {
    _state = _state.copyWith(masterGainDb: db);
    if (commit) _sendMaster();
    notifyListeners();
  }

  /// 恢复出厂设置（8 条全关、0dB）
  void resetAll() {
    _state = EqAllParam.defaults();
    _activePresetId = null;
    _selectedBand = null;
    _sendAll(_state);
    _refreshCurve();
    notifyListeners();
  }

  // ── 预设 ──────────────────────────────────────────────────────────
  void refreshPresets() {
    _presets = _requestPresets();
    notifyListeners();
  }

  /// 把当前状态存成新预设；成功返回新 id，失败返回 -1
  int saveCurrentAsPreset(String title) {
    final name = title.trim();
    if (name.isEmpty) return -1;

    final id = _withCString(name, (ptr) => bindings.saveEqPreset(ptr));
    if (id > 0) {
      _activePresetId = id;
      refreshPresets();
    }
    return id;
  }

  /// 应用一个预设
  bool applyPreset(int id) {
    final params = _requestPresetParams(id);
    if (params == null) return false;

    _state = params;
    _activePresetId = id;
    _selectedBand = null;
    _sendAll(_state);
    _refreshCurve();
    notifyListeners();
    return true;
  }

  void renamePreset(int id, String title) {
    final name = title.trim();
    if (name.isEmpty) return;
    _withCString(name, (ptr) {
      bindings.renameEqPreset(id, ptr);
      return true;
    });
    refreshPresets();
  }

  void deletePreset(int id) {
    bindings.deleteEqPreset(id);
    if (_activePresetId == id) _activePresetId = null;
    refreshPresets();
  }

  // ── 内部：本地状态 ────────────────────────────────────────────────
  void _updateLocalBand(
    int index,
    EqBandParam Function(EqBandParam band) change,
  ) {
    if (index < 0 || index >= _state.bands.length) return;
    _state = _state.withBand(index, change(_state.bands[index]));
  }

  void _commitBandChange(
    int index,
    EqBandParam Function(EqBandParam band) change,
  ) {
    _updateLocalBand(index, change);
    _sendBand(_state.bands[index]);
    _refreshCurve();
    notifyListeners();
  }

  void _refreshCurve() {
    final points = curvePoints.clamp(_minCurvePoints, _maxCurvePoints);
    final ptr = bindings.getEqCurve(points);
    if (ptr == nullptr) return;

    // 指针指向后端内部缓冲，下次调用会被覆盖，所以这里立刻拷走
    _curve = ptr.asTypedList(points).toList(growable: false);
  }

  // ── 内部：FFI 调用与转换 ──────────────────────────────────────────
  void _sendBand(EqBandParam band) {
    _withCString(band.type.wireName, (typePtr) {
      bindings.setEqBand(
        band.idx,
        band.enabled ? 1 : 0,
        typePtr,
        band.freqHz,
        band.gainDb,
        band.q,
        band.shelfSlope,
        band.cutSlopeDbPerOct,
      );
      return true;
    });
  }

  void _sendMaster() {
    bindings.setEqMaster(
      _state.enabled ? 1 : 0,
      _state.masterGainDb,
    );
  }

  void _sendAll(EqAllParam param) {
    _withCString(jsonEncode(_allEnvelope(param)), (ptr) {
      bindings.setEqAll(ptr);
      return true;
    });
  }

  EqAllParam? _requestState() {
    final envelope = _readJsonObject(bindings.getEqAllState());
    return _decodeAllEnvelope(envelope);
  }

  EqAllParam? _requestPresetParams(int id) {
    final envelope = _readJsonObject(bindings.getEqPresetParams(id));
    return _decodeAllEnvelope(envelope);
  }

  List<EqPresetItem> _requestPresets() {
    final envelope = _readJsonObject(bindings.getAllEqPresets());
    final list = envelope?['eqPresets'];
    if (list is! List) return const <EqPresetItem>[];

    return [
      for (final item in list)
        if (item is Map) EqPresetItem.fromJson(item.cast<String, dynamic>()),
    ];
  }

  /// 后端统一用 {"eqAll": {...}} 当信封
  Map<String, dynamic> _allEnvelope(EqAllParam param) => {
    'eqAll': param.toJson(),
  };

  EqAllParam? _decodeAllEnvelope(Map<String, dynamic>? envelope) {
    final body = envelope?['eqAll'];
    if (body is! Map) return null;
    return EqAllParam.fromJson(body.cast<String, dynamic>());
  }

  /// 传 C 字符串给后端，用完立刻释放
  T _withCString<T>(String text, T Function(Pointer<Char>) action) {
    final ptr = UtilFunction.string2cPtr(text);
    try {
      return action(ptr);
    } finally {
      malloc.free(ptr);
    }
  }

  /// 读后端返回的 C 字符串并解析成 JSON 对象
  ///
  /// 注意：`UtilFunction.cPtr2String` 内部已经调 `freeString` 释放了这份内存。
  Map<String, dynamic>? _readJsonObject(Pointer<Char> ptr) {
    if (ptr == nullptr) return null;

    final text = UtilFunction.cPtr2String(ptr);
    if (text.isEmpty) return null;

    final decoded = jsonDecode(text);
    return decoded is Map<String, dynamic> ? decoded : null;
  }
}
