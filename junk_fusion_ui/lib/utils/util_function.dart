import 'dart:ffi';

import 'package:ffi/ffi.dart';
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';

class UtilFunction {
  // 将秒数格式化为 分:秒 或 时:分:秒 的可读字符串
  //可选：是否显示毫秒数
  static String formatDuration(double seconds, [bool needMs = false]) {
    if (seconds < 0) return '0:00';

    // 1. 分离整数部分（秒）和小数部分（毫秒）
    final totalSecs = seconds.floor(); // 向下取整，得到整秒
    final milliseconds = ((seconds - totalSecs) * 1000).round();

    // 2. 计算 时/分/秒
    final hours = totalSecs ~/ 3600;
    final minutes = (totalSecs % 3600) ~/ 60;
    final secs = totalSecs % 60;

    // 3. 拼接主要字符串
    String result;
    if (hours > 0) {
      result =
          '$hours:${minutes.toString().padLeft(2, '0')}:${secs.toString().padLeft(2, '0')}';
    } else {
      result = '$minutes:${secs.toString().padLeft(2, '0')}';
    }
    if (needMs) {
      result = '$result.${milliseconds.toString().padLeft(3, '0')}';
    }
    return result;
  }

  //发送滑块信息，
  static void sendSliderMsg(String identifyParam, double value, bool isOsc) {
    final identifyPtr = string2cPtr(identifyParam);
    bindings.sendSliderValue(
      identifyPtr,
      value,
      isOsc ? 1 : 0,
    ); //dart没有bool转为int的运算符
    malloc.free(identifyPtr);
  }

  static String cPtr2String(Pointer<Char> ptr) {
    final dartString = ptr.cast<Utf8>().toDartString();
    bindings.freeString(ptr);
    return dartString;
  }

  static Pointer<Char> string2cPtr(String str) {
    final ptr = str.toNativeUtf8().cast<Char>();
    return ptr;
  }

  //获得组件中心点
  static Offset? getWidgetCenter(GlobalKey key) {
    final context = key.currentContext;
    if (context == null) return null;

    final box = context.findRenderObject() as RenderBox?;
    if (box == null || !box.hasSize) return null;

    // 中心点：局部坐标 (size.width/2, size.height/2) 转成全局坐标
    return box.localToGlobal(box.size.center(Offset.zero));
  }

  static String buildArtistsText(List<String>? l) {
    return l != null ? l.join(" / ") : "未知艺术家";
  }
}
