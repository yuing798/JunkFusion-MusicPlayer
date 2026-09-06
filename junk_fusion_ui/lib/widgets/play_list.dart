//播放列表
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/providers/song_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/global_key_defs.dart';
import 'package:junk_fusion_ui/utils/utils.dart';
import 'package:junk_fusion_ui/widgets/each_song.dart';
import 'package:provider/provider.dart';
import 'package:smooth_scroll_multiplatform/smooth_scroll_multiplatform.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

class PlayList {
  late final AnimationController _controller;
  late final Animation<Offset> _slideAnimation;
  OverlayEntry? _overlayEntry; // 用来持有覆盖层

  PlayList(TickerProvider vsync) {
    _controller = AnimationController(
      vsync: vsync,
      duration: const Duration(milliseconds: 350),
    );
    _slideAnimation = Tween<Offset>(
      begin: const Offset(1, 0),
      end: const Offset(0, 0),
    ).animate(CurvedAnimation(parent: _controller, curve: Curves.easeInOut));
  }

  void dispose() {
    _removeOverlay();
    _controller.dispose();
  }

  void _removeOverlay() {
    _overlayEntry?.remove();
    _overlayEntry = null;
    _controller.reset();
  }

  void showPlayList() {
    _removeOverlay(); //先移除旧的
    final overlay = navigatorKey.currentState!.overlay;
    // final theme = navigatorKey.currentContext!.watch<AppTheme>();//因为watch必须在builder或者build函数中才能使用
    _overlayEntry = OverlayEntry(
      builder: (context) {
        final theme = context.watch<AppTheme>();
        return Stack(
          children: [
            Positioned.fill(
              child: GestureDetector(
                onTap: () {
                  _controller.reverse().then((_) => _removeOverlay());
                  //_controller.reverse()：触发一个 Flutter 动画控制器反向执行（比如让播放栏滑出屏幕）。
                  //它返回一个 Future<void>，这个 Future 会在动画彻底结束（到达 0.0）时完成（resolve）。
                  //.then(...)：当 Future 完成时，自动调用括号里的函数。这是 Dart 中处理异步回调的经典写法，等价于 await 的效果。
                  //(_) => _removeOverlay()：这是一个箭头函数。_ 是一个占位符参数，
                  //代表 .then 传过来的返回值（因为 reverse 返回的是 void，所以用 _ 忽略它）。
                  //执行体是调用 _removeOverlay()（即你自定义的移除 OverlayEntry 的方法）。
                },
                child: ColoredBox(color: Colors.transparent),
              ),
            ),
            Align(
              alignment: Alignment.centerRight,
              child: SlideTransition(
                position: _slideAnimation,
                child: SizedBox(
                  width: MediaQuery.sizeOf(context).width * 0.25,
                  height: MediaQuery.sizeOf(context).height * 0.7,

                  child: Material(
                    //黄色双下划线的出现原因：在正常的页面中，
                    //我们的代码都写在 Scaffold 里，Scaffold 底层帮我们提供了一套 DefaultTextStyle（默认文本样式，去掉了难看的下划线）。
                    // 但是，你现在使用的是 OverlayEntry。Overlay 是悬浮在所有页面最顶层的一块画布，
                    //它完全脱离了你原有的 Scaffold 结构。
                    color: theme.colorHover,
                    child: Padding(
                      padding: EdgeInsetsGeometry.symmetric(
                        vertical: 10,
                        horizontal: 15,
                      ),
                      child: Column(
                        children: [
                          Row(
                            children: [
                              Text("播放列表", style: theme.midTextStyle),
                              SizedBox(width: 10),
                              Text(
                                "${context.watch<PlaybackProvider>().playList.length}首歌",
                                style: theme.littleTextStyle,
                              ),
                              const Spacer(),
                              IconButton(
                                onPressed: () {},
                                icon: Icon(TablerIcons.stackPush),
                                tooltip: "收藏全部",
                              ),
                              IconButton(
                                onPressed: () {},
                                icon: Icon(TablerIcons.heart),
                                tooltip: "喜欢全部",
                              ),
                              IconButton(
                                onPressed: () {},
                                icon: Icon(TablerIcons.trash),
                                tooltip: "清空列表",
                              ),
                            ],
                          ),
                          Expanded(
                            child: DynMouseScroll(
                              builder: (context_, controler_, physics_) {
                                final playback = context_
                                    .watch<PlaybackProvider>();
                                return ListView.builder(
                                  //ListView.builder是虚拟滚动的，而ListView是全量创建的
                                  controller: controler_,
                                  physics: physics_,
                                  // `itemCount` 等于列表长度
                                  itemCount: context
                                      .watch<PlaybackProvider>()
                                      .playListCount,
                                  padding: EdgeInsets.only(right: 10),
                                  // `itemExtent` 固定每个 item 高度（性能优化）
                                  // 对应原 Vue 虚拟滚动的 estimateSize: () => 80
                                  itemExtent: 80,
                                  // itemBuilder 构建每个 item
                                  // `(context, index) => Widget`
                                  itemBuilder: (context, index) {
                                    final song = context
                                        .watch<PlaybackProvider>()
                                        .playList[index];

                                    return Row(
                                      key: ValueKey(song.songId),
                                      // `ValueKey` 基于值的唯一 Key，帮助 Flutter 识别
                                      // 列表项的身份（diff 算法优化）
                                      crossAxisAlignment:
                                          CrossAxisAlignment.center,
                                      children: [
                                        SizedBox(
                                          width: 50,
                                          height: 50,
                                          child: GestureDetector(
                                            onTap: () {
                                              //箭头函数后面只能接上一句表达式
                                              if (playback.currentSong ==
                                                  song) {
                                                context
                                                    .read<PlaybackProvider>()
                                                    .togglePlayPause();
                                              } else {
                                                context
                                                    .read<PlaybackProvider>()
                                                    .setNewSong(song);
                                              }
                                            },
                                            child: Stack(
                                              alignment: Alignment.center,
                                              children: [
                                                // 封面图（非当前歌曲时显示）
                                                (playback.currentSong != song)
                                                    ? HoverPlayButton(
                                                        hash: song.hash,
                                                      )
                                                    : (playback.isPlaying)
                                                    ? Icon(
                                                        TablerIcons
                                                            .playerPauseFilled,
                                                        size: 32,
                                                      )
                                                    : Icon(
                                                        TablerIcons
                                                            .playerPlayFilled,
                                                        size: 32,
                                                      ),
                                              ],
                                            ),
                                          ),
                                        ),
                                        SizedBox(width: 15),
                                        Align(
                                          alignment:
                                              AlignmentGeometry.centerLeft,
                                          child: SizedBox(
                                            width: 120,

                                            //子组件必须显式设定高度才能被crossAxisAlignment:CrossAxisAlignment.center,影响
                                            height: 50,
                                            child: Column(
                                              crossAxisAlignment:
                                                  CrossAxisAlignment.start,
                                              children: [
                                                Tooltip(
                                                  message: song.title,
                                                  child: Text(
                                                    song.title,
                                                    style: context
                                                        .watch<AppTheme>()
                                                        .midTextStyle,
                                                    overflow:
                                                        TextOverflow.ellipsis,
                                                  ),
                                                ),
                                                Tooltip(
                                                  message:
                                                      song.artists?.join(
                                                        " / ",
                                                      ) ??
                                                      "未知",
                                                  child: Text(
                                                    song.artists?.join(" / ") ??
                                                        "未知",
                                                    style: context
                                                        .watch<AppTheme>()
                                                        .littleTextStyle,
                                                    overflow:
                                                        TextOverflow.ellipsis,
                                                  ),
                                                ),
                                              ],
                                            ),
                                          ),
                                        ),
                                        const Spacer(),
                                        Text(
                                          UtilFunction.formatDuration(
                                            song.duration,
                                          ),
                                        ),
                                      ],
                                    );
                                  },
                                );
                              },
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
                ),
              ),
            ),
          ],
        );
      },
    );
    overlay!.insert(_overlayEntry!);
    _controller.forward();
  }
}
