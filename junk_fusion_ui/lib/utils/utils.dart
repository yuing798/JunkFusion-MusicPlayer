import 'dart:io';
import 'dart:ffi' as ffi;

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

  static final ffi.DynamicLibrary libPath = () {
    final libName = Platform.isWindows
        ? "JunkFusionDLL.dll"
        : Platform.isMacOS
        ? "JunkFusionDLL.dylib"
        : "JunkFusionDLL.so";

    return ffi.DynamicLibrary.open(
      "${getExeDirectory()}${Platform.pathSeparator}$libName",
    );
  }();
}
