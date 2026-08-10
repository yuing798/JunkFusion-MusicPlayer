import 'dart:ffi' as dart_ffi;

import 'package:ffi/ffi.dart' as ffi;
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/bridge/dll/dllBridgeName.dart';
import 'package:junk_fusion_ui/bridge/dll/dll_invoke.dart';
import 'package:junk_fusion_ui/utils/utils.dart';
import 'package:junk_fusion_ui/widgets/info_window.dart';
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
    minimumSize: Size(1300, 850),
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

  print("准备初始化dll");
  final cacheDir = getApplicationCacheDirectory();

  final cacheDirPath = await cacheDir;
  if (!await cacheDirPath.exists()) {
    cacheDirPath.createSync(recursive: true);
  }
  AppCache.cacheDirString = cacheDirPath.path;
  // print(AppCache.cacheDirString);
  //C:\Users\sakuyayuing\AppData\Local\com.example\junk_fusion_ui
  final cPtr = AppCache.cacheDirString.toNativeUtf8().cast<dart_ffi.Char>();
  bindings.dllInit(cPtr); //dll初始化
  ffi.malloc.free(cPtr);

  AppCache.frontCacheRef = await SharedPreferences.getInstance();

  // sendDLLIsolateTask(B_dllInit.name, {});

  // `runApp` 接收一个 Widget 参数，把它设为屏幕上显示的根 widget
  // Flutter 会接管该 widget 的生命周期和渲染
  runApp(const JunkFusionApp());
}

/// JunkFusionApp — 应用根 Widget
class JunkFusionApp extends StatelessWidget {
  const JunkFusionApp({super.key});

  @override
  Widget build(BuildContext context) {
    // MultiProvider — 同时注入多个 Provider 到 Widget Tree
    return MultiProvider(
      providers: [
        // ChangeNotifierProvider 是 provider 包的核心 Widget
        // create: 延迟创建的回调，只在第一次需要时调用
        // `(_)` 中的下划线是 BuildContext（这里用不到所以不命名）
        ChangeNotifierProvider(create: (_) => SongProvider()),
        ChangeNotifierProvider(create: (_) => PlaybackProvider()),
      ],

      // `child` 是 Provider 包裹的子 widget
      child: MaterialApp(
        // `title`：应用标题（显示在任务管理器中）
        title: 'JunkFusion',
        navigatorKey: InfoWindow.navigatorKey,

        // `debugShowCheckedModeBanner`：关闭右上角 DEBUG 标签
        debugShowCheckedModeBanner: false,
        theme: ThemeData(
          fontFamily: "OpenSans",
          fontFamilyFallback: ["NotoSansSC"],
        ),
        // 关闭桌面端自带的拼写检查 — 否则中文全被标黄色双下划线
        // spellCheckConfiguration:
        //     SpellCheckConfiguration.disabled(),

        // `home`：应用的首页 widget
        // `const App()` 创建 App widget 的编译期常量实例
        home: const App(),
      ),
    );
  }
}
