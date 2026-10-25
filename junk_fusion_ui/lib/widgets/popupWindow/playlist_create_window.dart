import 'dart:io';

import 'package:desktop_drop/desktop_drop.dart';
import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:provider/provider.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

// ════════════════════════════════════════════════════════════════
// playlist_create_window.dart — 新建歌单弹窗
//
// 用 DialogUtil.showPlaylistCreateDialog() 打开，走 showDialog，
// 弹窗自己不碰后端：点「确定」就把用户填的东西 pop 出去
// （PlaylistCreateResult），谁打开的谁负责拿去建歌单。
//
// 内容：
//   歌单名称  —— 文本输入框，回车 = 确定，空名字不允许提交
//   封面图片  —— 可选。两种给图方式：
//                1. 点一下那个方框 → 打开系统文件选择框（file_picker）
//                2. 直接把本地图片拖到方框里（desktop_drop 的 DropTarget）
//   底部       —— 取消 / 确定
//
// 注意：弹窗只负责「拿到标题 + 图片路径」，图片要不要拷进缓存目录、
//       缩成多大，这些是后端/以后的封面逻辑干的事（见 playlist_view_page
//       里的 TODO）。
// ════════════════════════════════════════════════════════════════

/// 弹窗点「确定」之后带出去的结果
class PlaylistCreateResult {
  /// 歌单名称（已经 trim 过，保证不是空串）
  final String title;

  /// 用户选的封面图片的本地路径；没选就是 null
  final String? imagePath;

  const PlaylistCreateResult({required this.title, this.imagePath});
}

class PlaylistCreateWindow extends StatefulWidget {
  const PlaylistCreateWindow({super.key});

  @override
  State<PlaylistCreateWindow> createState() => _PlaylistCreateWindowState();
}

class _PlaylistCreateWindowState extends State<PlaylistCreateWindow> {
  /// 歌单名称输入框
  final TextEditingController _titleController = TextEditingController();

  /// 选中的封面图片路径（null = 还没选）
  String? _imagePath;

  /// 是否有文件正拖在上面的方框里（用来高亮）
  bool _isDragOver = false;

  /// 校验 / 读图的错误提示
  String? _errorText;

  @override
  void dispose() {
    _titleController.dispose();
    super.dispose();
  }

  // ────────────────────────────────────────────────────────────
  // 选图
  // ────────────────────────────────────────────────────────────

  /// 打开系统文件选择框挑一张图
  Future<void> _pickImage() async {
    final result = await FilePicker.pickFiles(
      dialogTitle: '选择歌单封面图片',
      type: FileType.image,
      allowMultiple: false,
      lockParentWindow: true,
    );

    // 用户点了取消
    final path = result?.files.firstOrNull?.path;
    if (path == null || path.isEmpty) return;

    if (!mounted) return;
    setState(() {
      _imagePath = path;
      _errorText = null;
    });
  }

  /// 拖着文件松手：只认第一个，而且得像个图片
  void _onDropDone(DropDoneDetails details) {
    setState(() => _isDragOver = false);

    if (details.files.isEmpty) return;
    final path = details.files.first.path;

    if (!_looksLikeImage(path)) {
      setState(() {
        _errorText = '这个文件看着不是图片（支持 png / jpg / jpeg / webp / bmp / gif）';
      });
      return;
    }

    setState(() {
      _imagePath = path;
      _errorText = null;
    });
  }

  /// 按扩展名粗筛一下（真正的解码错误由 Image.file 的 errorBuilder 兜底）
  static bool _looksLikeImage(String path) {
    final lower = path.toLowerCase();
    for (final extension in const [
      '.png',
      '.jpg',
      '.jpeg',
      '.webp',
      '.bmp',
      '.gif',
    ]) {
      if (lower.endsWith(extension)) return true;
    }
    return false;
  }

  // ────────────────────────────────────────────────────────────
  // 确定 / 取消
  // ────────────────────────────────────────────────────────────

  void _confirm() {
    final title = _titleController.text.trim();
    if (title.isEmpty) {
      setState(() => _errorText = '歌单名称不能为空');
      return;
    }

    Navigator.of(context).pop(
      PlaylistCreateResult(title: title, imagePath: _imagePath),
    );
  }

  void _cancel() {
    Navigator.of(context).pop();
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();

    return Dialog(
      backgroundColor: theme.colorCell,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(6)),
      child: SizedBox(
        width: 480,
        child: Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              // ── 标题行 ──
              Row(
                children: [
                  Text('新建歌单', style: theme.bigTextStyle),
                  const Spacer(),
                  IconButton(
                    icon: const Icon(TablerIcons.x, size: 24),
                    tooltip: '取消',
                    color: theme.colorTextMain,
                    onPressed: _cancel,
                  ),
                ],
              ),

              const SizedBox(height: 6),

              // ── 歌单名称 ──
              Text('歌单名称', style: theme.midTextStyle),
              const SizedBox(height: 6),
              TextField(
                controller: _titleController,
                autofocus: true,
                style: theme.midTextStyle,
                // 回车 = 确定（省得每次都要去点按钮）
                onSubmitted: (_) => _confirm(),
                onChanged: (_) {
                  // 名字一变动就把「不能为空」的提示撤掉
                  if (_errorText != null) {
                    setState(() => _errorText = null);
                  }
                },
                decoration: InputDecoration(
                  hintText: '给歌单起个名字',
                  hintStyle: theme.littleTextStyle.copyWith(
                    color: theme.colorTextSecond.withAlpha(120),
                  ),
                  isDense: true,
                  contentPadding: const EdgeInsets.symmetric(
                    horizontal: 10,
                    vertical: 10,
                  ),
                  border: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(4),
                    borderSide: BorderSide(color: theme.colorEdge, width: 2),
                  ),
                  enabledBorder: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(4),
                    borderSide: BorderSide(color: theme.colorEdge, width: 2),
                  ),
                  focusedBorder: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(4),
                    borderSide: BorderSide(color: theme.colorFocus, width: 2),
                  ),
                ),
              ),

              const SizedBox(height: 16),

              // ── 封面图片 ──
              Text('封面图片（可选）', style: theme.midTextStyle),
              const SizedBox(height: 6),
              _buildImagePicker(theme),

              // ── 错误提示 ──
              if (_errorText != null) ...[
                const SizedBox(height: 8),
                Text(
                  _errorText!,
                  style: theme.littleTextStyle.copyWith(
                    color: theme.colorError,
                  ),
                ),
              ],

              const SizedBox(height: 18),

              // ── 底部：取消 / 确定 ──
              Row(
                mainAxisAlignment: MainAxisAlignment.end,
                children: [
                  TextButton(
                    onPressed: _cancel,
                    child: Text('取消', style: theme.midTextStyle),
                  ),
                  const SizedBox(width: 8),
                  ElevatedButton(
                    onPressed: _confirm,
                    style: ElevatedButton.styleFrom(
                      backgroundColor: theme.colorStress,
                      foregroundColor: theme.colorTextMain,
                      elevation: 0,
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(4), // 直角一点，和全站一致
                      ),
                      padding: const EdgeInsets.symmetric(
                        horizontal: 20,
                        vertical: 12,
                      ),
                    ),
                    child: Text('确定', style: theme.midTextStyle),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }

  // ────────────────────────────────────────────────────────────
  // 封面选择区
  //
  // 外层是 DropTarget：本地图片拖进来 → onDragDone；
  // 内层是 GestureDetector：点一下 → 打开文件选择框。
  // 两种方式拿到的东西是同一个（本地路径），存到 _imagePath。
  // ────────────────────────────────────────────────────────────
  Widget _buildImagePicker(AppTheme theme) {
    final imagePath = _imagePath;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        DropTarget(
          // 拖进来的时候高亮一下，告诉用户「松手就放这儿」
          onDragEntered: (_) => setState(() => _isDragOver = true),
          onDragExited: (_) => setState(() => _isDragOver = false),
          onDragDone: _onDropDone,
          child: GestureDetector(
            onTap: _pickImage,
            child: MouseRegion(
              cursor: SystemMouseCursors.click,
              child: Container(
                height: 180,
                decoration: BoxDecoration(
                  color: _isDragOver ? theme.colorHover : theme.colorMain,
                  borderRadius: BorderRadius.circular(4),
                  border: Border.all(
                    color: _isDragOver ? theme.colorStress : theme.colorEdge,
                    width: 2,
                  ),
                ),
                // 圆角要裁一下，不然图片会盖住边框
                clipBehavior: Clip.antiAlias,
                child: imagePath == null
                    ? _buildImagePlaceholder(theme)
                    : Image.file(
                        File(imagePath),
                        fit: BoxFit.cover,
                        // 文件被删了 / 根本不是图片 → 别让界面炸成红框
                        errorBuilder: (_, _, _) =>
                            _buildImagePlaceholder(theme, isError: true),
                      ),
              ),
            ),
          ),
        ),

        // 选了图才能清除
        if (imagePath != null)
          Align(
            alignment: Alignment.centerRight,
            child: TextButton(
              onPressed: () => setState(() => _imagePath = null),
              child: Text('清除图片', style: theme.littleTextStyle),
            ),
          ),
      ],
    );
  }

  /// 方框里没图的时候显示的东西
  Widget _buildImagePlaceholder(AppTheme theme, {bool isError = false}) {
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(
            isError ? TablerIcons.photoOff : TablerIcons.photoPlus,
            size: 40,
            color: theme.colorEdge,
          ),
          const SizedBox(height: 8),
          Text(
            isError ? '这张图读不出来，换一张试试' : '点击选择图片，或者把图片拖到这里',
            style: theme.littleTextStyle,
            textAlign: TextAlign.center,
          ),
        ],
      ),
    );
  }
}
