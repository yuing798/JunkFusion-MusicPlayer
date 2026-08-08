import 'package:flutter/material.dart';

// provider 包：状态管理
// 需要在 pubspec.yaml 中添加: provider: ^6.1.2
import 'package:provider/provider.dart';
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

        // `debugShowCheckedModeBanner`：关闭右上角 DEBUG 标签
        debugShowCheckedModeBanner: false,
        theme: ThemeData(
          fontFamily: "OpenSans",
          fontFamilyFallback: ["NotoSansSC"],
        ),

        // `home`：应用的首页 widget
        // `const App()` 创建 App widget 的编译期常量实例
        home: const App(),
      ),
    );
  }
}
