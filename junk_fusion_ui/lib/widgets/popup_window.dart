import 'dart:async'; // Timer 类在此库中
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/utils/global_key_defs.dart';
import 'package:junk_fusion_ui/widgets/popupWindow/favorite_playlist.dart';
import 'package:junk_fusion_ui/widgets/popupWindow/ordinary_info_window.dart';
import 'package:junk_fusion_ui/widgets/popupWindow/playlist_create_window.dart';

class DialogUtil {
  // 使用原生的 showDialog 弹出自定义内容
  //这个弹窗没有任何其他按钮，只有内容和关闭
  static Future<void> showInfoDialog(String message) async {
    // 关键：利用 navigatorKey 获取 Context，无需手动传 context
    return showDialog(
      context: navigatorKey.currentContext!,
      barrierDismissible: true, // 点击灰色蒙版是否自动关闭（带默认退场动画）
      builder: (BuildContext context) {
        //当前弹窗组件自带的上下文
        return OrdinaryInfoWindow(
          message: message,
          // 关闭时调用 Navigator.pop，系统会自动执行淡出+缩放动画
          onClose: () => Navigator.of(context).pop(),
        );
      },
    );
  }

  /// 致命错误弹窗（非致命错误不走这里，走 widgets/popupWindow/error_toast.dart）
  ///
  /// 和 showInfoDialog 的区别只有两点：顶上有「致命错误」的红色标题，
  /// 而且不能点蒙版关掉 —— 必须点右上角的 × 表示自己看见了。
  /// UtilFunction.showFatalMessage() 就是调的这里，C++ 的致命错误回调也走这儿。
  static Future<void> showFatalDialog(String message) async {
    final context = navigatorKey.currentContext;
    if (context == null) {
      // 界面还没挂起来（正常不会：回调注册在 initState 里，首帧后才会有错误）
      print('[DialogUtil] 界面还没准备好，这条致命错误只留在控制台：$message');
      return;
    }
    await showDialog(
      context: context,
      barrierDismissible: false, // 不能点外面关，必须点 ×
      builder: (BuildContext dialogContext) {
        return OrdinaryInfoWindow(
          message: message,
          title: '致命错误',
          isError: true,
          onClose: () => Navigator.of(dialogContext).pop(),
        );
      },
    );
  }

  /// 新建歌单弹窗
  ///
  /// 点「确定」返回用户填的内容（标题 + 可选的封面图片路径），
  /// 点「取消」/ 右上角 × / Esc 都返回 null。
  static Future<PlaylistCreateResult?> showPlaylistCreateDialog() {
    return showDialog<PlaylistCreateResult>(
      context: navigatorKey.currentContext!,
      // 里面填了东西，别点一下蒙版就没了
      barrierDismissible: false,
      builder: (BuildContext context) {
        return const PlaylistCreateWindow();
      },
    );
  }

  /// 「收藏到歌单」弹窗
  ///
  /// 返回用户勾选的歌单；点「新建歌单」时 createNew = true（调用方接着弹
  /// 新建歌单窗口）；取消 / × / Esc 返回 null。
  static Future<FavoritePlaylistResult?> showFavoritePlaylistDialog({
    required List<int> songIds,
  }) {
    return showDialog<FavoritePlaylistResult>(
      context: navigatorKey.currentContext!,
      barrierDismissible: false,
      builder: (BuildContext context) {
        return FavoritePlaylistWindow(songIds: songIds);
      },
    );
  }
}
