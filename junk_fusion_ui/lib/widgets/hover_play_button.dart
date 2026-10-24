//正方形图片和播放按键组合的组件
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/utils/app_cache.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

class HoverPlayButton extends StatefulWidget {
  final String? hash; //图片哈希值
  const HoverPlayButton({super.key, required this.hash});

  @override
  State<HoverPlayButton> createState() => _HoverPlayButtonState();
}

class _HoverPlayButtonState extends State<HoverPlayButton> {
  bool _isHovered = false; // 悬停状态

  @override
  Widget build(BuildContext context) {
    return MouseRegion(
      onEnter: (_) => setState(() => _isHovered = true),
      onExit: (_) => setState(() => _isHovered = false),
      child: SizedBox(
        width: 50,
        height: 50,
        child: Stack(
          // 移除 fit: StackFit.expand，让子组件自然布局
          children: [
            // 1. 图片（始终撑满）
            AnimatedOpacity(
              opacity: _isHovered ? 0.5 : 1.0,
              duration: const Duration(milliseconds: 300),
              child: SizedBox(
                width: 50,
                height: 50,
                child: (widget.hash != null)
                    ? Image.file(
                        File(
                          '${AppCache.cacheDirString}/image/${widget.hash}/original.jpg',
                        ),
                        fit: BoxFit.cover,
                      )
                    : Image.asset(
                        "assets/image/JunkFusion.png",
                        fit: BoxFit.cover,
                      ),
              ),
            ),
            // 2. 遮罩（黑色半透明，加深图片）
            AnimatedOpacity(
              opacity: _isHovered ? 0.4 : 0.0,
              duration: const Duration(milliseconds: 300),
              child: Container(color: Colors.black),
            ),
            // 3. 播放图标（居中，无背景圆形）
            Center(
              child: AnimatedOpacity(
                opacity: _isHovered ? 1.0 : 0.0, // 悬停时完全显示，不透明
                duration: const Duration(milliseconds: 300),
                child: Icon(
                  TablerIcons.playerPlayFilled,
                  size: 32,
                  color: Colors.white, // 白色图标，在暗色背景下清晰
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
