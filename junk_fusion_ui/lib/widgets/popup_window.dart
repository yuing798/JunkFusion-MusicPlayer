import 'dart:async'; // Timer 类在此库中
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/utils/global_key_defs.dart';
import 'package:junk_fusion_ui/widgets/helper_widget.dart';
import 'package:junk_fusion_ui/widgets/popupWindow/ordinary_info_window.dart';
import 'package:junk_fusion_ui/widgets/popupWindow/song_error_window.dart';
import 'package:smooth_scroll_multiplatform/smooth_scroll_multiplatform.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';
import '../theme/app_theme.dart';

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

  //歌曲错误弹窗
  static Future<void> showSongErrorDialog({
    required String msg,
    required bool hasNextSong,
  }) {
    return showDialog(
      context: navigatorKey.currentContext!,
      barrierDismissible: false, // 禁止点击外部关闭
      builder: (BuildContext context) {
        return SongErrorDialog(msg: msg, hasNextSong: hasNextSong);
      },
    );
  }
}
