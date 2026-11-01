import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/layout_controller.dart';
import 'package:junk_fusion_ui/widgets/nav/pages_nav_table.dart';
import 'package:junk_fusion_ui/widgets/pages/pages_route.dart';
import 'package:provider/provider.dart';

// ════════════════════════════════════════════════════════════════
// icon_rail.dart — 三段式最左边那条图标栏
//
// 就是 VS Code 的 activity bar：固定 56px 宽，只放图标，不显示文字，
// 鼠标悬停浮 tooltip。图标和名字全部来自 widgets/nav/pages_nav_table.dart，
// 这里不存任何导航数据。
//
// 三种状态：
//   选中   —— 图标底色加深 + 最左边一条强调色（colorStress）
//   悬停   —— 图标底色变浅（半透明的 colorHover）
//   默认   —— 透明
//
// 选中项不在这里存：归 PagesRouteController.currentNavId 管，
// 所以点图标栏 / 点内容区左上角返回按钮，高亮都会自动同步。
//
// 用法（只有 ThreeColumnLayout 用到，不用传参）：
//     const IconRail()
// ════════════════════════════════════════════════════════════════

/// 左侧图标栏
class IconRail extends StatelessWidget {
  const IconRail({super.key});

  @override
  Widget build(BuildContext context) {
    // watch：当前段变了、主题变了都要重画
    final theme = context.watch<AppTheme>();
    final selectedId = context.watch<PagesRouteController>().currentNavId;

    // 有歌在播的时候底部 PlayBar 会盖住最下面 90px，最后几个图标得能滚上来；
    // 没歌的时候不留这块空白（不然图标栏明明放得下却能滚，很怪）
    final hasPlayBar = context.select<PlaybackProvider, bool>(
      (provider) => provider.curBasicInfo != null,
    );

    return Container(
      width: LayoutController.railWidth,
      color: theme.colorNav,
      // 和中间栏同色，加一条细线把两段分开（foregroundDecoration 不占宽度，
      // 不会把 56px 的栏挤成 55px 导致里面的按钮溢出）
      foregroundDecoration: BoxDecoration(
        border: Border(
          right: BorderSide(color: theme.colorEdge.withAlpha(120), width: 1),
        ),
      ),
      child: SingleChildScrollView(
        padding: EdgeInsets.only(top: 8, bottom: hasPlayBar ? 100 : 8),
        child: Column(
          children: [
            // 按分组画：组和组之间一条细分隔线
            for (var i = 0; i < PagesNavTable.groups.length; i++) ...[
              if (i > 0) _buildGroupDivider(theme),
              for (final section in PagesNavTable.groups[i])
                _RailButton(
                  key: ValueKey(section.id),
                  section: section,
                  isSelected: section.id == selectedId,
                  // 传标题是因为路由表里还没写这一页的时候，占位页要用它当标题
                  onTap: () => context.read<PagesRouteController>().openNav(
                    section.id,
                    title: section.title,
                  ),
                ),
            ],
          ],
        ),
      ),
    );
  }

  /// 分组之间的分隔线
  Widget _buildGroupDivider(AppTheme theme) {
    return Container(
      height: 1,
      margin: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
      color: theme.colorEdge.withAlpha(120),
    );
  }
}

// ════════════════════════════════════════════════════════════════
// _RailButton — 图标栏上的一个按钮
//
// 自己做 hover 状态（Flutter 的 IconButton 带一圈默认 padding 和最小尺寸，
// 塞不进 56px 宽的栏里，而且选中态要画左边那条强调色）
// ════════════════════════════════════════════════════════════════
class _RailButton extends StatefulWidget {
  final PagesNavSection section;
  final bool isSelected;
  final VoidCallback onTap;

  const _RailButton({
    super.key,
    required this.section,
    required this.isSelected,
    required this.onTap,
  });

  @override
  State<_RailButton> createState() => _RailButtonState();
}

class _RailButtonState extends State<_RailButton> {
  /// 鼠标是否悬浮在按钮上
  bool _isHovered = false;

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();

    // 选中比悬浮更实：选中用实色，悬浮用半透明
    final Color background;
    if (widget.isSelected) {
      background = theme.colorHover;
    } else if (_isHovered) {
      background = theme.colorHover.withAlpha(90);
    } else {
      background = Colors.transparent;
    }

    return Tooltip(
      message: widget.section.title,
      waitDuration: const Duration(milliseconds: 450),
      child: GestureDetector(
        // opaque：整块 56×48 都能点，而不是只有中间那个图标能点
        behavior: HitTestBehavior.opaque,
        onTap: widget.onTap,
        child: MouseRegion(
          cursor: SystemMouseCursors.click,
          onEnter: (_) => setState(() => _isHovered = true),
          onExit: (_) => setState(() => _isHovered = false),
          child: SizedBox(
            width: LayoutController.railWidth,
            height: 48,
            child: Stack(
              children: [
                // ── 图标本体 ──
                Center(
                  child: AnimatedContainer(
                    duration: const Duration(milliseconds: 120),
                    width: 44,
                    height: 40,
                    decoration: BoxDecoration(
                      color: background,
                      borderRadius: BorderRadius.circular(8),
                    ),
                    child: Icon(
                      widget.section.icon,
                      size: 24,
                      color: widget.isSelected
                          ? theme.colorTextMain
                          : theme.colorTextSecond,
                    ),
                  ),
                ),

                // ── 选中态：最左侧一条强调色 ──
                if (widget.isSelected)
                  Positioned(
                    left: 0,
                    top: 10,
                    bottom: 10,
                    child: Container(
                      width: 3,
                      decoration: BoxDecoration(
                        color: theme.colorStress,
                        borderRadius: BorderRadius.circular(3),
                      ),
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
