import 'dart:async';
import 'dart:ffi' as dart_ffi;

import 'package:ffi/ffi.dart' as ffi;
import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/bridge/cpp_func_manager.dart';
import 'package:junk_fusion_ui/bridge/dll_invoke.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/utils/global_key_defs.dart';
import 'package:junk_fusion_ui/utils/utils.dart';
import 'package:junk_fusion_ui/widgets/popup_window.dart';
import 'package:path_provider/path_provider.dart';

// provider 包：状态管理
// 需要在 pubspec.yaml 中添加: provider: ^6.1.2
import 'package:provider/provider.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:tray_manager/tray_manager.dart';
import 'package:window_manager/window_manager.dart';

import 'app.dart'; // 根组件
import 'providers/song_provider.dart';
import 'providers/playback_provider.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized(); //确保flutter绑定初始化
  await windowManager.ensureInitialized();

  // 【必加】告诉底层：点关闭按钮时，不要杀进程，交给我(onWindowClose)来处理！
  await windowManager.setPreventClose(true);

  WindowOptions options = const WindowOptions(
    // size: AppCache.defaultWindowSize,
    minimumSize: AppCache.defaultWindowSize,
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
  final cacheDirPtr = UtilFunction.string2cPtr(AppCache.cacheDirString);
  // print("exe所在目录和缓存目录分别为：");
  // print(AppCache.getExeDirectory());
  // print(AppCache.cacheDirString);
  final exeDirPtr = UtilFunction.string2cPtr(AppCache.getExeDirectory());
  bindings.dllInit(cacheDirPtr, exeDirPtr); //dll初始化
  ffi.malloc.free(cacheDirPtr);
  ffi.malloc.free(exeDirPtr);

  AppCache.frontCacheRef = await SharedPreferences.getInstance(); //初始化前端缓存指针
  AppCache.appDocDir = (await getApplicationDocumentsDirectory()).path;

  final songProvider = SongProvider();
  final playbackProvider = PlaybackProvider(songProvider: songProvider);

  runApp(
    JunkFusionApp(
      songProvider: songProvider,
      playbackProvider: playbackProvider,
    ),
  ); //runApp不是阻塞式的，所以下面不能放析构逻辑
}

class JunkFusionApp extends StatefulWidget {
  final SongProvider songProvider;
  final PlaybackProvider playbackProvider;
  const JunkFusionApp({
    super.key,
    required this.songProvider,
    required this.playbackProvider,
  });

  @override
  State<StatefulWidget> createState() {
    return JunkFusionAppState();
  }
}

/// JunkFusionApp — 应用根 Widget
class JunkFusionAppState extends State<JunkFusionApp>
    with WindowListener, TrayListener {
  final cppCallbackManager = CppCallbackManager();

  @override
  void initState() {
    super.initState();
    windowManager.addListener(this);
    trayManager.addListener(this);
    cppCallbackManager.setupCallbacks(
      widget.songProvider,
      widget.playbackProvider,
    );
    _initSystemTray();

    WidgetsBinding.instance.addPostFrameCallback((_) {
      _loadWindowState();
      widget.songProvider.getAllSongs();
      widget.playbackProvider.initPlaybackProvider();
    });
  }

  @override
  void dispose() {
    //dispose的执行时机不可靠，必须使用onWindowClose()来监听整个应用的关闭
    trayManager.removeListener(this);
    windowManager.removeListener(this);
    _saveWindowState();
    super.dispose();
  }

  Future<void> _saveWindowState() async {
    if (await windowManager.isMinimized()) return;
    //保存窗口是否处于最大化状态
    final isMaximized = await windowManager.isMaximized();
    await AppCache.frontCacheRef.setBool('windowMaximized', isMaximized);

    if (!isMaximized) {
      //只有在没有最大化的时候才保存尺寸和位置，放置将全屏尺寸误存为普通尺寸
      final size = await windowManager.getSize();
      final position = await windowManager.getPosition();

      await AppCache.frontCacheRef.setDouble('windowWidth', size.width);
      await AppCache.frontCacheRef.setDouble('windowHeight', size.height);
      await AppCache.frontCacheRef.setDouble('windowX', position.dx);
      await AppCache.frontCacheRef.setDouble('windowY', position.dy);
    }
  }

  Future<void> _loadWindowState() async {
    if (AppCache.frontCacheRef.getBool('windowMaximized') == true) {
      await windowManager.maximize();
      return;
    }
    final width =
        AppCache.frontCacheRef.getDouble('windowWidth') ??
        AppCache.defaultWindowWidth;
    final height =
        AppCache.frontCacheRef.getDouble('windowHeight') ??
        AppCache.defaultWindowHeight;
    final x = AppCache.frontCacheRef.getDouble('windowX');
    final y = AppCache.frontCacheRef.getDouble('windowY');

    await windowManager.setSize(Size(width, height));

    // 如果位置信息存在，则恢复位置
    if (x != null && y != null) {
      await windowManager.setPosition(Offset(x, y));
    } else {
      // 如果没有保存位置，让窗口居中
      await windowManager.center();
    }
  }

  // 初始化托盘的方法
  Future<void> _initSystemTray() async {
    // 设置托盘图标
    await trayManager.setIcon("assets/image/JunkFusion.ico");

    await trayManager.setToolTip("Junk Fusion");

    // 构建右键菜单
    Menu menu = Menu(
      items: [
        MenuItem(key: 'show_window', label: '显示主界面'),
        MenuItem.separator(), // 分割线
        MenuItem(key: 'exit_app', label: '完全退出程序'),
      ],
    );
    await trayManager.setContextMenu(menu);
  }

  // 拦截所有的窗口关闭请求 (无论是按 X 还是 Alt+F4)
  @override
  void onWindowClose() async {
    // print("准备关闭窗口...");
    _saveWindowState();

    bool isPreventClose = await windowManager.isPreventClose();
    if (isPreventClose) {
      // print("点击了关闭，窗口最小化到托盘...");
      await windowManager.hide(); // 隐藏窗口，进程继续在后台运行
    }
  }

  @override
  void onWindowMove() {
    _saveWindowState();

    super.onWindowMove();
  }

  // 鼠标左键单击托盘图标：恢复显示窗口
  @override
  void onTrayIconMouseDown() {
    windowManager.show();
    windowManager.focus(); // 聚焦到最前面
    _loadWindowState();
  }

  // 鼠标右击托盘图标：(tray_manager 默认会自动弹出刚才设置的菜单，无需手动写代码)
  @override
  void onTrayIconRightMouseDown() {
    trayManager.popUpContextMenu(); // 部分系统需要手动调用这行
  }

  // 监听菜单项的点击
  @override
  void onTrayMenuItemClick(MenuItem menuItem) {
    if (menuItem.key == 'show_window') {
      windowManager.show();
      windowManager.focus();
      _loadWindowState();
    } else if (menuItem.key == 'exit_app') {
      // 托盘右键点击了退出，执行终极清理！
      _safeExit();
    }
  }

  // ==================== 终极安全退出协议 ====================

  Future<void> _safeExit() async {
    // print("托盘触发退出，开始安全清理系统...");

    _saveWindowState();
    widget.playbackProvider.saveState();
    // 2. 释放 Dart 端内存
    cppCallbackManager.dispose();
    // 3. 关闭 C++ 后端
    bindings.closeBackend();
    // 4. 清理托盘图标
    await trayManager.destroy();
    // 5. 彻底干掉进程
    await windowManager.destroy();
  }

  @override
  Widget build(BuildContext context) {
    // MultiProvider — 同时注入多个 Provider 到 Widget Tree
    return MultiProvider(
      //数据层，没画布
      providers: [
        ChangeNotifierProvider<SongProvider>.value(value: widget.songProvider),
        ChangeNotifierProvider<PlaybackProvider>.value(
          value: widget.playbackProvider,
        ),
        ChangeNotifierProvider(create: (_) => AppTheme()),
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
