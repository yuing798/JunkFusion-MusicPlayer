import 'package:flutter/material.dart';
import '../theme/app_theme.dart';

class LeftColumn extends StatefulWidget {
  final void Function(int) onSelectionChanged;
  static const int initialSelectedId = 0;
  //final：值可以在运行时确定。比如 final time = DateTime.now(); —— 程序跑起来的那一刻才知道具体时间，赋值一次后就锁死了。
  // const：值必须在编译时就确定死。比如 const pi = 3.14159; —— 在代码写出来的时候，编译器就已经算好这个数字了

  const LeftColumn({super.key, required this.onSelectionChanged});
  //（定义构造函数）里看不到 参数名: 值 是完全正常的，因为参数名: 值 这种写法只出现在“调用”该构造函数的地方。
  //required:“调用函数时必须显式写出参数名并传值

  @override
  State<LeftColumn> createState() {
    return _LeftColumnState();
  }

  // 在 Flutter 里，State 对象专门负责两件 Vue 里由组件实例统一干的事：存动态数据和响应式刷新。
  // 如果用 Vue 3 的视角来看，State 对象就相当于 <script setup> 里的所有 ref、reactive 数据，
  //再加上 onMounted 和 onUnmounted 这些生命周期钩子。
}

//一个以 _（下划线）开头的类，意味着它是“库私有（Library Private）
class _LeftColumnState extends State<LeftColumn> {
  /// 当前选中的按钮 ID
  ///
  /// 对应原 Vue: const selectedId = ref(0)
  /// //late:这个变量我现在不初始化，但在我第一次使用它之前，绝对会赋值。出了事我负责
  /// 相当于!非空断言
  int _selectedId = 0;

  static const List<_NavSection> _sections = [
    _NavSection(
      label: '曲库浏览',
      buttons: [
        _NavButton(id: 0, text: '所有音乐'),
        _NavButton(id: 1, text: '我喜欢'),
        _NavButton(id: 2, text: '最近播放'),
      ],
    ),
    _NavSection(
      label: '分类浏览',
      buttons: [
        _NavButton(id: 3, text: '作者'),
        _NavButton(id: 4, text: '专辑'),
        _NavButton(id: 5, text: '歌单'),
        _NavButton(id: 6, text: '风格'),
      ],
    ),
    _NavSection(
      label: '功能板块',
      buttons: [
        _NavButton(id: 7, text: 'AI助手'),
        _NavButton(id: 8, text: '效果器'),
        _NavButton(id: 9, text: '均衡器'),
        _NavButton(id: 10, text: '音箱阵列'),
        _NavButton(id: 11, text: '设置'),
      ],
    ),
  ];

  /// 处理按钮点击（收音机行为：不允许取消选中）
  ///
  /// 对应原 Vue handleSelect(id)
  void _handleSelect(int id) {
    if (_selectedId != id) {
      setState(() => _selectedId = id);
      // 通知父组件
      widget.onSelectionChanged(id);
    }
  }

  @override
  Widget build(BuildContext context) {
    //作用：build 函数是 Flutter 的 “施工图绘制员”，它的唯一职责就是根据当前的数据（State）和配置（Widget）
    //，返回一棵你要显示在屏幕上的 Widget 树（比如返回一堆 Container、Row、Text 拼成的界面）
    // Container — 类似 HTML 的 <div>，可设置宽高、颜色、边距等
    return Container(
      width: 220,
      color: AppTheme.colorNav,

      // Scrollbar — 滚动条（thumb 颜色使用强调色）
      child: Scrollbar(
        thumbVisibility: true, // 始终显示滚动条滑块
        child: ListView(
          // `padding` 是列表整体内边距
          // `EdgeInsets.only(bottom: 100)` 仅底部留 100px（对应原 CSS padding-bottom: 100px）
          padding: const EdgeInsets.only(bottom: 100),

          children: [
            // ── Logo 区域 ──
            _buildLogoArea(),

            // ── 三个分组 ──
            for (final section in _sections) _buildSection(section),
          ],
        ),
      ),
    );
  }

  Widget _buildLogoArea() {
    return Container(
      height: 60,
      padding: const EdgeInsets.all(5),
      // `Center` widget：水平和垂直居中
      child: Image.asset(
        // td: 添加 Logo 资源文件到 pubspec.yaml 的 assets 中
        // 原 Vue 代码: import logoImage from '@/assets/image/junk-fusion.png'
        'assets/image/junk-fusion.png',

        // `fit: BoxFit.contain` 保持宽高比缩放，完整放入容器
        // 对应 CSS object-fit: contain
        fit: BoxFit.contain,
      ),
    );
  }

  Widget _buildSection(_NavSection section) {
    return Column(
      // `crossAxisAlignment: CrossAxisAlignment.stretch` 让子元素水平撑满
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        // --- 分组标签 ---
        _buildGroupLabel(section.label),

        // --- 分组按钮 ---
        for (final button in section.buttons) _buildNavButton(button),
      ],
    );
  }

  /// 构建分组标签
  Widget _buildGroupLabel(String label) {
    return Container(
      height: 50,
      alignment: Alignment.center,
      child: Text(label, style: AppTheme.bigTextStyle),
    );
  }

  /// 构建单个导航按钮
  Widget _buildNavButton(_NavButton button) {
    final isActive = _selectedId == button.id;

    // `GestureDetector` — 检测手势（点击、滑动等）的 widget
    // 对应 Vue 的 @click 事件绑定
    return GestureDetector(
      onTap: () => _handleSelect(button.id),
      child: Container(
        width: double.infinity, // 撑满父容器宽度
        height: 40,
        margin: const EdgeInsets.all(4),
        alignment: Alignment.center,

        // `AnimatedContainer` — 带动画的容器
        // 在属性改变时会自动过渡（duration + curve）
        // 对应 Vue CSS transition
        decoration: isActive ? _activeDecoration() : null,
        child: Text(
          button.text,
          style: AppTheme.midTextStyle.copyWith(
            fontWeight: isActive ? FontWeight.bold : FontWeight.normal,
          ),
        ),
      ),
    );
  }

  /// 构建激活态按钮的装饰（八角形 + 渐变 + 发光）
  ///
  /// 对应原 Vue .nav-button.active 及其 ::before / ::after 伪元素
  BoxDecoration _activeDecoration() {
    return BoxDecoration(
      // 渐变背景（对应 CSS background-image 多层渐变）
      gradient: LinearGradient(
        begin: Alignment.topLeft,
        end: Alignment.bottomRight,
        colors: [AppTheme.colorStress, AppTheme.colorSuperStress],
      ),
      // 外发光（对应 ::after 的 filter: blur(8px)）
      // Flutter 的 BoxShadow 可以直接模拟外发光
      boxShadow: [
        BoxShadow(
          color: AppTheme.colorStress.withAlpha(153), // 60% 透明度
          blurRadius: 8,
        ),
      ],
    );

    // 注意：八角形裁剪路径在实际运行时用 ClipPath 实现
    // 此处简化为圆角矩形，完整八角形实现见 _OctagonClipper
  }
}

class _NavSection {
  final String label;
  final List<_NavButton> buttons;

  const _NavSection({required this.label, required this.buttons});
}

/// _NavButton — 单个导航按钮数据
class _NavButton {
  final int id;
  //final代表第一次被赋值就变成了const
  final String text;

  const _NavButton({required this.id, required this.text});
  //const:这个构造函数生成的实例，必须是编译时就能确定的常量
}
