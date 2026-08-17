import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

/// 显示歌曲播放错误弹窗

/// 错误弹窗内容组件（有状态，用于管理复选框）
class SongErrorDialog extends StatefulWidget {
  final String msg;
  final bool hasNextSong;

  const SongErrorDialog({
    super.key,
    required this.msg,
    required this.hasNextSong,
  });

  @override
  SongErrorDialogState createState() => SongErrorDialogState();
}

class SongErrorDialogState extends State<SongErrorDialog> {
  // 复选框状态
  bool _autoSkip = false;

  @override
  Widget build(BuildContext context) {
    // 根据是否有下一首决定左侧按钮文案
    final String rightButtonText = widget.hasNextSong ? '播放下一首' : '停止播放';

    return Dialog(
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(12.0), // 圆角
      ),
      child: Padding(
        padding: const EdgeInsets.all(24.0),
        child: Column(
          mainAxisSize: MainAxisSize.min, // 高度自适应
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // ========== 第一行：标题栏 ==========
            Row(
              children: [
                // 红色错误图标
                const Icon(
                  TablerIcons.alertTriangleFilled,
                  color: Colors.red,
                  size: 28,
                ),
                const SizedBox(width: 8),
                // 标题文字
                Expanded(
                  child: Text(
                    '播放失败', // 可根据喜好改为“无法播放该歌曲”
                    style: AppTheme.littleTextStyle,
                  ),
                ),
                // 右上角关闭按钮
                IconButton(
                  icon: const Icon(Icons.close),
                  onPressed: () {
                    // TODO: 关闭弹窗，仅关闭，不执行其他操作
                    Navigator.of(context).pop();
                  },
                  padding: EdgeInsets.zero,
                  constraints: const BoxConstraints(),
                ),
              ],
            ),
            const SizedBox(height: 16),

            // ========== 第二行：文本内容 ==========
            Text(
              widget.msg,
              style: TextStyle(fontSize: 14, color: Colors.grey[700]),
            ),
            const SizedBox(height: 16),

            // ========== 第三行：复选框（小字） ==========
            Row(
              children: [
                // 复选框（紧凑样式）
                SizedBox(
                  width: 20,
                  height: 20,
                  child: Checkbox(
                    value: _autoSkip,
                    onChanged: (bool? value) {
                      setState(() {
                        _autoSkip = value ?? false;
                      });
                    },
                    materialTapTargetSize: MaterialTapTargetSize.shrinkWrap,
                  ),
                ),
                const SizedBox(width: 4),
                // 复选框文字
                Text('以后遇到歌曲错误直接跳过（不再询问）', style: AppTheme.littleTextStyle),
              ],
            ),
            const SizedBox(height: 24),

            // ========== 第四行：操作按钮 ==========
            Row(
              mainAxisAlignment: MainAxisAlignment.end, // 按钮右对齐
              children: [
                // 左侧次要按钮（从列表移除 / 停止播放）
                TextButton(
                  onPressed: () {
                    // TODO: 执行次要操作
                    // 点击后关闭弹窗，具体业务逻辑由外部处理
                    Navigator.of(context).pop();
                  },
                  child: Text("从列表删除"),
                ),
                const SizedBox(width: 8),
                // 右侧主要按钮（播放下一首）
                ElevatedButton(
                  onPressed: () {
                    // TODO: 执行主要操作（播放下一首）
                    Navigator.of(context).pop();
                  },
                  child: Text(rightButtonText),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
