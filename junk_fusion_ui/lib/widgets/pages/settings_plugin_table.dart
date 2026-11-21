/// 插件管理表格（设置页里唯一一块内容）
///
/// ── 数据从哪来 ────────────────────────────────────────────────────────
/// `PluginProvider.plugins` —— 后端 `plugins` 表的原样投影。
/// 表格自己**不做任何过滤/排序**：顺序就是后端 `ORDER BY name COLLATE NOCASE`
/// 的结果，想改顺序去改后端那条 SQL，别在前端另写一套。
///
/// ── 布局为什么是这个形状 ──────────────────────────────────────────────
/// 设置页是一整页 `SingleChildScrollView`（为了目录跳转），而"表格要自己的
/// 滚动器"意味着它不能跟着页面一起滚。所以这里给表格一个**自己算出来的固定
/// 高度**：
///     height = clamp(表头 + 行数 × 行高, 3 行, maxHeight)
/// 插件少时表格就矮（不浪费空白），插件多时到 maxHeight 封顶、内部纵向滚动。
///
/// ── 两个滚动器，为什么只有一个"平滑" ──────────────────────────────────
///   · 纵向（[_vScroll]）：`SmoothScrollController`，滚轮平滑滚行——这是用户
///     最常做的操作，必须顺滑；
///   · 横向（[_hScroll]）：普通 `ScrollController`。**刻意不用平滑的**：
///     表格外层是横向 Scrollable、里面是纵向 ListView，两套滚轮拦截叠在一起
///     时，横向那个会把手感正常的纵向滚轮抢走（变成"滚轮横向跑"）。
///     横向现在靠拖滚动条（`thumbVisibility: true` 常显）或 Shift+滚轮。
///
/// ── 列宽 ──────────────────────────────────────────────────────────────
/// 除「路径」外都是固定宽度；路径列吃掉剩余空间（最少 [_pathMinWidth]）。
/// 窗口再窄也不会挤成一团——整张表有一个最小宽度，不够就横向滚。
///
/// 用法：
///     const SettingsPluginTable()                 // 默认最大高度 420
///     SettingsPluginTable(maxHeight: 560)
library;

import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:flutter_smooth_wheel_scroll/flutter_smooth_wheel_scroll.dart';
import 'package:junk_fusion_ui/model/plugin_info.dart';
import 'package:junk_fusion_ui/providers/plugin_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/widgets/helper_widget/combo_box.dart';
import 'package:provider/provider.dart';

/// 各列宽度（路径列是弹性的，见 [_SettingsPluginTableState._widthsFor]）
class _TableWidths {
  final double name;
  final double vendor;
  final double type;
  final double category;
  final double channel;
  final double status;
  final double path;

  const _TableWidths({
    required this.name,
    required this.vendor,
    required this.type,
    required this.category,
    required this.channel,
    required this.status,
    required this.path,
  });

  double get total => name + vendor + type + category + channel + status + path;
}

class SettingsPluginTable extends StatefulWidget {
  /// 表格最多长到多高（超过就内部纵向滚动）
  final double maxHeight;

  const SettingsPluginTable({super.key, this.maxHeight = 420});

  @override
  State<SettingsPluginTable> createState() => _SettingsPluginTableState();
}

class _SettingsPluginTableState extends State<SettingsPluginTable> {
  static const double rowHeight = 40;
  static const double headerHeight = 38;

  /// 固定列宽
  static const double _nameWidth = 190;
  static const double _vendorWidth = 150;
  static const double _typeWidth = 118;
  static const double _categoryWidth = 132;
  static const double _channelWidth = 78;
  static const double _statusWidth = 86;

  /// 路径列最小宽度
  static const double _pathMinWidth = 360;

  static const double _minTableWidth =
      _nameWidth +
      _vendorWidth +
      _typeWidth +
      _categoryWidth +
      _channelWidth +
      _statusWidth +
      _pathMinWidth;

  /// 纵向：平滑滚轮
  final ScrollController _vScroll = SmoothScrollController();

  /// 横向：普通控制器（理由见文件头）
  final ScrollController _hScroll = ScrollController();

  @override
  void dispose() {
    _vScroll.dispose();
    _hScroll.dispose();
    super.dispose();
  }

  /// 按可用宽度算出这一帧的列宽：路径列吃掉剩余空间
  _TableWidths _widthsFor(double availableWidth) {
    final extra = math.max(0.0, availableWidth - _minTableWidth);
    return _TableWidths(
      name: _nameWidth,
      vendor: _vendorWidth,
      type: _typeWidth,
      category: _categoryWidth,
      channel: _channelWidth,
      status: _statusWidth,
      path: _pathMinWidth + extra,
    );
  }

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<PluginProvider>();
    final theme = context.watch<AppTheme>();
    final plugins = provider.plugins;

    // 行数决定高度：至少留 3 行的空白，最多 maxHeight
    final wanted =
        headerHeight +
        1 +
        math.max(3, plugins.isEmpty ? 3 : plugins.length) * rowHeight;
    final height = math.min(widget.maxHeight, wanted);

    return SizedBox(
      height: height,
      child: LayoutBuilder(
        builder: (context, constraints) {
          final widths = _widthsFor(constraints.maxWidth);
          // 表宽永远不小于可用宽度（否则右边会留一条难看的空隙）
          final tableWidth = math.max(widths.total, constraints.maxWidth);

          return Container(
            decoration: BoxDecoration(
              border: Border.all(color: theme.colorEdge),
              borderRadius: BorderRadius.circular(6),
            ),
            clipBehavior: Clip.antiAlias,
            child: Scrollbar(
              controller: _hScroll,
              thumbVisibility: true,
              // depth == 0 只认最外层这个横向 Scrollable，
              // 别把里面纵向 ListView 的通知也接过来
              notificationPredicate: (notification) => notification.depth == 0,
              child: SingleChildScrollView(
                controller: _hScroll,
                scrollDirection: Axis.horizontal,
                child: SizedBox(
                  width: tableWidth,
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      _buildHeader(theme, widths),
                      Container(height: 1, color: theme.colorEdge),
                      Expanded(
                        child: plugins.isEmpty
                            ? _buildEmpty(theme)
                            : _buildRows(theme, widths, plugins),
                      ),
                    ],
                  ),
                ),
              ),
            ),
          );
        },
      ),
    );
  }

  // ── 表头 ──────────────────────────────────────────────────────────
  Widget _buildHeader(AppTheme theme, _TableWidths widths) {
    final style = theme.littleTextStyle.copyWith(
      fontSize: 13,
      fontWeight: FontWeight.bold,
    );

    return Container(
      height: headerHeight,
      color: theme.colorCell,
      child: Row(
        children: [
          _headerCell('名称', widths.name, style),
          _headerCell('厂商', widths.vendor, style),
          _headerCell('类型（可改）', widths.type, style),
          _headerCell('原始分类', widths.category, style),
          _headerCell('进/出', widths.channel, style),
          _headerCell('状态', widths.status, style),
          _headerCell('路径', widths.path, style),
        ],
      ),
    );
  }

  Widget _headerCell(String text, double width, TextStyle style) {
    return SizedBox(
      width: width,
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 10),
        child: Align(
          alignment: Alignment.centerLeft,
          child: Text(
            text,
            style: style,
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
          ),
        ),
      ),
    );
  }

  // ── 行 ────────────────────────────────────────────────────────────
  Widget _buildRows(
    AppTheme theme,
    _TableWidths widths,
    List<PluginItem> plugins,
  ) {
    return Scrollbar(
      controller: _vScroll,
      notificationPredicate: (notification) => notification.depth == 0,
      child: ListView.builder(
        controller: _vScroll,
        itemExtent: rowHeight,
        itemCount: plugins.length,
        itemBuilder: (context, index) =>
            _PluginRow(item: plugins[index], widths: widths, index: index),
      ),
    );
  }

  // ── 空状态 ────────────────────────────────────────────────────────
  Widget _buildEmpty(AppTheme theme) {
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(Icons.extension_outlined, size: 34, color: theme.colorEdge),
          const SizedBox(height: 8),
          Text(
            '还没有扫描到插件\n点上方的「开始扫描」扫一遍 VST3 目录',
            textAlign: TextAlign.center,
            style: theme.littleTextStyle,
          ),
        ],
      ),
    );
  }
}

// ════════════════════════════════════════════════════════════════
// 一行插件
//
// 做成 StatefulWidget 只为一件事：鼠标悬停高亮。
// 行高由 ListView 的 itemExtent 定死，这里不要再改高度，
// 否则 40px 的格子会对不齐表头。
// ════════════════════════════════════════════════════════════════
class _PluginRow extends StatefulWidget {
  final PluginItem item;
  final _TableWidths widths;
  final int index;

  const _PluginRow({
    required this.item,
    required this.widths,
    required this.index,
  });

  @override
  State<_PluginRow> createState() => _PluginRowState();
}

class _PluginRowState extends State<_PluginRow> {
  bool _hovered = false;

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final item = widget.item;
    final widths = widget.widths;

    // 隔行淡染 + 悬停加深：表格感全靠这两笔，不加边框会糊成一片
    final background = _hovered
        ? theme.colorHover
        : (widget.index.isOdd
              ? theme.colorCell.withValues(alpha: 0.35)
              : Colors.transparent);

    return MouseRegion(
      onEnter: (_) => setState(() => _hovered = true),
      onExit: (_) => setState(() => _hovered = false),
      child: Container(
        color: background,
        child: Row(
          children: [
            _textCell(
              item.name,
              widths.name,
              theme.midTextStyle.copyWith(
                fontSize: 14,
                fontWeight: FontWeight.w600,
              ),
              tooltip: item.name,
            ),
            _textCell(item.vendor, widths.vendor, theme.littleTextStyle),
            _typeCell(theme, item, widths.type),
            _textCell(item.category, widths.category, theme.littleTextStyle),
            _textCell(
              item.isFailed ? '—' : item.channelText,
              widths.channel,
              theme.littleTextStyle,
            ),
            _statusCell(theme, item, widths.status),
            _textCell(
              item.filePath,
              widths.path,
              theme.littleTextStyle.copyWith(fontSize: 12),
              tooltip: item.filePath,
            ),
          ],
        ),
      ),
    );
  }

  /// 普通文本格：单行、超长省略、悬停看全文
  Widget _textCell(
    String text,
    double width,
    TextStyle style, {
    String? tooltip,
  }) {
    final content = SizedBox(
      width: width,
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 10),
        child: Align(
          alignment: Alignment.centerLeft,
          child: Text(
            text,
            style: style,
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
          ),
        ),
      ),
    );

    if (tooltip == null || tooltip.isEmpty) return content;
    return Tooltip(
      message: tooltip,
      waitDuration: const Duration(milliseconds: 600),
      child: content,
    );
  }

  /// 类型格：可下拉改（CustomComboBox，项目规范不让用原生 DropdownButton）
  Widget _typeCell(AppTheme theme, PluginItem item, double width) {
    return SizedBox(
      width: width,
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 6),
        child: Align(
          alignment: Alignment.centerLeft,
          child: CustomComboBox<PluginType>(
            items: PluginType.values,
            value: item.type,
            itemLabel: (type) => type.displayName,
            width: width - 12,
            fontSize: 13,
            contentPadding: const EdgeInsets.symmetric(
              horizontal: 8,
              vertical: 6,
            ),
            // 扫描失败的行没有 identifier，后端定位不到，直接禁用
            enabled: item.canEditType,
            onChanged: (type) {
              if (type == null) return;
              _changeType(context, item, type);
            },
          ),
        ),
      ),
    );
  }

  /// 状态格：正常 / 失败（失败悬停看原因）
  Widget _statusCell(AppTheme theme, PluginItem item, double width) {
    if (item.isFailed) {
      return _textCell(
        '失败',
        width,
        theme.littleTextStyle.copyWith(
          color: theme.colorError,
          fontWeight: FontWeight.w600,
        ),
        tooltip: item.scanError.isEmpty ? '扫描失败' : item.scanError,
      );
    }

    return _textCell(
      '正常',
      width,
      theme.littleTextStyle.copyWith(color: theme.colorTextSecond),
    );
  }

  /// 改类型：乐观更新在后端失败时回滚，这里只负责把原因告诉用户
  void _changeType(BuildContext context, PluginItem item, PluginType type) {
    final provider = context.read<PluginProvider>();
    if (provider.setType(item, type)) return;

    final error = provider.lastError;
    provider.clearError();
    ScaffoldMessenger.maybeOf(
      context,
    )?.showSnackBar(SnackBar(content: Text(error ?? '改类型失败')));
  }
}
