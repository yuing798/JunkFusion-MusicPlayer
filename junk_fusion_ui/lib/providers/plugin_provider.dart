/// 插件库页的数据层（Provider）
///
/// ── 职责 ──────────────────────────────────────────────────────────────
/// 1. 持有 `plugins` 表的**前端副本**（真正权威的那份在 SQLite 里）；
/// 2. 触发后台扫描并轮询进度（扫描在 Coordinator 的独立线程里跑，
///    子进程还会被坏插件带走，所以前端只能"点开始 + 轮询状态"）；
/// 3. 改 type：先本地乐观更新再调后端，失败就回滚 + 把原因留给页面弹提示。
///
/// ── 为什么要轮询而不是回调 ────────────────────────────────────────────
/// 后端扫描完成的时机完全不可预测（可能 1 秒，也可能某个插件卡 15 秒超时），
/// 现在 FFI 还没有"扫描结束"的回调通道。轮询只在扫描期间开，
/// 扫完立刻停，代价可以忽略；等以后要做实时进度条再加回调。
///
/// ── 用法 ─────────────────────────────────────────────────────────────
/// ```dart
/// context.read<PluginProvider>().ensureInitialized();  // 进页面时一次
/// context.watch<PluginProvider>();                     // 画 UI
/// provider.startScan();                                // 点「开始扫描」
/// provider.setType(item, PluginType.effect);           // 改类型
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

class PluginProvider extends ChangeNotifier {
  /// 扫描期间轮询后端的间隔
  ///
  /// 后端自己的子进程轮询是 50ms 一次，前端只是画"5/12"这种粗进度，
  /// 400ms 完全够，而且不会给 DLL 制造无谓的 JSON 序列化压力。
  static const Duration _pollInterval = Duration(milliseconds: 400);

  List<PluginItem> _plugins = const <PluginItem>[];
  List<PluginDirItem> _dirs = const <PluginDirItem>[];
  PluginScanState _scan = const PluginScanState.idle();

  bool _initialized = false;
  Timer? _pollTimer;

  /// 最近一次操作失败的原因（给页面弹 SnackBar 用；读完请调 [clearError]）
  String? _lastError;

  // ── 只读状态 ──────────────────────────────────────────────────────
  List<PluginItem> get plugins => _plugins;
  List<PluginDirItem> get dirs => _dirs;
  PluginScanState get scan => _scan;
  String? get lastError => _lastError;

  bool get isScanning => _scan.isRunning;

  /// 各类型的数量，给表头上面的汇总行用
  int countOf(PluginType type) =>
      _plugins.where((item) => item.type == type && !item.isFailed).length;

  int get failedCount => _plugins.where((item) => item.isFailed).length;

  // ── 初始化 ────────────────────────────────────────────────────────
  /// 进设置页时调一次：拉插件列表 + 目录，顺便接上可能已经在跑的扫描
  void ensureInitialized() {
    if (_initialized) return;
    _initialized = true;

    refresh();
    refreshDirs();
    refreshPolicy();
    _refreshScanState();
    if (_scan.isRunning) _startPolling();
  }

  void clearError() {
    _lastError = null;
  }

  // ── 读 ────────────────────────────────────────────────────────────
  void refresh() {
    final envelope = _readJsonObject(bindings.getAllPluginsView());
    final raw = envelope?['plugins'];
    if (raw is! List) {
      _plugins = const <PluginItem>[];
    } else {
      _plugins = <PluginItem>[
        for (final item in raw)
          if (item is Map) PluginItem.fromJson(item.cast<String, dynamic>()),
      ];
    }
    notifyListeners();
  }

  void refreshDirs() {
    final envelope = _readJsonObject(bindings.getPluginDirs());
    final raw = envelope?['dirs'];
    if (raw is! List) {
      _dirs = const <PluginDirItem>[];
    } else {
      _dirs = <PluginDirItem>[
        for (final item in raw)
          if (item is Map) PluginDirItem.fromJson(item.cast<String, dynamic>()),
      ];
    }
    notifyListeners();
  }

  // ── 扫描目录 ──────────────────────────────────────────────────────
  /// 加一个自定义扫描目录；失败时把原因留在 [lastError]
  ///
  /// 系统默认目录（`C:\Program Files\Common Files\VST3`）是后端启动时自己插的，
  /// 用户只能增删自己的目录。
  bool addDir(String path) {
    final trimmed = path.trim();
    if (trimmed.isEmpty) return false;

    final result = _withCString(
      trimmed,
      (ptr) => _readJsonObject(bindings.addPluginDir(ptr)),
    );

    if (result?['ok'] == true) {
      refreshDirs();
      return true;
    }

    _lastError = result?['error'] is String
        ? result!['error'] as String
        : '添加扫描目录失败';
    notifyListeners();
    return false;
  }

  /// 删一个扫描目录；系统默认目录会被后端拒绝（原因在 [lastError]）
  bool removeDir(String path) {
    if (path.isEmpty) return false;

    final result = _withCString(
      path,
      (ptr) => _readJsonObject(bindings.removePluginDir(ptr)),
    );

    if (result?['ok'] == true) {
      refreshDirs();
      return true;
    }

    _lastError = result?['error'] is String
        ? result!['error'] as String
        : '删除扫描目录失败';
    notifyListeners();
    return false;
  }

  // ── 黑白名单（host-integration.md §6）────────────────────────────
  List<PluginBanItem> _blacklist = const <PluginBanItem>[];
  List<PluginAllowItem> _whitelist = const <PluginAllowItem>[];

  List<PluginBanItem> get blacklist => _blacklist;
  List<PluginAllowItem> get whitelist => _whitelist;

  /// 拉一次黑名单 + 白名单（设置页展示用，**全量显示不隐藏**）
  void refreshPolicy() {
    final envelope = _readJsonObject(bindings.getPluginPolicyView());

    final rawBlack = envelope?['blacklist'];
    _blacklist = rawBlack is List
        ? <PluginBanItem>[
            for (final item in rawBlack)
              if (item is Map)
                PluginBanItem.fromJson(item.cast<String, dynamic>()),
          ]
        : const <PluginBanItem>[];

    final rawWhite = envelope?['whitelist'];
    _whitelist = rawWhite is List
        ? <PluginAllowItem>[
            for (final item in rawWhite)
              if (item is Map)
                PluginAllowItem.fromJson(item.cast<String, dynamic>()),
          ]
        : const <PluginAllowItem>[];

    notifyListeners();
  }

  bool isWhitelisted(String identifier) =>
      _whitelist.any((item) => item.identifier == identifier);

  /// 强制允许某个插件（白名单优先于黑名单）
  bool addWhitelist(String identifier, String filePath) {
    if (identifier.isEmpty) return false;

    final ok = _callTwoStrings(
      identifier,
      filePath,
      (idPtr, pathPtr) => bindings.addPluginWhitelist(idPtr, pathPtr),
    );
    refreshPolicy();
    return ok;
  }

  bool removeWhitelist(String identifier) {
    final ok = _callOneString(
      identifier,
      (ptr) => bindings.removePluginWhitelist(ptr),
    );
    refreshPolicy();
    return ok;
  }

  /// 解除黑名单（"重试"）
  bool removeBlacklist(String identifier) {
    final ok = _callOneString(
      identifier,
      (ptr) => bindings.removePluginBlacklist(ptr),
    );
    refreshPolicy();
    return ok;
  }

  bool _callOneString(
    String text,
    Pointer<Char> Function(Pointer<Char>) action,
  ) {
    final result = _withCString(text, (ptr) => _readJsonObject(action(ptr)));
    return result?['ok'] == true;
  }

  bool _callTwoStrings(
    String first,
    String second,
    Pointer<Char> Function(Pointer<Char>, Pointer<Char>) action,
  ) {
    final result = _withCString(first, (firstPtr) {
      return _withCString(second, (secondPtr) {
        return _readJsonObject(action(firstPtr, secondPtr));
      });
    });
    return result?['ok'] == true;
  }

  // ── 扫描 ──────────────────────────────────────────────────────────
  /// 开始扫描；返回 false 表示没启动（已经在扫 / 后端没初始化）
  bool startScan() {
    if (_scan.isRunning) return false;

    final result = _readJsonObject(bindings.startPluginScan());
    final started = result?['started'] == true;

    if (!started) {
      _lastError = result?['error'] is String
          ? result!['error'] as String
          : '扫描没能启动';
      // 后端说"已经在扫"时把本地状态对齐过去，别让按钮一直停在可点状态
      _refreshScanState();
      if (_scan.isRunning) _startPolling();
      notifyListeners();
      return false;
    }

    _scan = const PluginScanState(
      state: 'running',
      total: 0,
      processed: 0,
      failed: 0,
      message: '',
    );
    _startPolling();
    notifyListeners();
    return true;
  }

  void _startPolling() {
    _pollTimer?.cancel();
    _pollTimer = Timer.periodic(_pollInterval, (_) {
      _refreshScanState();
      if (!_scan.isRunning) {
        _pollTimer?.cancel();
        _pollTimer = null;
        // 扫描结束后端已经写库了，这里必须重拉一次才看得到新行
        refresh();
      } else {
        notifyListeners();
      }
    });
  }

  void _refreshScanState() {
    final envelope = _readJsonObject(bindings.getPluginScanState());
    final raw = envelope?['scan'];
    _scan = raw is Map
        ? PluginScanState.fromJson(raw.cast<String, dynamic>())
        : const PluginScanState.idle();
  }

  // ── 改类型 ────────────────────────────────────────────────────────
  /// 把某个插件改成 [newType]
  ///
  /// 先本地改（界面立刻反应），后端失败就回滚。
  /// 失败行（没有 identifier）直接拒绝——后端是按 identifier 定位的。
  bool setType(PluginItem item, PluginType newType) {
    if (!item.canEditType) {
      _lastError = '这条扫描失败，没有插件标识，改不了类型';
      notifyListeners();
      return false;
    }
    if (item.type == newType) return true;

    _applyLocalType(item.identifier, newType);
    notifyListeners();

    final result = _withCString(item.identifier, (identifierPtr) {
      return _withCString(newType.wireName, (typePtr) {
        return _readJsonObject(bindings.setPluginType(identifierPtr, typePtr));
      });
    });

    if (result?['ok'] == true) return true;

    // 回滚
    _applyLocalType(item.identifier, item.type);
    _lastError = result?['error'] is String
        ? result!['error'] as String
        : '改类型失败';
    notifyListeners();
    return false;
  }

  void _applyLocalType(String identifier, PluginType type) {
    _plugins = <PluginItem>[
      for (final item in _plugins)
        if (item.identifier == identifier) item.copyWithType(type) else item,
    ];
  }

  // ── 析构 ──────────────────────────────────────────────────────────
  @override
  void dispose() {
    _pollTimer?.cancel();
    _pollTimer = null;
    super.dispose();
  }

  // ── 内部：FFI 字符串转换 ──────────────────────────────────────────
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
  /// `UtilFunction.cPtr2String` 内部已经调 `freeString` 释放了这份内存。
  /// 解析失败返回 null 而不是抛：FFI 边界上的脏数据不该让整个设置页崩掉。
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
