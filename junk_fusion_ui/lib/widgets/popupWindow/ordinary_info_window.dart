import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/widgets/helper_widget.dart';
import 'package:smooth_scroll_multiplatform/smooth_scroll_multiplatform.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

class OrdinaryInfoWindow extends StatefulWidget {
  final String message;
  final VoidCallback onClose; // VoidCallback = void Function() 的类型别名

  const OrdinaryInfoWindow({
    super.key,
    required this.message,
    required this.onClose,
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
    return Center(
      child: Material(
        type: MaterialType.transparency,
        child: Container(
          width: 500,
          // 高度约束：最小 400，最大 700
          constraints: const BoxConstraints(minHeight: 400, maxHeight: 700),
          decoration: BoxDecoration(
            color: AppTheme.colorHover,
            borderRadius: BorderRadius.circular(AppTheme.borderRadius),
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
              fit: StackFit.expand,
              children: [
                DynMouseScroll(
                  builder: (_, controler_, physics_) {
                    return SingleChildScrollView(
                      controller: controler_,
                      physics: physics_,
                      child: Padding(
                        padding: EdgeInsets.symmetric(
                          vertical: 40,
                          horizontal: 30,
                        ),
                        child: Text(
                          widget.message,
                          style: AppTheme.midTextStyle,
                          textAlign: TextAlign.left,
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
                  child: RectIconButton(
                    iconData: TablerIcons.x,
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
