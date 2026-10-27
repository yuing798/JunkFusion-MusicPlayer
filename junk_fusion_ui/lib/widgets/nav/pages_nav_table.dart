import 'package:flutter/material.dart';
import 'package:tabler_icons_plus/tabler_icons_plus.dart';

// ════════════════════════════════════════════════════════════════
// pages_nav_table.dart — 左侧图标栏的「导航段」表
//
// 三段式里的「一段」= 一个 navId。这张表同时决定两件事：
//   1. 左侧图标栏里这个位置画什么图标、叫什么名字；
//   2. 点它之后，中间栏画什么（sidebar）。
//
// 为什么单独一张表：以前图标按钮在 widgets/left_column.dart、navId 的含义
// 在 widgets/pages/pages_route.dart 的注释里，两处手工同步，很容易漂。
// 现在图标 + 中间栏都从这里来，图标栏（widgets/layout/icon_rail.dart）和
// 中间栏（widgets/layout/sidebar_host.dart）都读这一份。
//
// navId 和 pages_route.dart 的路由表一一对应，两边改 id 要一起改：
//   0 所有音乐   1 我喜欢   2 最近播放
//   3 专辑       4 歌单     5 文件夹    6 风格
//   7 AI助手     8 效果器   9 均衡器    10 音箱阵列   11 设置
//
// 用法：
//     PagesNavTable.groups          // 分组（图标栏分组之间画分隔线）
//     PagesNavTable.all             // 拍平后的全部段
//     PagesNavTable.byId(navId)     // navId → 段；null = 没有这一段
//
// 表里的 sidebar 目前全是 null（具体内容还没定），SidebarHost 会画占位。
// 以后某一页的中间栏想好了，就在对应段上补 sidebar： (context) => 组件()
// ════════════════════════════════════════════════════════════════

/// 一个导航段 = 左侧图标栏的一个图标 + 中间栏的一屏内容
class PagesNavSection {
  /// 和 pages_route.dart 里的 navId 一致
  final int id;

  /// 图标栏上画的图标
  final IconData icon;

  /// 名字：图标栏的 tooltip、中间栏占位的标题
  final String title;

  /// 中间栏内容；null = 还没设计，画占位
  final WidgetBuilder? sidebar;

  const PagesNavSection({
    required this.id,
    required this.icon,
    required this.title,
    this.sidebar,
  });
}

// ════════════════════════════════════════════════════════════════
// 导航段表
// ════════════════════════════════════════════════════════════════
abstract class PagesNavTable {
  /// 图标栏的分组；组和组之间会画一条分隔线
  static const List<List<PagesNavSection>> groups = [
    // ── 曲库浏览 ──
    [
      PagesNavSection(id: 0, icon: TablerIcons.music, title: '所有音乐'),
      PagesNavSection(id: 1, icon: TablerIcons.heart, title: '我喜欢'),
      PagesNavSection(id: 2, icon: TablerIcons.history, title: '最近播放'),
    ],

    // ── 曲库分类 ──
    [
      PagesNavSection(id: 3, icon: TablerIcons.album, title: '专辑'),
      PagesNavSection(id: 4, icon: TablerIcons.playlist, title: '歌单'),
      PagesNavSection(id: 5, icon: TablerIcons.folder, title: '文件夹'),
      PagesNavSection(id: 6, icon: TablerIcons.tag, title: '风格'),
    ],

    // ── 功能板块 ──
    [
      PagesNavSection(id: 7, icon: TablerIcons.robot, title: 'AI助手'),
      PagesNavSection(id: 8, icon: TablerIcons.waveSine, title: '效果器'),
      PagesNavSection(
        id: 9,
        icon: TablerIcons.adjustmentsHorizontal,
        title: '均衡器',
      ),
      PagesNavSection(id: 10, icon: TablerIcons.deviceSpeaker, title: '音箱阵列'),
      PagesNavSection(id: 11, icon: TablerIcons.settings, title: '设置'),
    ],
  ];

  /// 拍平后的全部段（按图标栏从上到下的顺序）
  static List<PagesNavSection> get all => [
    for (final group in groups) ...group,
  ];

  /// navId → 段；null / 找不到都返回 null（调用方画占位或空着）
  static PagesNavSection? byId(int? id) {
    if (id == null) return null;

    for (final group in groups) {
      for (final section in group) {
        if (section.id == id) return section;
      }
    }
    return null;
  }
}
