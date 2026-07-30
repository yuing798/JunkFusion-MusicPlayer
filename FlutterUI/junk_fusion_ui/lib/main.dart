/// ════════════════════════════════════════════════════════════════
/// main.dart — 应用入口文件
///
/// 对应原 Vue 项目 main.ts：
///   import { createApp } from 'vue'
///   import App from './App.vue'
///   import { createPinia } from 'pinia'
///   const pinia = createPinia()
///   createApp(App).use(pinia).mount('#app')
///
/// Dart/Flutter 入口概念：
/// - `main()` 函数是 Dart 程序的入口点（相当于 C 的 main() / JS 的全局脚本）
/// - `void main()` 表示 main 函数没有返回值
/// - `runApp()` 是 Flutter 的函数，接收一个 Widget 并将其填满整个屏幕
/// - `MyApp` 继承自 `StatelessWidget`（无状态组件），是 Flutter 的根 widget
///
/// Provider 注入：
/// - `MultiProvider` 是 provider 包的 widget，可以同时注入多个 ChangeNotifier
/// - `ChangeNotifierProvider` 包装单个 ChangeNotifier 使其在 Widget Tree 中可用
/// - `create: (_) => SongProvider()` 延迟创建 Provider 实例
/// - `_` 是 Dart 的惯用写法，表示"这个参数我不关心"
///   这里它接收 BuildContext，但在 create 中用不到
/// ════════════════════════════════════════════════════════════════

// `import` 引入依赖
// `package:flutter/material.dart` — Flutter Material Design 组件库
import 'package:flutter/material.dart';

// provider 包：状态管理
// 需要在 pubspec.yaml 中添加: provider: ^6.1.2
import 'package:provider/provider.dart';

// 导入自己的文件（相对路径，不需要 package: 前缀）
import 'app.dart'; // 根组件
import 'providers/song_provider.dart';
import 'providers/playback_provider.dart';
import 'theme/app_theme.dart';

/// main() — 程序入口函数
///
/// Dart 语法说明：
/// - `void main()` 返回类型 void + 函数名 + 参数列表（这里为空）
/// - `runApp(...)` 启动 Flutter 应用的函数
/// - 花括号 `{}` 定义函数体（和 C/Java/JS 一样）
void main() {
  // `runApp` 接收一个 Widget 参数，把它设为屏幕上显示的根 widget
  // Flutter 会接管该 widget 的生命周期和渲染
  runApp(const JunkFusionApp());
  // 根 Widget（Root Widget），就是你传给 runApp() 函数的那个最顶层的 Dart 类。
  //它是整棵 Widget 树（Widget Tree） 的“树根”，所有你在屏幕上看到的按钮、文字、输入框、列表，都是这个“树根”上长出的枝叶。
}

/// JunkFusionApp — 应用根 Widget
///
/// 对应原 Vue: createApp(App).use(pinia).mount('#app')
///
/// Dart 语法说明：
/// - `class A extends B`：A 继承 B（单继承，Dart 不支持多重继承）
/// - `StatelessWidget`：无状态的 Widget，所有数据来自外部（immutable）
///   等价于 Vue 中只有 props 没有 data/computed 的组件
/// - `const` 构造函数：表示可以在编译期创建该类的常量实例，
///   有助于 Flutter 的性能优化
/// - `super.key`：把 key 参数传给父类 StatelessWidget 的构造函数
class JunkFusionApp extends StatelessWidget {
  /// const 构造函数
  ///
  /// `{super.key}` 语法糖：把命名参数 key 直接传递给父类
  /// 等价于手动写: JunkFusionApp({Key? key}) : super(key: key);
  const JunkFusionApp({super.key});

  /// build() 方法 — Widget 的核心
  ///
  /// 每个 Widget 都必须实现 build()，返回一个 Widget 子树
  /// Flutter 框架在需要时调用 build()（状态改变、父级重建等）
  ///
  /// Dart 语法：
  /// - `@override` 注解表示覆盖父类方法（虽然不是必须但强烈推荐）
  /// - `Widget` 是返回类型
  /// - `BuildContext context`：构建上下文，包含了 Widget 在树中的位置信息
  @override
  Widget build(BuildContext context) {
    // MultiProvider — 同时注入多个 Provider 到 Widget Tree
    //
    // 等价于 Vue 的:
    //   const pinia = createPinia()
    //   createApp(App).use(pinia).mount('#app')
    //
    // 原理：MultiProvider 在 Widget Tree 的根部注入 Provider，
    // 其下的任何 Widget 都能通过 context.read/watch 访问这些 Provider
    return MultiProvider(
      // `providers` 参数接受一个列表
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
        title: 'Junk Fusion',

        // `theme`：Material Design 主题
        theme: AppTheme.lightTheme,

        // `debugShowCheckedModeBanner`：关闭右上角 DEBUG 标签
        debugShowCheckedModeBanner: false,

        // `home`：应用的首页 widget
        // `const App()` 创建 App widget 的编译期常量实例
        home: const App(),
      ),
    );
  }
}

/// ════════════════════════════════════════════════════════════════
/// 关键概念：
///
/// Flutter 中一切都是 Widget：
/// - StatelessWidget：无内部可变状态，数据完全由外部传入
/// - StatefulWidget：有内部可变状态，通过 setState 触发重建
/// - InheritedWidget：沿 widget tree 向下传递数据（provider 的底层原理）
///
/// Provider 的工作流程：
/// 1. ChangeNotifierProvider 创建并持有 ChangeNotifier 实例
/// 2. 子 Widget 通过 context.watch<T>() 获取实例并订阅
/// 3. ChangeNotifier.notifyListeners() 触发所有订阅者重建
/// ════════════════════════════════════════════════════════════════
