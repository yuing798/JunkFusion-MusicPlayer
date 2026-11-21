/// 设置页的「目录（分节）」状态 + 跳转请求
///
/// ── 为什么需要一个跨组件的对象 ────────────────────────────────────────
/// 设置页是三段式里的两栏：
///   中间栏（widgets/nav/settings_sidebar.dart）画目录；
///   主内容区（widgets/pages/settings_page.dart）画所有分节 + 自己的滚动器。
/// 它们是**两棵不相干的子树**，点"插件管理"要让主内容区滚到那一段，
/// 只能靠一个共同的 ChangeNotifier —— 也就是这个类。
///
/// ── 请求-消费模型（重要）──────────────────────────────────────────────
/// 目录那边只调 [requestJump]（写一个待处理请求 + notify）；
/// 页面那边 addListener，收到通知后调 [takePending] 取走请求再滚。
/// **消费必须用 takePending（取完即清）**，否则每次 notify 都会重滚一遍：
/// 页面自己也调 [setActive]，那样会变成 notify → 滚 → notify 的死循环。
///
/// ── 以后加分节 ────────────────────────────────────────────────────────
/// 只改 [SettingsSection] 枚举（标题/图标在下面的 extension 里），
/// 目录和页面都是按 `SettingsSection.values` 自动生成的，别处不用动。
library;

import 'package:flutter/material.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

/// 设置页有哪些分节（同时也是中间栏目录的一项）
enum SettingsSection {
  /// 插件库：扫描结果表格 + 改类型
  pluginLibrary,

  // 以后加在这里，比如：
  // audioDevice,   // 声卡
  // appearance,    // 外观/主题
}

extension SettingsSectionX on SettingsSection {
  /// 目录里显示的名字；同时也是主内容区那一节的标题
  String get title => switch (this) {
    SettingsSection.pluginLibrary => '插件管理',
  };

  /// 目录项和分节标题前面的小图标
  IconData get icon => switch (this) {
    SettingsSection.pluginLibrary => TablerIcons.settings,
  };

  /// 目录项下面那行小字（说清楚这一节干什么）
  String get subtitle => switch (this) {
    SettingsSection.pluginLibrary => '扫描 VST3、管理插件库',
  };

  /// 锚点 key 的名字，调试时看得懂
  String get debugName => switch (this) {
    SettingsSection.pluginLibrary => 'pluginLibrary',
  };
}

class SettingsNavProvider extends ChangeNotifier {
  SettingsSection _active = SettingsSection.pluginLibrary;
  SettingsSection? _pending;

  /// 目录里高亮哪一项
  SettingsSection get active => _active;

  /// 目录被点击：登记一个"要跳到哪"的请求
  void requestJump(SettingsSection section) {
    _pending = section;
    _active = section;
    notifyListeners();
  }

  /// 主内容区取走待处理请求（取完即清，不 notify）
  SettingsSection? takePending() {
    final pending = _pending;
    _pending = null;
    return pending;
  }

  /// 主内容区主动同步高亮（以后做"滚动时跟随"用；现在只有一节，用处不大）
  void setActive(SettingsSection section) {
    if (_active == section) return;
    _active = section;
    notifyListeners();
  }
}
