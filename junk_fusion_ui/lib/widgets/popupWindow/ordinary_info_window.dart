import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:provider/provider.dart';
import 'package:smooth_scroll_multiplatform/smooth_scroll_multiplatform.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

class OrdinaryInfoWindow extends StatefulWidget {
  final String message;
  final VoidCallback onClose; // VoidCallback = void Function() 的类型别名

  /// 顶部标题；null 就只显示内容（老行为）
  final String? title;

  /// 是否是「致命错误」样式：标题前加红色警告图标、标题字也变红
  final bool isError;

  const OrdinaryInfoWindow({
    super.key,
    required this.message,
    required this.onClose,
    this.title,
    this.isError = false,
  });

  @override
  State<OrdinaryInfoWindow> createState() {
    return OrdinaryInfoWindowState();
  }
}

// ordinaryInfoWindow — 通知弹窗的实际 UI widget
class OrdinaryInfoWindowState extends State<OrdinaryInfoWindow> {
  final _scrollController = ScrollController();

  @override
  void dispose() {
    _scrollController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    return Center(
      child: Material(
        type: MaterialType.transparency,
        child: Container(
          width: 500,
          // 高度约束：最小 200，最大 700
          constraints: const BoxConstraints(minHeight: 200, maxHeight: 700),
          decoration: BoxDecoration(
            color: theme.colorHover,
            borderRadius: BorderRadius.circular(6),
            boxShadow: const [
              BoxShadow(
                color: Colors.black26,
                offset: Offset(0, 4),
                blurRadius: 16,
              ),
            ],
          ),
          child: Padding(
            padding: const EdgeInsets.symmetric(vertical: 5, horizontal: 5),
            child: Stack(
              // fit: StackFit.expand,
              children: [
                DynMouseScroll(
                  builder: (_, controler_, physics_) {
                    return SizedBox(
                      width: double.infinity,
                      child: SingleChildScrollView(
                        controller: controler_,
                        physics: physics_,
                        child: Padding(
                          padding: EdgeInsets.symmetric(
                            vertical: 40,
                            horizontal: 30,
                          ),
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              // 有标题就先画一行标题（致命错误是红图标 + 红字）
                              if (widget.title != null) ...[
                                Row(
                                  children: [
                                    if (widget.isError) ...[
                                      Icon(
                                        TablerIcons.alertTriangleFilled,
                                        size: 26,
                                        color: theme.colorError,
                                      ),
                                      const SizedBox(width: 8),
                                    ],
                                    Expanded(
                                      child: Text(
                                        widget.title!,
                                        style: theme.bigTextStyle.copyWith(
                                          fontSize: 20,
                                          color: widget.isError
                                              ? theme.colorError
                                              : null,
                                        ),
                                      ),
                                    ),
                                  ],
                                ),
                                const SizedBox(height: 14),
                              ],
                              Text(
                                widget.message,
                                style: theme.midTextStyle,
                                textAlign: TextAlign.left,
                              ),
                            ],
                          ),
                        ),
                      ),
                    );
                  },
                ),
                // ),

                // 3. 关闭按钮（固定在右上角，不随内容滚动）
                Positioned(
                  top: 6,
                  right: 6,
                  child: IconButton(
                    icon: Icon(TablerIcons.x, size: 32),
                    onPressed: widget.onClose,
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
