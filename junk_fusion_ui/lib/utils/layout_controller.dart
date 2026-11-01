import 'package:flutter/foundation.dart';
import 'package:junk_fusion_ui/utils/app_cache.dart';

// ════════════════════════════════════════════════════════════════
// layout_controller.dart — 三段式布局的宽度状态
//
// 管三件事：
//   1. 中间栏现在多宽（用户拖出来 / 上次退出时存下来的）；
//   2. 在当前窗口宽度下这个宽度最多能到多少（窗口变小要自动收口，
//      不能把右侧主内容挤没）；
//   3. 拖拽结束把宽度写进 SharedPreferences，下次启动原样还原。
//
// 用法（只有一个地方 new 它，见 widgets/layout/three_column_layout.dart）：
//
//     final layout = LayoutController();          // 自己从缓存里读初值
//     SizedBox(width: layout.effectiveWidth(可用总宽度))       // 画
//     layout.resizeBy(拖拽增量, 可用总宽度)                     // 拖
//     layout.persist()                                       // 拖完落盘
//
// 注意：不要拖一帧就 persist 一次，SharedPreferences 是磁盘写，拖拽过程中
//       只在 onHorizontalDragEnd 调一次（ThreeColumnLayout 里已经这么做）。
//
// 宽度是全局一份，不按「段」分开记 —— 和 VS Code 一样，拖一次所有栏都变。
// ════════════════════════════════════════════════════════════════

/// 三段式布局的宽度状态
class LayoutController extends ChangeNotifier {
  /// 左侧图标栏宽度（固定，不参与拖拽）
  static const double railWidth = 56;

  /// 中间栏最小宽度（再窄就放不下标题和列表了）
  static const double sidebarMinWidth = 180;

  /// 中间栏最大宽度
  static const double sidebarMaxWidth = 480;

  /// 中间栏默认宽度（第一次启动、缓存里没值时用）
  static const double sidebarDefaultWidth = 240;

  /// 窗口宽度里中间栏最多占多少 —— 防止用户把主内容挤成一条缝
  static const double sidebarMaxRatio = 0.45;

  /// SharedPreferences 里存宽度的键
  static const String sidebarWidthCacheKey = 'layoutSidebarWidth';

  double _sidebarWidth;

  /// [initialWidth] 只给测试用；正常情况下从 SharedPreferences 读上次的宽度
  LayoutController({double? initialWidth})
    : _sidebarWidth = initialWidth ?? _readCachedWidth();

  static double _readCachedWidth() {
    final cached = AppCache.frontCacheRef.getDouble(sidebarWidthCacheKey);
    if (cached == null) return sidebarDefaultWidth;
    // 上次存的宽度可能比这版允许的范围大（改过上限），读进来先夹一次
    return cached.clamp(sidebarMinWidth, sidebarMaxWidth).toDouble();
  }

  /// 用户拖出来的原始宽度
  ///
  /// 注意这个值可能超出**当前窗口**允许的范围（窗口可以被缩小），
  /// 真正画的时候请用 [effectiveWidth]
  double get sidebarWidth => _sidebarWidth;

  /// 当前窗口宽度下，中间栏最多能有多宽
  ///
  /// [availableWidth] 是三段式那一行拿到的总宽度（含图标栏和分隔线）
  static double maxWidthFor(double availableWidth) {
    final ratioLimit = availableWidth * sidebarMaxRatio;
    final maxWidth = ratioLimit < sidebarMaxWidth ? ratioLimit : sidebarMaxWidth;
    // 极端窄窗口下算出来的 max 会比 min 还小，clamp 会抛 ArgumentError，
    // 这里兜底成 min（当窗口真的窄到这一步时，主内容本来也没什么空间了）
    return maxWidth < sidebarMinWidth ? sidebarMinWidth : maxWidth;
  }

  /// 当前窗口宽度下，中间栏实际画多宽（窗口缩小后自动收口）
  double effectiveWidth(double availableWidth) {
    return _sidebarWidth
        .clamp(sidebarMinWidth, maxWidthFor(availableWidth))
        .toDouble();
  }

  /// 拖拽：在当前有效宽度上加减 [delta]（像素）
  ///
  /// 从**有效宽度**算起而不是从 [_sidebarWidth] 算起：
  /// 窗口被缩到很窄、存着的宽度超出上限时，如果从原始值算，用户往回拖
  /// 会有一段"拖了没反应"的死区
  void resizeBy(double delta, double availableWidth) {
    final maxWidth = maxWidthFor(availableWidth);
    final current = effectiveWidth(availableWidth);
    final next = (current + delta)
        .clamp(sidebarMinWidth, maxWidth)
        .toDouble();
    if (next == current) return;

    _sidebarWidth = next;
    notifyListeners();
  }

  /// 落盘（只在拖拽结束时调一次）
  void persist() {
    AppCache.frontCacheRef.setDouble(sidebarWidthCacheKey, _sidebarWidth);
  }
}
