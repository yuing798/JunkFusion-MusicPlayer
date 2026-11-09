/// FavoritePlaylistWindow — 「收藏到歌单」弹窗（网易云那种）
///
/// ── 用途 ─────────────────────────────────────────────────────────────
/// 在歌曲/专辑的右键菜单里点「收藏到歌单」时弹出来，让用户勾选要收进哪些歌单。
///
/// ── 用法 ─────────────────────────────────────────────────────────────
///     // 1. 只弹窗、拿结果（自己处理后续）
///     final result = await DialogUtil.showFavoritePlaylistDialog();
///
///     // 2. 一步到位（弹窗 + 真正写库），右键菜单用这个
///     await showFavoritePlaylistPicker(context: context, songIds: [song.songId]);
///
/// ── 交互 ─────────────────────────────────────────────────────────────
///   · 歌单可以**多选**，底部按钮上会显示「收藏 (N)」
///   · 「新建歌单」→ 关掉本窗口，接着弹 playlist_create_window.dart，
///     建完之后直接把歌塞进新歌单（走 PlaylistProvider.createPlaylist）
///   · 取消 / × / Esc → 返回 null，什么也不做
///
/// 弹窗自己不写库：它只负责收集用户勾了谁，写库交给 PlaylistProvider。
library;

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:junk_fusion_ui/model/playlist_basic_info.dart';
import 'package:junk_fusion_ui/providers/playlist_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/widgets/popup_window.dart';
import 'package:provider/provider.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

/// 弹窗点「收藏」/「新建歌单」之后带出去的结果
class FavoritePlaylistResult {
  /// 用户勾选的歌单 id（可能为空：只有在 [createNew] 为 true 时才会空）
  final List<int> playlistIds;

  /// 用户点了「新建歌单」：调用方要去弹新建歌单窗口
  final bool createNew;

  const FavoritePlaylistResult({
    required this.playlistIds,
    this.createNew = false,
  });
}

class FavoritePlaylistWindow extends StatefulWidget {
  /// 这次要收藏哪些歌（目前只用来显示数量，写库由调用方做）
  final List<int> songIds;

  const FavoritePlaylistWindow({super.key, required this.songIds});

  @override
  State<FavoritePlaylistWindow> createState() => _FavoritePlaylistWindowState();
}

class _FavoritePlaylistWindowState extends State<FavoritePlaylistWindow> {
  /// 勾上的歌单 id
  final Set<int> _selectedIds = <int>{};

  @override
  void initState() {
    super.initState();

    // 歌单列表可能是空的（还没进过歌单页），补拉一次
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted) return;
      final provider = context.read<PlaylistProvider>();
      if (provider.playlists.isEmpty) provider.getAllPlaylists();
    });
  }

  void _toggle(int playlistId, bool? checked) {
    setState(() {
      if (checked == true) {
        _selectedIds.add(playlistId);
      } else {
        _selectedIds.remove(playlistId);
      }
    });
  }

  void _confirm() {
    if (_selectedIds.isEmpty) return;
    Navigator.of(
      context,
    ).pop(FavoritePlaylistResult(playlistIds: _selectedIds.toList()));
  }

  void _createNew() {
    // 关掉自己，让调用方去弹「新建歌单」窗口
    Navigator.of(
      context,
    ).pop(const FavoritePlaylistResult(playlistIds: <int>[], createNew: true));
  }

  void _cancel() {
    Navigator.of(context).pop();
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<AppTheme>();
    final playlists = context.watch<PlaylistProvider>().playlists;

    return CallbackShortcuts(
      bindings: {const SingleActivator(LogicalKeyboardKey.escape): _cancel},
      child: Focus(
        autofocus: true,
        child: Dialog(
          backgroundColor: theme.colorCell,
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(6)),
          child: SizedBox(
            width: 420,
            child: Padding(
              padding: const EdgeInsets.all(20),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  // ── 标题行 ──
                  Row(
                    children: [
                      Text('收藏到歌单', style: theme.bigTextStyle),
                      const SizedBox(width: 10),
                      Text(
                        '共 ${widget.songIds.length} 首',
                        style: theme.littleTextStyle,
                      ),
                      const Spacer(),
                      IconButton(
                        icon: const Icon(TablerIcons.x, size: 24),
                        tooltip: '取消',
                        color: theme.colorTextMain,
                        onPressed: _cancel,
                      ),
                    ],
                  ),

                  const SizedBox(height: 10),

                  // ── 新建歌单 ──
                  InkWell(
                    onTap: _createNew,
                    borderRadius: BorderRadius.circular(4),
                    child: Padding(
                      padding: const EdgeInsets.symmetric(
                        horizontal: 8,
                        vertical: 10,
                      ),
                      child: Row(
                        children: [
                          Icon(
                            TablerIcons.playlistAdd,
                            size: 20,
                            color: theme.colorStress,
                          ),
                          const SizedBox(width: 8),
                          Text('新建歌单', style: theme.midTextStyle),
                        ],
                      ),
                    ),
                  ),

                  Divider(height: 1, color: theme.colorEdge),

                  // ── 歌单列表 ──
                  ConstrainedBox(
                    constraints: const BoxConstraints(maxHeight: 320),
                    child: playlists.isEmpty
                        ? Padding(
                            padding: const EdgeInsets.symmetric(vertical: 30),
                            child: Center(
                              child: Text(
                                '还没有歌单，先新建一个吧',
                                style: theme.littleTextStyle,
                              ),
                            ),
                          )
                        : ListView.builder(
                            shrinkWrap: true,
                            itemCount: playlists.length,
                            itemBuilder: (context, index) {
                              final playlist = playlists[index];
                              return _buildPlaylistRow(theme, playlist);
                            },
                          ),
                  ),

                  const SizedBox(height: 12),

                  // ── 底部：取消 / 收藏 ──
                  Row(
                    mainAxisAlignment: MainAxisAlignment.end,
                    children: [
                      TextButton(
                        onPressed: _cancel,
                        child: Text('取消', style: theme.midTextStyle),
                      ),
                      const SizedBox(width: 8),
                      ElevatedButton(
                        onPressed: _selectedIds.isEmpty ? null : _confirm,
                        style: ElevatedButton.styleFrom(
                          backgroundColor: theme.colorStress,
                          foregroundColor: theme.colorTextMain,
                          elevation: 0,
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(4),
                          ),
                          padding: const EdgeInsets.symmetric(
                            horizontal: 20,
                            vertical: 12,
                          ),
                        ),
                        child: Text(
                          _selectedIds.isEmpty
                              ? '收藏'
                              : '收藏 (${_selectedIds.length})',
                          style: theme.midTextStyle,
                        ),
                      ),
                    ],
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildPlaylistRow(AppTheme theme, PlaylistBasicInfo playlist) {
    final isSelected = _selectedIds.contains(playlist.id);

    return InkWell(
      onTap: () => _toggle(playlist.id, !isSelected),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 4, vertical: 2),
        child: Row(
          children: [
            Checkbox(
              value: isSelected,
              onChanged: (checked) => _toggle(playlist.id, checked),
            ),
            Expanded(
              child: Text(
                playlist.title,
                style: theme.midTextStyle,
                maxLines: 1,
                overflow: TextOverflow.ellipsis,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

/// 「收藏到歌单」一条龙：弹窗 → 用户选择 → 真正写库
///
/// 右键菜单里直接用这个，省得每处都写一遍。
Future<void> showFavoritePlaylistPicker({
  required BuildContext context,
  required List<int> songIds,
}) async {
  if (songIds.isEmpty) return;

  final playlistProvider = context.read<PlaylistProvider>();
  final result = await DialogUtil.showFavoritePlaylistDialog(songIds: songIds);
  if (result == null) return;

  // 用户点了「新建歌单」：走 createPlaylist，它内部会弹新建窗口并顺手把歌塞进去
  if (result.createNew) {
    await playlistProvider.createPlaylist(songIds);
    return;
  }

  if (result.playlistIds.isEmpty) return;
  playlistProvider.addSongsTo(result.playlistIds, songIds);
}
