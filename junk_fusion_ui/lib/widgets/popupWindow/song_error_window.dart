import 'package:flutter/material.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

class SongErrorDialog extends StatefulWidget {
  final String msg;

  const SongErrorDialog({super.key, required this.msg});

  @override
  State<SongErrorDialog> createState() => _SongErrorDialogState();
}

class _SongErrorDialogState extends State<SongErrorDialog> {
  // 当前动作：0=跳过，1=跳过并移除
  bool _selectedAction = false;

  // 全局偏好（复选框状态）
  bool _applyToFuture = false;

  // 获取动态复选框文案
  String get _checkboxLabel {
    return _selectedAction == false
        ? '以后遇到播放失败歌曲自动跳过（不再询问）'
        : '以后遇到播放失败歌曲自动跳过并移除（不再询问）';
  }

  @override
  Widget build(BuildContext context) {
    return Dialog(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12.0)),
      child: Padding(
        padding: const EdgeInsets.all(24.0),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // ========== 第一行：标题 ==========
            Row(
              children: [
                const Icon(
                  TablerIcons.alertTriangleFilled,
                  color: Colors.red,
                  size: 28,
                ),
                const SizedBox(width: 8),
                Expanded(
                  child: Text(
                    '播放失败',
                    style: const TextStyle(
                      fontSize: 18,
                      fontWeight: FontWeight.w600,
                    ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),

            // ========== 第二行：正文 ==========
            Text(
              widget.msg,
              style: TextStyle(fontSize: 14, color: Colors.grey[700]),
            ),
            const SizedBox(height: 20),

            // ========== 第三行：当前操作（单选组，带边框） ==========
            Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  '请选择当前歌曲的处理方式：',
                  style: TextStyle(
                    fontSize: 13,
                    fontWeight: FontWeight.w500,
                    color: Colors.grey[700],
                  ),
                ),
                const SizedBox(height: 8),
                Container(
                  decoration: BoxDecoration(
                    border: Border.all(color: Colors.grey[300]!),
                    borderRadius: BorderRadius.circular(8.0),
                  ),
                  child: RadioGroup<bool>(
                    groupValue: _selectedAction,
                    onChanged: (value) => _selectedAction = value!,
                    child: const Column(
                      children: [
                        RadioListTile<bool>(value: false, title: Text('跳过这首歌')),
                        RadioListTile(
                          value: true,
                          title: Text("跳过并在歌曲列表删除这首歌"),
                        ),
                      ],
                    ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 20),

            // ========== 第四行：全局偏好（复选框，文案动态变化） ==========
            Row(
              children: [
                SizedBox(
                  width: 20,
                  height: 20,
                  child: CheckboxListTile(
                    value: _applyToFuture,
                    onChanged: (bool? value) {
                      setState(() {
                        _applyToFuture = value ?? false;
                      });
                    },
                    materialTapTargetSize: MaterialTapTargetSize.shrinkWrap,
                  ),
                ),
                const SizedBox(width: 4),
                // 动态文案
                Expanded(
                  child: Text(
                    _checkboxLabel,
                    style: TextStyle(fontSize: 12, color: Colors.grey[600]),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 24),

            // ========== 第五行：确认按钮 ==========
            Row(
              mainAxisAlignment: MainAxisAlignment.end,
              children: [
                TextButton(
                  onPressed: () {
                    Navigator.of(context).pop();
                  },
                  child: const Text('取消'),
                ),
                const SizedBox(width: 8),
                ElevatedButton(
                  onPressed: () {
                    // TODO:
                    // 1. 根据 _selectedAction (0 或 1) 执行对应的当前操作
                    // 2. 如果 _applyToFuture == true，将 _selectedAction 的值保存到 SharedPreferences
                    // 3. 下次解码失败时，直接根据存储的值静默执行，不再弹窗
                    Navigator.of(context).pop();
                  },
                  child: const Text('确定'),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
