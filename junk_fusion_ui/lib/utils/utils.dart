import 'dart:io';
import 'dart:ffi' as ffi;

import 'package:ffi/ffi.dart';
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/bridge/dll_invoke.dart';
import 'package:path_provider/path_provider.dart';
import 'package:shared_preferences/shared_preferences.dart';

class AppCache {
  static late SharedPreferences frontCacheRef; //前端UI缓存
  static late String cacheDirString; //其他缓存

  static String getExeDirectory() {
    // 1. 获取 exe 的绝对路径（解析符号链接）
    final String exePath = Platform.resolvedExecutable;
    // 2. 获取该文件所在的父级目录
    final Directory exeDir = File(exePath).parent;
    return exeDir.path;
  }

  static const double defaultWindowWidth = 1300.0;
  static const double defaultWindowHeight = 700.0;
  static const defaultWindowSize = Size(
    defaultWindowWidth,
    defaultWindowHeight,
  );
}

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

  static String cPtr2String(ffi.Pointer<ffi.Char> ptr) {
    final dartString = ptr.cast<Utf8>().toDartString();
    bindings.freeString(ptr);
    return dartString;
  }

  static ffi.Pointer<ffi.Char> string2cPtr(String str) {
    final ptr = str.toNativeUtf8().cast<ffi.Char>();
    return ptr;
  }
}
