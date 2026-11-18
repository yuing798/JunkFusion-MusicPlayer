import 'package:flutter/material.dart';
import 'package:flutter_root_context_menu/flutter_root_context_menu.dart';
import 'package:junk_fusion_ui/model/fixed_playlist.dart';
import 'package:junk_fusion_ui/model/playlist_basic_info.dart';
import 'package:junk_fusion_ui/providers/playlist_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/widgets/helper_widget/hash_image_or_default.dart';
import 'package:junk_fusion_ui/widgets/pages/pages_route.dart';
import 'package:provider/provider.dart';
import 'package:smooth_scroll_multiplatform/smooth_scroll_multiplatform.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

// ════════════════════════════════════════════════════════════════
// playlist_view_page.dart — 歌单视图（列表）
//
// 布局（自上而下）：
//   第 1 行（50px）：标题「歌单」+「共 N 个」
//   第 2 行（80px）：工具栏（新建歌单）
//   第 3 部分：歌单列表（鼠标滚轮平滑滚动），分两段：
//
//       ── 固定歌单 ──     我喜欢 / 最近播放 / 最多播放
//       ── 我的歌单 ──     用户自己建的（可右键删除）
//
// 固定歌单不是 playlists 表里的行，是对 songs 表的三种固定查询
// （见 model/fixed_playlist.dart）。所以：
//   · 它们**没有**右键菜单——一个只有「删除」且被禁用的菜单比不弹更糟；
//   · 卡片封面位置画图标，不画封面图；
//   · 打开时走 PlaylistProvider.openFixedPlaylist()，单歌单页照常复用。
//
// 点卡片走路由进 SinglePlaylistPage（见 pages_route.dart）。
// ════════════════════════════════════════════════════════════════

class PlaylistViewPage extends StatefulWidget {
  const PlaylistViewPage({super.key});

  @override
  State<PlaylistViewPage> createState() => _PlaylistViewPageState();
}

class _PlaylistViewPageState extends State<PlaylistViewPage> {
  /// 打开某个歌单：先把歌单装进 Provider，再往路由栈里压一条记录
  void _openPlaylist(PlaylistBasicInfo playlist) {
    context.read<PlaylistProvider>().setNewPlaylist(playlist.id);
    context.read<PagesRouteController>().push(
      PagesRouteTable.singlePlaylist(playlist),
    );
  }

  /// 打开固定歌单：数据由 Provider 现查，路由只用来压栈
  void _openFixedPlaylist(FixedPlaylistKind kind) {
    final provider = context.read<PlaylistProvider>();

    // 先让 provider 把歌准备好，再压路由（SinglePlaylistPage 不接参数）
    provider.openFixedPlaylist(kind);

    context.read<PagesRouteController>().push(
      PagesRouteTable.singlePlaylist(
        PlaylistBasicInfo(
          id: kind.syntheticId,
          title: kind.title,
          createTime: '',
          hash: null,
        ),
      ),
    );
  }

  /// 右键菜单（只有「我的歌单」有）
  void _showPlaylistMenu(
    BuildContext context,
    Offset position,
    int playlistId,
  ) {
    showRootContextMenu(
      context: context,
      position: position,
      useBarrier: true,
      items: [
        ContextMenuItem(
          label: '删除歌单',
          onTap: () =>
              context.read<PlaylistProvider>().deletePlaylist(playlistId),
        ),
      ],
    );
  }

  @override
  Widget build(BuildContext context) {
    // watch 放在最前面：歌单列表变了、主题变了都要重画
    final theme = context.watch<AppTheme>();
    final playlists = context.watch<PlaylistProvider>().playlists.toList()
      ..sort((a, b) => a.id.compareTo(b.id));

    // 一段一段拼成列表项：段标题 / 固定歌单 / 用户歌单 / 空提示
    final entries = <_PlaylistListEntry>[
      const _SectionHeaderEntry('固定歌单'),
      for (final kind in FixedPlaylistKind.values) _FixedEntry(kind),
      const _SectionHeaderEntry('我的歌单'),
      if (playlists.isEmpty)
        const _EmptyHintEntry()
      else
        for (final playlist in playlists) _CustomEntry(playlist),
    ];

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // ── 第 1 行：标题行 ──
        SizedBox(
          height: 50,
          child: Row(
            children: [
              Text('歌单', style: theme.bigTextStyle),
              const SizedBox(width: 15),
              Text('共 ${playlists.length} 个', style: theme.littleTextStyle),
            ],
          ),
        ),

        // ── 第 2 行：工具栏 ──
        SizedBox(
          height: 80,
          child: Row(
            children: [
              // ── 新建歌单（弹窗在 widgets/popupWindow/playlist_create_window.dart） ──
              SizedBox(
                height: 40,
                child: TextButton(
                  onPressed: () =>
                      context.read<PlaylistProvider>().createPlaylist(null),
                  style: TextButton.styleFrom(
                    backgroundColor: theme.colorHover,
                    foregroundColor: theme.colorTextMain,
                    shape: const RoundedRectangleBorder(
                      borderRadius: BorderRadius.zero, // 直角矩形
                    ),
                  ),
                  child: Text('新建歌单', style: theme.midTextStyle),
                ),
              ),

              // 弹性空间（以后排序控件放右边）
              const Spacer(),
            ],
          ),
        ),

        // ── 第 3 部分：歌单列表（虚拟滚动，填充剩余空间） ──
        Expanded(
          child: DynMouseScroll(
            builder: (context_, controler_, physics_) {
              return ListView.builder(
                controller: controler_,
                physics: physics_,
                itemCount: entries.length,
                // 播放栏在布局里，不用再留底部空白
                padding: const EdgeInsets.only(right: 10),
                itemBuilder: (context, index) => _buildEntry(entries[index]),
              );
            },
          ),
        ),
      ],
    );
  }

  Widget _buildEntry(_PlaylistListEntry entry) {
    return switch (entry) {
      _SectionHeaderEntry(:final title) => _SectionHeader(title: title),
      _FixedEntry(:final kind) => GestureDetector(
        behavior: HitTestBehavior.opaque,
        onTap: () => _openFixedPlaylist(kind),
        // 固定歌单刻意不挂右键菜单（删不掉的东西不该有删除菜单）
        child: _FixedPlaylistCard(key: ValueKey(kind), kind: kind),
      ),
      _CustomEntry(:final playlist) => GestureDetector(
        behavior: HitTestBehavior.opaque,
        onTap: () => _openPlaylist(playlist),
        onSecondaryTapDown: (details) =>
            _showPlaylistMenu(context, details.globalPosition, playlist.id),
        child: _PlaylistCard(key: ValueKey(playlist.id), playlist: playlist),
      ),
      _EmptyHintEntry() => const _EmptyHint(),
    };
  }
}

// ════════════════════════════════════════════════════════════════
// 列表项类型（sealed：加一种新的行，下面的 switch 会强制你处理）
// ════════════════════════════════════════════════════════════════
sealed class _PlaylistListEntry {
  const _PlaylistListEntry();
}

class _SectionHeaderEntry extends _PlaylistListEntry {
  final String title;
  const _SectionHeaderEntry(this.title);
}

class _FixedEntry extends _PlaylistListEntry {
  final FixedPlaylistKind kind;
  const _FixedEntry(this.kind);
}

class _CustomEntry extends _PlaylistListEntry {
  final PlaylistBasicInfo playlist;
  const _CustomEntry(this.playlist);
}

class _EmptyHintEntry extends _PlaylistListEntry {
  const _EmptyHintEntry();
}

// ════════════════════════════════════════════════════════════════
// 段标题：固定歌单 / 我的歌单
// ════════════════════════════════════════════════════════════════
class _SectionHeader extends StatelessWidget {
  final String title;

  const _SectionHeader({required this.title});

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();

    return Padding(
      padding: const EdgeInsets.only(top: 8, bottom: 2, left: 2),
      child: Text(
        title,
        style: theme.littleTextStyle.copyWith(fontWeight: FontWeight.bold),
      ),
    );
  }
}

// ════════════════════════════════════════════════════════════════
// _FixedPlaylistCard — 固定歌单卡片
//
// 和用户歌单卡片同样高（50px 封面 + 一行字），只是封面位置画图标，
// 右边那列换成"最多 200 首"，让人一眼看出这不是自己建的。
// ════════════════════════════════════════════════════════════════
class _FixedPlaylistCard extends StatelessWidget {
  final FixedPlaylistKind kind;

  const _FixedPlaylistCard({super.key, required this.kind});

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();

    return Row(
      children: [
        // 图标代替封面
        Container(
          width: 50,
          height: 50,
          decoration: BoxDecoration(
            color: theme.colorHover,
            borderRadius: BorderRadius.circular(4),
          ),
          child: Icon(kind.icon, size: 24, color: theme.colorStress),
        ),
        const VerticalDivider(width: 10),
        Expanded(
          child: Text(
            kind.title,
            style: theme.midTextStyle,
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
          ),
        ),
        Text(kind.subtitle, style: theme.littleTextStyle),
        const SizedBox(width: 20),
        Text(kind.limitText, style: theme.littleTextStyle),
      ],
    );
  }
}

// ════════════════════════════════════════════════════════════════
// _EmptyHint — 「我的歌单」为空时的提示
//
// 固定歌单永远有三个，所以页面不会再出现"整页空白"，
// 空状态只针对用户歌单那一段。
// ════════════════════════════════════════════════════════════════
class _EmptyHint extends StatelessWidget {
  const _EmptyHint();

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();

    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24),
      child: Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(TablerIcons.playlist, size: 40, color: theme.colorEdge),
            const SizedBox(height: 10),
            Text('还没有自己建的歌单', style: theme.midTextStyle),
          ],
        ),
      ),
    );
  }
}

// ════════════════════════════════════════════════════════════════
// _PlaylistCard — 用户自己建的歌单卡片
// ════════════════════════════════════════════════════════════════
class _PlaylistCard extends StatefulWidget {
  final PlaylistBasicInfo playlist;

  const _PlaylistCard({super.key, required this.playlist});

  @override
  State<_PlaylistCard> createState() => _PlaylistCardState();
}

class _PlaylistCardState extends State<_PlaylistCard> {
  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        SizedBox(
          width: 50,
          height: 50,
          child: HashImageOrDefault(hash: widget.playlist.hash),
        ),
        const VerticalDivider(width: 10),
        Text(widget.playlist.title),
        const Spacer(),
        Text(widget.playlist.createTime),
      ],
    );
  }
}
