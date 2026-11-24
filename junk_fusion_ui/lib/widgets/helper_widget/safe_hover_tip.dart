import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';

class SafeHoverTip extends StatefulWidget {
  const SafeHoverTip({super.key, required this.message, required this.child});

  final String message;
  final Widget child;

  @override
  State<SafeHoverTip> createState() => _SafeHoverTipState();
}

class _SafeHoverTipState extends State<SafeHoverTip> {
  bool _show = false;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return MouseRegion(
      onEnter: (_) => setState(() => _show = true),
      onExit: (_) => setState(() => _show = false),
      child: Stack(
        clipBehavior: Clip.none,
        alignment: Alignment.center,
        children: [
          widget.child,
          if (_show)
            Positioned(
              // 按你的布局微调：图标按钮一般在下方或上方
              bottom: 28,
              child: Material(
                elevation: 4,
                borderRadius: BorderRadius.circular(4),
                color: theme.colorScheme.inverseSurface,
                child: Padding(
                  padding: const EdgeInsets.symmetric(
                    horizontal: 8,
                    vertical: 4,
                  ),
                  child: Text(
                    widget.message,
                    style: theme.textTheme.bodySmall?.copyWith(
                      color: theme.colorScheme.onInverseSurface,
                    ),
                  ),
                ),
              ),
            ),
        ],
      ),
    );
  }
}
