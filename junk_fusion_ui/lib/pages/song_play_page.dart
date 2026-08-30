import 'package:flutter/material.dart';

class SongPlayPage extends StatefulWidget {
  const SongPlayPage({super.key});
  @override
  State<StatefulWidget> createState() {
    return SongPlayPageState();
  }
}

class SongPlayPageState extends State<SongPlayPage>
    with SingleTickerProviderStateMixin {
  late final AnimationController _controller;
  late final Animation<Offset> _slideAnimation;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      duration: Duration(milliseconds: 500),
    );
    _slideAnimation =
        Tween<Offset>(
          begin: const Offset(0, 1), // 向下移动自身高度的100%
          end: const Offset(0, 0), // 原位
        ).animate(
          CurvedAnimation(
            parent: _controller,
            curve: Curves.easeInOut, // 缓动曲线，更自然
          ),
        );
  }

  @override
  Widget build(BuildContext context) {
    return SlideTransition(position: _slideAnimation, child: Container());
  }
}
