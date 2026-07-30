/// ════════════════════════════════════════════════════════════════
/// page_change.dart — 分页组件
///
/// 对应原 Vue 项目 components/other/pageChange.vue
/// 对应 C++ PageChange 类
///
/// 布局：
///   第 1 行：页码导航（< 上一页, 1, 2, ..., N, > 下一页）
///   第 2 行：跳转行（输入框 + go 按钮）
///   总尺寸：360×80
///
/// 页码规则（与 C++ PageChange::doLayout() 一致）：
///   ≤ 7 页 → 全部显示
///   > 7 页 → [1] + 中间 5 槽 + [N]，根据 currentPage 动态计算窗口
///
/// Dart 语法说明：
/// - `typedef` 定义函数类型别名
/// - `sealed class`（Dart 3.x）封闭类：所有子类必须在同一文件中
///   用于实现"联合类型"（union type）的效果
/// - `ListView` horizontal 实现水平滚动
/// ════════════════════════════════════════════════════════════════

import 'package:flutter/material.dart';
import '../theme/app_theme.dart';

/// 页码变化回调类型
///
/// Dart 语法：
/// - `typedef` 定义类型别名（类似 C++ 的 using / TypeScript 的 type）
///   `typedef PageChangeCallback = void Function(int page);`
typedef PageChangeCallback = void Function(int page);

/// PageChange — 分页组件
///
/// 参数：
/// - `totalPages`：总页码数
/// - `currentPage`：当前选中页（从 1 开始）
/// - `onPageChange`：页码变化回调
class PageChange extends StatelessWidget {
  final int totalPages;
  final int currentPage;
  final PageChangeCallback onPageChange;

  const PageChange({
    super.key,
    required this.totalPages,
    required this.currentPage,
    required this.onPageChange,
  });

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: 360,
      height: 80,
      child: Column(
        children: [
          // --- 第 1 行：页码导航 ---
          _PageRow(
            totalPages: totalPages,
            currentPage: currentPage,
            onPageChange: onPageChange,
          ),
          // --- 第 2 行：跳转行 ---
          _JumpRow(
            totalPages: totalPages,
            currentPage: currentPage,
            onPageChange: onPageChange,
          ),
        ],
      ),
    );
  }
}

/// _PageRow — 页码导航行
///
/// 包含：< 上一页、页码按钮/省略号、> 下一页
class _PageRow extends StatelessWidget {
  final int totalPages;
  final int currentPage;
  final PageChangeCallback onPageChange;

  const _PageRow({
    required this.totalPages,
    required this.currentPage,
    required this.onPageChange,
  });

  @override
  Widget build(BuildContext context) {
    final isPrevDisabled = currentPage <= 1;
    final isNextDisabled = currentPage >= totalPages;

    return SizedBox(
      height: 40,
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          // < 上一页按钮
          _PageButton(
            label: '<',
            enabled: !isPrevDisabled,
            onTap: isPrevDisabled ? null : () => onPageChange(currentPage - 1),
          ),

          // 页码 / 省略号列表
          ..._buildPageItems().map((item) {
            // `switch` 配合 sealed class 实现类型安全的模式匹配
            return switch (item) {
              PageItemPage(:final page) => _PageButton(
                  label: '$page',
                  isActive: page == currentPage,
                  onTap: () => onPageChange(page),
                ),
              PageItemEllipsis() => const SizedBox(
                  width: 40,
                  height: 40,
                  child: Center(
                    child: Text('...', style: AppTheme.midTextStyle),
                  ),
                ),
            };
          }),

          // > 下一页按钮
          _PageButton(
            label: '>',
            enabled: !isNextDisabled,
            onTap: isNextDisabled ? null : () => onPageChange(currentPage + 1),
          ),
        ],
      ),
    );
  }

  /// 构建页码按钮列表（含省略号）
  ///
  /// 对应原 Vue pageItems computed
  ///
  /// Dart 语法：
  /// - `List<PageItem>` 返回类型，PageItem 是 sealed class
  /// - 算法逻辑与原 Vue 完全一致
  List<PageItem> _buildPageItems() {
    final n = totalPages;
    final cur = currentPage;

    if (n <= 0) return [];
    if (n <= 7) {
      // 全部显示（1 到 n）
      return List.generate(n, (i) => PageItemPage(page: i + 1));
    }

    // > 7 页：[1] + 中间窗口 + [N]
    final List<PageItem> items = [const PageItemPage(page: 1)];

    int winStart, winEnd;

    if (cur <= 3) {
      // 靠近首页
      winStart = 2;
      winEnd = 5;
    } else if (cur >= n - 2) {
      // 靠近末页
      winEnd = n - 1;
      winStart = n - 4;
    } else {
      // 中间
      winStart = cur - 1;
      winEnd = cur + 1;
    }

    if (winStart > 2) {
      items.add(const PageItemEllipsis());
    }

    for (int p = winStart; p <= winEnd; ++p) {
      items.add(PageItemPage(page: p));
    }

    if (winEnd < n - 1) {
      items.add(const PageItemEllipsis());
    }

    items.add(PageItemPage(page: n));

    return items;
  }
}

/// _JumpRow — 跳转输入行
///
/// 包含：80px spacer + 输入框 + go 按钮
class _JumpRow extends StatefulWidget {
  final int totalPages;
  final int currentPage;
  final PageChangeCallback onPageChange;

  const _JumpRow({
    required this.totalPages,
    required this.currentPage,
    required this.onPageChange,
  });

  @override
  State<_JumpRow> createState() => _JumpRowState();
}

class _JumpRowState extends State<_JumpRow> {
  /// 输入框控制器
  ///
  /// TextEditingController 用于读取/清空 TextField 的内容
  final TextEditingController _controller = TextEditingController();

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  /// 点击 go 或按回车
  void _go() {
    final text = _controller.text.trim();
    if (text.isEmpty) return;

    // `int.tryParse` 尝试解析字符串为整数，失败返回 null
    final targetPage = int.tryParse(text);
    if (targetPage == null) return;

    if (targetPage >= 1 &&
        targetPage <= widget.totalPages &&
        targetPage != widget.currentPage) {
      _controller.clear();
      widget.onPageChange(targetPage);
    }
  }

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      height: 40,
      child: Row(
        children: [
          // 左侧 spacer（80px）
          const SizedBox(width: 80),

          // 输入框
          SizedBox(
            width: 160,
            height: 30,
            child: TextField(
              controller: _controller,
              // `onSubmitted` 在用户按回车时触发
              onSubmitted: (_) => _go(),
              decoration: InputDecoration(
                hintText: '跳转到...',
                hintStyle: AppTheme.littleTextStyle.copyWith(
                  color: AppTheme.colorTextSecond.withAlpha(153),
                ),
                contentPadding:
                    const EdgeInsets.symmetric(horizontal: 8),
                border: OutlineInputBorder(
                  borderRadius:
                      BorderRadius.circular(AppTheme.borderRadius),
                  borderSide: const BorderSide(
                      color: AppTheme.colorEdge, width: 3),
                ),
                enabledBorder: OutlineInputBorder(
                  borderRadius:
                      BorderRadius.circular(AppTheme.borderRadius),
                  borderSide: const BorderSide(
                      color: AppTheme.colorEdge, width: 3),
                ),
                focusedBorder: OutlineInputBorder(
                  borderRadius:
                      BorderRadius.circular(AppTheme.borderRadius),
                  borderSide: const BorderSide(
                      color: AppTheme.colorStress, width: 3),
                ),
                isDense: true,
              ),
              style: AppTheme.midTextStyle,
            ),
          ),

          const SizedBox(width: 5),

          // go 按钮
          _PageButton(
            label: 'go',
            onTap: _go,
          ),
        ],
      ),
    );
  }
}

/// _PageButton — 单个页码按钮
///
/// 40×40 按钮，带悬浮和激活态
class _PageButton extends StatelessWidget {
  final String label;
  final VoidCallback? onTap; // VoidCallback? = void Function()? 可空回调
  final bool isActive;
  final bool enabled;

  const _PageButton({
    required this.label,
    this.onTap,
    this.isActive = false,
    this.enabled = true,
  });

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: enabled ? onTap : null,
      child: Container(
        width: 40,
        height: 40,
        margin: const EdgeInsets.all(2),
        alignment: Alignment.center,
        decoration: BoxDecoration(
          color: isActive
              ? AppTheme.colorTextSecond // 激活态：深灰背景
              : AppTheme.colorMain, // 默认态：主背景色
          borderRadius: BorderRadius.circular(AppTheme.borderRadius),
        ),
        child: Text(
          label,
          style: AppTheme.midTextStyle.copyWith(
            color: isActive
                ? Colors.white // 激活态：白色文字
                : (enabled
                    ? AppTheme.colorTextMain
                    : AppTheme.colorTextSecond.withAlpha(77)), // 禁用态
          ),
        ),
      ),
    );
  }
}

// ════════════════════════════════════════════════════════════════
// 页码项的类型定义（sealed class — 联合类型）
//
// Dart 3 引入的 sealed class 允许你在一组有限的子类型之间
// 做穷尽检查（exhaustive checking）。switch 表达式必须覆盖
// 所有子类型，否则编译器报错。
//
// 这等价于 TypeScript 的 discriminated union：
//   type PageItem = { type: 'page'; page: number } | { type: 'ellipsis' };
// ════════════════════════════════════════════════════════════════

/// 页码项的基类
///
/// Dart 语法：
/// - `sealed class` 封闭类，所有子类必须在同一文件中定义
/// - 编译器可以确认所有子类型已覆盖（exhaustiveness check）
sealed class PageItem {
  const PageItem();
}

/// 页码类型 — 显示数字按钮
class PageItemPage extends PageItem {
  final int page;
  const PageItemPage({required this.page});
}

/// 省略号类型 — 显示 "..."
class PageItemEllipsis extends PageItem {
  const PageItemEllipsis();
}

/// ════════════════════════════════════════════════════════════════
/// Dart sealed class 对比 TS discriminated union：
///
/// TypeScript:
///   type PageItem =
///     | { type: 'page'; page: number }
///     | { type: 'ellipsis' };
///
/// Dart:
///   sealed class PageItem {}
///   class PageItemPage extends PageItem { final int page; ... }
///   class PageItemEllipsis extends PageItem {}
///
/// 使用时（Dart 3 switch 表达式）：
///   switch (item) {
///     PageItemPage(:final page) => print(page),
///     PageItemEllipsis() => print('...'),
///   }
/// ════════════════════════════════════════════════════════════════
