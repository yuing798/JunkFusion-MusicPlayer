import 'dart:async';
import 'dart:ffi' as dart_ffi;

import 'package:ffi/ffi.dart' as ffi;
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/bridge/dll/dllBridgeName.dart';
import 'package:junk_fusion_ui/bridge/dll/dll_invoke.dart';
import 'package:junk_fusion_ui/utils/global_key_defs.dart';
import 'package:junk_fusion_ui/utils/utils.dart';
import 'package:junk_fusion_ui/widgets/popup_window.dart';
import 'package:path_provider/path_provider.dart';

// provider 包：状态管理
// 需要在 pubspec.yaml 中添加: provider: ^6.1.2
import 'package:provider/provider.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:window_manager/window_manager.dart';

// 导入自己的文件（相对路径，不需要 package: 前缀）
import 'app.dart'; // 根组件
import 'providers/song_provider.dart';
import 'providers/playback_provider.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized(); //确保flutter绑定初始化
  await windowManager.ensureInitialized();
  WindowOptions options = const WindowOptions(
    minimumSize: Size(1450, 850),
    center: true,
    backgroundColor: Colors.transparent,
    skipTaskbar: false,
    titleBarStyle: TitleBarStyle.hidden, //隐藏原生的标题栏
  );
  // 4. 等待窗口准备好后再显示（防止应用启动时闪烁白屏）
  windowManager.waitUntilReadyToShow(options, () async {
    await windowManager.show();
    await windowManager.focus();
  });

  // print("准备初始化dll");

  final cacheDirPath = await getApplicationCacheDirectory();
  if (!await cacheDirPath.exists()) {
    cacheDirPath.createSync(recursive: true);
  }
  AppCache.cacheDirString = cacheDirPath.path;
  final cacheDirPtr = AppCache.cacheDirString
      .toNativeUtf8()
      .cast<dart_ffi.Char>();
  print("exe所在目录和缓存目录分别为：");
  print(AppCache.getExeDirectory());
  print(AppCache.cacheDirString);
  final exeDirPtr = AppCache.getExeDirectory()
      .toNativeUtf8()
      .cast<dart_ffi.Char>();
  bindings.dllInit(cacheDirPtr, exeDirPtr); //dll初始化
  ffi.malloc.free(cacheDirPtr);
  ffi.malloc.free(exeDirPtr);
  print("dll初始化完成");

  AppCache.frontCacheRef = await SharedPreferences.getInstance(); //初始化前端缓存指针

  // `runApp` 接收一个 Widget 参数，把它设为屏幕上显示的根 widget
  // Flutter 会接管该 widget 的生命周期和渲染

  final songProvider = SongProvider();
  songProvider.getAllSongs(); //全量获取歌曲元数据

  runApp(JunkFusionApp(songProvider: songProvider));
}

/// JunkFusionApp — 应用根 Widget
class JunkFusionApp extends StatelessWidget {
  //最外层，但没画布
  final SongProvider songProvider;
  const JunkFusionApp({super.key, required this.songProvider});

  @override
  Widget build(BuildContext context) {
    // MultiProvider — 同时注入多个 Provider 到 Widget Tree
    return MultiProvider(
      //数据层，没画布
      providers: [
        ChangeNotifierProvider<SongProvider>.value(value: songProvider),
        ChangeNotifierProvider(create: (_) => PlaybackProvider()),
      ],

      child: MaterialApp(
        //有画布,且overlay就在这里面
        title: 'JunkFusion',
        navigatorKey: navigatorKey,

        debugShowCheckedModeBanner: false, //关闭右上角 DEBUG 标签
        theme: ThemeData(
          fontFamily: "OpenSans",
          fontFamilyFallback: ["NotoSansSC"],
        ),
        home: const App(),
      ),
    );
  }
}
