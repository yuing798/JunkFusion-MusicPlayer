/// ════════════════════════════════════════════════════════════════
/// song_detail_info.dart — 歌曲详情面板
///
/// 对应原 Vue 项目 components/cell/songDetailInfo.vue
///
/// 布局：
///   左侧列：240×240 封面 + "联网自动获取补全元数据" shiny-btn
///   右侧列：主信息 + 技术参数 grid + 音乐参数 grid + 备注文本框
///
/// Dart 语法说明：
/// - `TextEditingController` 控制 TextField 的内容
/// - `FocusNode` 监听焦点的获取和丢失
/// - `addListener` 注册回调
/// - `dispose()` 释放资源（Controller/FocusNode 必须在 dispose 中释放）
/// - `SingleChildScrollView` 可滚动区域的容器
/// ════════════════════════════════════════════════════════════════

import 'package:flutter/material.dart';
import '../models/song_info.dart';
import '../theme/app_theme.dart';

/// SongDetailInfo — 歌曲详情面板（在弹窗中使用）
///
/// 参数：
/// - `song`：要显示详情的歌曲对象
class SongDetailInfo extends StatefulWidget {
  final SongInfo song;

  const SongDetailInfo({super.key, required this.song});

  @override
  State<SongDetailInfo> createState() => _SongDetailInfoState();
}

class _SongDetailInfoState extends State<SongDetailInfo> {
  /// 备注文本控制器
  late TextEditingController _remarkController;

  /// 焦点节点（用于检测失焦 → 保存）
  final FocusNode _remarkFocusNode = FocusNode();

  @override
  void initState() {
    super.initState();

    // 初始化备注文本（现有备注或空字符串）
    _remarkController = TextEditingController(text: widget.song.comment ?? '');

    // `addListener` 注册监听回调
    // 当焦点丢失时自动保存备注
    _remarkFocusNode.addListener(_onFocusChange);
  }

  @override
  void dispose() {
    // 必须释放 TextEditingController 和 FocusNode！
    // 否则会造成内存泄漏
    _remarkController.dispose();
    _remarkFocusNode.removeListener(_onFocusChange);
    _remarkFocusNode.dispose();
    super.dispose();
  }

  /// 焦点变化回调
  void _onFocusChange() {
    if (!_remarkFocusNode.hasFocus) {
      // 失焦 → 保存备注
      _saveComment();
    }
  }

  /// 保存备注
  ///
  /// 对应原 Vue: async function saveComment()
  void _saveComment() {
    // TODO: 桥接层 - 调用原生函数保存备注
    // 原 Vue 代码:
    //   s.comment = remarkText.value;
    //   await getNativeFunction(B_saveComment.name)(
    //     { text: remarkText.value, songId: s.songId }
    //   );
  }

  @override
  Widget build(BuildContext context) {
    final s = widget.song;

    // `SingleChildScrollView` + `IntrinsicHeight` 保证内容可滚动
    return SizedBox(
      width: 900,
      child: Padding(
        padding: const EdgeInsets.all(10),
        child: Row(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // ── 左侧列：封面 + shiny-btn ──
            _buildLeftColumn(s),

            const SizedBox(width: 10),

            // ── 右侧列：详细信息 ──
            Expanded(child: _buildRightColumn(s)),
          ],
        ),
      ),
    );
  }

  /// 构建左侧列（封面 + 按钮）
  Widget _buildLeftColumn(SongInfo s) {
    return Column(
      children: [
        // 封面图片 240×240
        // TODO: 桥接层 - 加载后端资源图片
        // 原 Vue: <img :src="getBackendResourceAddress(`songId/${s.songId}/image/240x240`)" />
        Container(
          width: 240,
          height: 240,
          color: AppTheme.colorEdge,
          child: const Icon(Icons.album, size: 100),
        ),

        const SizedBox(height: 10),

        // ShinyBtn — "联网自动获取补全元数据"
        // 对应原 Vue: <button class="shiny-btn">联网自动获取补全元数据</button>
        GestureDetector(
          onTap: () {
            // TODO: 实现联网获取元数据功能
          },
          child: Container(
            padding: const EdgeInsets.symmetric(horizontal: 36, vertical: 14),
            decoration: AppTheme.shinyBtnDecoration(),
            child: const Text(
              '联网自动获取补全元数据',
              style: TextStyle(
                fontSize: 18,
                fontWeight: FontWeight.w700,
                color: AppTheme.colorTextMain,
              ),
            ),
          ),
        ),
      ],
    );
  }

  /// 构建右侧列（详情信息）
  Widget _buildRightColumn(SongInfo s) {
    return SingleChildScrollView(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // --- 主信息：歌名、艺术家、专辑 ---
          _buildMainInfo(s),

          // --- 技术参数 grid ---
          _buildMetaGrid(_buildTechInfoList(s)),

          // --- 音乐参数 grid ---
          _buildMetaGrid(_buildMusicInfoList(s)),

          // --- 备注文本框 ---
          _buildRemarkArea(),
        ],
      ),
    );
  }

  /// 构建主信息区域
  Widget _buildMainInfo(SongInfo s) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 8),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(s.title, style: const TextStyle(fontSize: 21)),
          if (s.artist != null) Text(s.artist!),
          if (s.album != null) Text(s.album!),
        ],
      ),
    );
  }

  /// 构建技术参数列表
  ///
  /// 对应原 Vue techInfoArray
  List<({String label, String value})> _buildTechInfoList(SongInfo s) {
    return [
      (label: '时长', value: SongInfo.formatDuration(s.duration)),
      (label: '采样率', value: '${s.sampleRate}Hz'),
      (label: '比特率', value: '${s.bitRate}Kbps'),
      (label: '通道布局', value: s.channelLayout),
      (label: '位深', value: '${s.bitDepth}'),
      if (s.codecName != null) (label: '解码器', value: s.codecName!),
    ];
  }

  /// 构建音乐参数列表
  ///
  /// 对应原 Vue musicInfoArray
  List<({String label, String value})> _buildMusicInfoList(SongInfo s) {
    // Dart 的 records（记录类型）：`(label: 'BPM', value: '120')`
    // 括号 + 命名字段 = record，类似于轻量级匿名结构体
    return [
      (label: '播放次数', value: '${s.playNum}'),
      if (s.bpm != null) (label: 'BPM', value: '${s.bpm}'),
      if (s.key != null) (label: '调性', value: s.key!),
      if (s.genre != null) (label: '体裁(原始)', value: s.genre!),
      if (s.aiGenre != null) (label: '体裁(AI分析)', value: s.aiGenre!),
      if (s.trackNumber != null) (label: '轨道号', value: '${s.trackNumber}'),
      if (s.discNumber != null) (label: '碟片号', value: '${s.discNumber}'),
      if (s.year != null) (label: '发行年份', value: '${s.year}'),
      if (s.composer != null) (label: '作曲家', value: s.composer!),
      if (s.albumArtist != null) (label: '专辑艺术家', value: s.albumArtist!),
    ];
  }

  /// 构建元数据 grid（3 列等宽）
  ///
  /// 对应原 Vue .meta-grid: display: grid; grid-template-columns: repeat(3, 1fr);
  Widget _buildMetaGrid(List<({String label, String value})> items) {
    if (items.isEmpty) return const SizedBox.shrink();

    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // 分组（每 3 个一行）
          for (int i = 0; i < items.length; i += 3)
            Row(
              children: [
                for (int j = i; j < i + 3 && j < items.length; j++)
                  Expanded(
                    child: _buildLabelValue(items[j].label, items[j].value),
                  ),
                // 填充空位（保持 3 列对齐）
                for (int j = items.length - i; j < 3; j++)
                  const Expanded(child: SizedBox()),
              ],
            ),
        ],
      ),
    );
  }

  /// 构建单个标签-值对
  ///
  /// 对应原 Vue .label-value
  Widget _buildLabelValue(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 3),
      child: Row(
        children: [
          Text(
            '$label ',
            style: AppTheme.midTextStyle.copyWith(
              color: AppTheme.colorTextSecond,
            ),
          ),
          Flexible(
            child: Text(
              value,
              style: AppTheme.midTextStyle,
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
            ),
          ),
        ],
      ),
    );
  }

  /// 构建备注文本框
  ///
  /// 对应原 Vue: <textarea class="remark-area" v-model="remarkText"
  ///                        placeholder="点击输入备注" @blur="saveComment()">
  Widget _buildRemarkArea() {
    return TextField(
      controller: _remarkController,
      focusNode: _remarkFocusNode,
      // `maxLines: null` 允许无限行（+ expands 填满）
      maxLines: 4,
      minLines: 4,
      decoration: InputDecoration(
        hintText: '点击输入备注',
        hintStyle: AppTheme.littleTextStyle,
        border: OutlineInputBorder(
          borderRadius: BorderRadius.circular(AppTheme.borderRadius),
          borderSide: BorderSide.none,
        ),
        focusedBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(AppTheme.borderRadius),
          borderSide: const BorderSide(color: AppTheme.colorEdge),
        ),
        contentPadding: const EdgeInsets.all(8),
        isDense: true,
      ),
      style: AppTheme.littleTextStyle,
    );
  }
}

/// ════════════════════════════════════════════════════════════════
/// Dart Records（记录类型）：
///
/// Dart 3 引入的 records 是轻量级的匿名复合类型：
/// ```dart
/// final item = (label: 'BPM', value: '120');
/// print(item.label); // BPM
/// print(item.value); // 120
/// ```
///
/// 用途：替代"只在一个地方使用的小型数据结构"，
/// 避免为每个临时数据组合新建 class。
/// 对应 TypeScript 的:
///   const item: { label: string; value: string } = { label: 'BPM', value: '120' };
/// ════════════════════════════════════════════════════════════════
