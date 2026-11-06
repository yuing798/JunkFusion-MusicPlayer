/// 均衡器页面（主内容区）
///
/// ── 布局 ──────────────────────────────────────────────────────────────
///   ┌ 顶栏：EQ 总开关 | 输出增益 | 恢复默认 ─────────────────────────┐
///   ├ 曲线区：网格 + 频响曲线 + 8 个可拖的圆点（eq_curve_view.dart） ┤
///   └ 底栏 inspector：选中那一段的参数（类型/频率/增益/Q或斜率/斜率/开关）┘
///
/// 中间栏（预设列表）不在这里，是 widgets/nav/eq_preset_sidebar.dart，
/// 由 PagesNavTable 挂在 navId = 9 上。
///
/// ── 数据从哪来 ────────────────────────────────────────────────────────
/// 全部走 EqProvider（→ FFI → Coordinator）。这个页面不直接碰 ffi。
///
/// ── 频谱背景 ──────────────────────────────────────────────────────────
/// 预留：EqCurveView 支持注入 SpectrumSource，本轮没有实现数据通路，
/// 所以这里先不传（画布不画背景）。
library;

import 'package:flutter/material.dart';
import 'package:junk_fusion_ui/model/eq_param.dart';
import 'package:junk_fusion_ui/providers/eq_provider.dart';
import 'package:junk_fusion_ui/providers/playback_provider.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/widgets/helper_widget/combo_box.dart';
import 'package:junk_fusion_ui/widgets/helper_widget/knob_control.dart';
import 'package:junk_fusion_ui/widgets/pages/eq_curve_view.dart';
import 'package:provider/provider.dart';

class EqViewPage extends StatefulWidget {
  const EqViewPage({super.key});

  /// 底部播放栏占掉的高度
  ///
  /// PlayBar 是浮在整个窗口最上面的（进度条和时域谱必须悬浮），所以内容区
  /// 最下面这么多像素会被盖住。这里预留出来，否则底部 inspector 会被压住。
  /// TODO: 这个 100 和 widgets/PlayControl/play_bar.dart 里的 `_height` 是同一个值，
  ///       以后抽成一个共享常量更稳妥。
  static const double playBarHeight = 100;

  @override
  State<EqViewPage> createState() => _EqViewPageState();
}

class _EqViewPageState extends State<EqViewPage> {
  @override
  void initState() {
    super.initState();

    // 进页面时拉一次后端状态 + 预设列表。
    // 放在 postFrame 里：ensureInitialized 会 notifyListeners，
    // 在 initState 里直接通知会在首帧 build 期间触发重建。
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted) return;
      context.read<EqProvider>().ensureInitialized();
    });
  }

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<EqProvider>();
    final theme = context.watch<AppTheme>();

    // 只在"有没有歌"这件事变化时重建；播放进度每秒几十次的通知不要传到这一页
    final hasSong = context.select<PlaybackProvider, bool>(
      (playback) => playback.curBasicInfo != null,
    );

    return Padding(
      // 有歌时播放栏才滑进来，所以底部留白也跟着走
      padding: EdgeInsets.only(
        bottom: hasSong ? EqViewPage.playBarHeight : 0,
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          _buildTopBar(provider, theme),
          const SizedBox(height: 8),

          // TODO(频谱): 以后后端把频谱数据通路做好，这里传进去就能画背景
          const Expanded(child: EqCurveView()),

          const SizedBox(height: 8),
          _buildInspector(provider, theme),
        ],
      ),
    );
  }

  // ── 顶栏 ──────────────────────────────────────────────────────────
  Widget _buildTopBar(EqProvider provider, AppTheme theme) {
    return SizedBox(
      height: 40,
      child: Row(
        children: [
          Text('均衡器', style: theme.bigTextStyle),
          const SizedBox(width: 16),

          // EQ 总开关
          Switch(
            value: provider.state.enabled,
            onChanged: provider.setEqEnabled,
          ),
          Text('启用', style: theme.littleTextStyle),

          const SizedBox(width: 20),

          // 输出增益（拖动中只更新本地，松手才下发）
          SizedBox(
            width: 180,
            child: Row(
              children: [
                Text('输出', style: theme.littleTextStyle),
                Expanded(
                  child: SliderTheme(
                    data: _compactSliderTheme,
                    child: Slider(
                      value: provider.state.masterGainDb.clamp(-18.0, 18.0),
                      min: -18,
                      max: 18,
                      onChanged: provider.setMasterGain,
                      onChangeEnd: (value) =>
                          provider.setMasterGain(value, commit: true),
                    ),
                  ),
                ),
                SizedBox(
                  width: 46,
                  child: Text(
                    _formatGain(provider.state.masterGainDb),
                    style: theme.littleTextStyle,
                    textAlign: TextAlign.right,
                  ),
                ),
              ],
            ),
          ),

          const Spacer(),

          TextButton.icon(
            onPressed: provider.resetAll,
            icon: const Icon(Icons.restart_alt, size: 18),
            label: const Text('恢复默认'),
          ),
        ],
      ),
    );
  }

  // ── 底栏 inspector ────────────────────────────────────────────────
  Widget _buildInspector(EqProvider provider, AppTheme theme) {
    final selected = provider.selectedBand;

    return Container(
      height: 84,
      decoration: BoxDecoration(
        color: theme.colorNav,
        borderRadius: BorderRadius.circular(6),
        border: Border.all(color: theme.colorEdge, width: 1),
      ),
      padding: const EdgeInsets.symmetric(horizontal: 8),
      child: Row(
        children: [
          // 8 个段号按钮：关掉的段只能从这里选中
          _BandSelector(provider: provider, theme: theme),
          const VerticalDivider(width: 16, indent: 12, endIndent: 12),

          Expanded(
            child: selected == null
                ? Center(
                    child: Text(
                      '点曲线上的圆点编辑那一段；双击空白处新建一段',
                      style: theme.littleTextStyle,
                    ),
                  )
                : _buildBandEditor(provider, theme, selected),
          ),
        ],
      ),
    );
  }

  Widget _buildBandEditor(EqProvider provider, AppTheme theme, int index) {
    final band = provider.bands[index];

    return SingleChildScrollView(
      scrollDirection: Axis.horizontal,
      child: Row(
        children: [
          // 这一段的开关
          Switch(
            value: band.enabled,
            onChanged: (value) => provider.setBandEnabled(index, value),
          ),

          _buildTypeDropdown(provider, band),

          const SizedBox(width: 10),

          // 频率：对数旋钮（线性的话 1kHz 会挤在最左边 5% 的位置）
          KnobControl(
            label: '频率',
            value: band.freqHz,
            min: EqCurveView.minHz,
            max: EqCurveView.maxHz,
            defaultValue: 1000,
            curve: KnobCurve.logarithmic,
            valueText: _formatFreq(band.freqHz),
            enabled: band.enabled,
            onChanged: (value) =>
                provider.dragBandLive(index, freqHz: value),
            onChangeEnd: (_) => provider.commitBand(index),
          ),

          if (band.type.usesGain)
            KnobControl(
              label: '增益',
              value: band.gainDb,
              min: EqCurveView.minDb,
              max: EqCurveView.maxDb,
              defaultValue: 0,
              valueText: _formatGain(band.gainDb),
              enabled: band.enabled,
              onChanged: (value) =>
                  provider.dragBandLive(index, gainDb: value),
              onChangeEnd: (_) => provider.commitBand(index),
            ),

          if (band.type.usesQ)
            KnobControl(
              label: 'Q',
              value: band.q,
              min: 0.1,
              max: 10.0,
              defaultValue: 0.70710678,
              curve: KnobCurve.logarithmic,
              valueText: band.q.toStringAsFixed(2),
              enabled: band.enabled,
              onChanged: (value) => provider.dragBandLive(index, q: value),
              onChangeEnd: (_) => provider.commitBand(index),
            ),

          if (band.type.usesSlope)
            KnobControl(
              label: '斜率',
              value: band.shelfSlope,
              min: 0.1,
              max: 1.0,
              defaultValue: 1.0,
              valueText: '${(band.shelfSlope * 100).round()}%',
              enabled: band.enabled,
              onChanged: (value) =>
                  provider.dragBandLive(index, shelfSlope: value),
              onChangeEnd: (_) => provider.commitBand(index),
            ),

          if (band.type.usesCutSlope) ...[
            const SizedBox(width: 10),
            CustomComboBox<int>(
              items: const [12, 24],
              value: band.cutSlopeDbPerOct >= 24 ? 24 : 12,
              itemLabel: (slope) => '$slope dB/oct',
              width: 104,
              contentPadding: const EdgeInsets.symmetric(
                horizontal: 10,
                vertical: 7,
              ),
              fontSize: 13,
              menuDirection: ComboBoxMenuDirection.auto,
              screenPadding: _menuSafeArea,
              onChanged: (value) {
                if (value == null) return;
                provider.updateBand(
                  index,
                  (b) => b.copyWith(cutSlopeDbPerOct: value),
                );
              },
            ),
          ],
        ],
      ),
    );
  }

  Widget _buildTypeDropdown(EqProvider provider, EqBandParam band) {
    return CustomComboBox<EqBandType>(
      items: EqBandType.values,
      value: band.type,
      itemLabel: (type) => type.displayName,
      width: 96,
      contentPadding: const EdgeInsets.symmetric(horizontal: 10, vertical: 7),
      fontSize: 13,
      menuDirection: ComboBoxMenuDirection.auto,
      screenPadding: _menuSafeArea,
      onChanged: (type) {
        if (type == null) return;
        provider.updateBand(band.idx, (b) => b.copyWith(type: type));
      },
    );
  }

  // ── 小工具 ────────────────────────────────────────────────────────
  /// 下拉菜单允许出现的区域：底部把悬浮的播放栏让出来
  EdgeInsets get _menuSafeArea => const EdgeInsets.only(
    left: 8,
    right: 8,
    top: 8,
    bottom: EqViewPage.playBarHeight,
  );

  static SliderThemeData get _compactSliderTheme => SliderThemeData(
    trackHeight: 2,
    thumbShape: const RoundSliderThumbShape(enabledThumbRadius: 5),
    overlayShape: const RoundSliderOverlayShape(overlayRadius: 10),
    showValueIndicator: ShowValueIndicator.never,
  );

  static String _formatGain(double db) {
    final clamped = db.abs() < 0.05 ? 0.0 : db;
    return '${clamped >= 0 ? '+' : ''}${clamped.toStringAsFixed(1)} dB';
  }

  static String _formatFreq(double freqHz) {
    if (freqHz >= 1000) {
      return '${(freqHz / 1000).toStringAsFixed(freqHz >= 10000 ? 1 : 2)} kHz';
    }
    return '${freqHz.toStringAsFixed(0)} Hz';
  }
}

// ════════════════════════════════════════════════════════════════
// 8 个段号按钮：选中 / 开关状态一目了然，关掉的段也能从这里点开
// ════════════════════════════════════════════════════════════════
class _BandSelector extends StatelessWidget {
  const _BandSelector({required this.provider, required this.theme});

  final EqProvider provider;
  final AppTheme theme;

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        for (var i = 0; i < provider.bands.length; i++)
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 2),
            child: _button(i),
          ),
      ],
    );
  }

  Widget _button(int index) {
    final band = provider.bands[index];
    final isSelected = provider.selectedBand == index;

    final Color background;
    if (isSelected) {
      background = theme.colorStress;
    } else if (band.enabled) {
      background = theme.colorSuperStress.withValues(alpha: 0.35);
    } else {
      background = theme.colorCell;
    }

    return InkWell(
      onTap: () => provider.selectBand(isSelected ? null : index),
      borderRadius: BorderRadius.circular(4),
      child: Container(
        width: 26,
        height: 26,
        alignment: Alignment.center,
        decoration: BoxDecoration(
          color: background,
          borderRadius: BorderRadius.circular(4),
          border: Border.all(
            color: band.enabled ? theme.colorSuperStress : theme.colorEdge,
            width: 1,
          ),
        ),
        child: Text(
          '${index + 1}',
          style: TextStyle(
            fontSize: 12,
            fontWeight: FontWeight.bold,
            color: theme.colorTextMain,
          ),
        ),
      ),
    );
  }
}

