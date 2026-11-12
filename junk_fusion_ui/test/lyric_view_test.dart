/// 测试歌词视图与时间轴的接线，而非普通 UI 布局。
/// 通过标准：首次显示和 seek 都选中整组；同组文字靠近且居中；点击翻译
/// 返回原始行号；替换歌词及放大字体不越界；两种自定义颜色随时间正确扫色。
/// 原因：仅测试时间计算无法发现旧的单行选中、初始不滚动和绘制颜色未生效问题。
/// 运行：flutter test test/lyric_view_test.dart。
library;

import 'dart:ui' as ui;

import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:junk_fusion_ui/model/lyric_info.dart';
import 'package:junk_fusion_ui/theme/app_theme.dart';
import 'package:junk_fusion_ui/widgets/PlayControl/lyric_view.dart';
import 'package:provider/provider.dart';

void main() {
  const lyrics = LyricDocument(
    lines: [
      LyricLine(startMs: 1000, text: 'First'),
      LyricLine(startMs: 10000, text: 'Young black cool independent'),
      LyricLine(startMs: 10000, text: '年轻、酷炫、独立'),
      LyricLine(startMs: 20000, text: 'Next', translation: '下一句'),
      LyricLine(startMs: 30000, text: 'Last'),
    ],
  );

  Widget host(Widget child) => ChangeNotifierProvider(
    create: (_) => AppTheme(),
    child: MaterialApp(
      home: Scaffold(
        body: Center(child: SizedBox(width: 600, height: 360, child: child)),
      ),
    ),
  );

  Finder label(String text) => find.byWidgetPredicate(
    (widget) => widget is Semantics && widget.properties.label == text,
  );
  final selected = find.byWidgetPredicate(
    (widget) => widget is Semantics && widget.properties.selected == true,
  );

  testWidgets('首次打开按整组居中，同时间戳两行同时激活且支持点击翻译跳转', (tester) async {
    int? soughtLine;
    await tester.pumpWidget(
      host(
        LyricView(
          lyrics: lyrics,
          positionSeconds: 12,
          durationSeconds: 40,
          onSeekToLine: (line) => soughtLine = line,
        ),
      ),
    );
    await tester.pumpAndSettle();
    expect(selected, findsNWidgets(2));
    final original = tester.getRect(label('Young black cool independent'));
    final translation = tester.getRect(label('年轻、酷炫、独立'));
    expect(translation.top - original.bottom, closeTo(4, .01));
    expect(
      (original.center.dy + translation.center.dy) / 2,
      closeTo(tester.getCenter(find.byType(LyricView)).dy, .1),
    );
    await tester.tap(label('年轻、酷炫、独立'));
    expect(soughtLine, 2);

    await tester.pumpWidget(
      host(
        const LyricView(
          lyrics: lyrics,
          positionSeconds: 20,
          durationSeconds: 40,
        ),
      ),
    );
    await tester.pumpAndSettle();
    expect(selected, findsOneWidget);
    expect(label('下一句'), findsOneWidget);
    await tester.pumpWidget(
      host(
        const LyricView(
          lyrics: lyrics,
          positionSeconds: 0,
          durationSeconds: 40,
        ),
      ),
    );
    await tester.pumpAndSettle();
    expect(selected, findsNothing);
    expect(tester.takeException(), isNull);
  });

  testWidgets('替换歌词和放大字体仍可正确布局和定位', (tester) async {
    await tester.pumpWidget(
      host(
        const LyricView(
          lyrics: lyrics,
          positionSeconds: 31,
          durationSeconds: 40,
        ),
      ),
    );
    await tester.pumpAndSettle();
    await tester.pumpWidget(
      host(
        MediaQuery(
          data: const MediaQueryData(textScaler: TextScaler.linear(2)),
          child: const LyricView(
            lyrics: LyricDocument(
              lines: [
                LyricLine(startMs: 0, text: 'A', translation: '甲'),
                LyricLine(startMs: 0, text: 'B'),
                LyricLine(startMs: 2000, text: 'C'),
              ],
            ),
            positionSeconds: 1,
            durationSeconds: 4,
          ),
        ),
      ),
    );
    await tester.pumpAndSettle();
    expect(selected, findsNWidgets(2));
    expect(tester.takeException(), isNull);
  });

  testWidgets('普通色和高亮色接口确实绘制，进度推进时已唱文字增加', (tester) async {
    final boundaryKey = GlobalKey();
    const document = LyricDocument(
      lines: [LyricLine(startMs: 0, text: 'ABCD')],
    );
    Future<(int, int)> pixelsAt(double seconds) async {
      await tester.pumpWidget(
        host(
          RepaintBoundary(
            key: boundaryKey,
            child: LyricView(
              lyrics: document,
              positionSeconds: seconds,
              durationSeconds: 4,
              normalColor: const Color(0xFFFF0000),
              highlightColor: const Color(0xFF0000FF),
            ),
          ),
        ),
      );
      await tester.pumpAndSettle();
      return (await tester.runAsync(() async {
        final boundary =
            boundaryKey.currentContext!.findRenderObject()!
                as RenderRepaintBoundary;
        final image = await boundary.toImage();
        final bytes = (await image.toByteData(
          format: ui.ImageByteFormat.rawRgba,
        ))!;
        var normal = 0;
        var highlight = 0;
        for (var i = 0; i < bytes.lengthInBytes; i += 4) {
          final r = bytes.getUint8(i);
          final b = bytes.getUint8(i + 2);
          if (r > 200 && b < 30) normal++;
          if (b > 200 && r < 30) highlight++;
        }
        image.dispose();
        return (normal, highlight);
      }))!;
    }

    final early = await pixelsAt(1);
    final late = await pixelsAt(3);
    expect(early.$1, greaterThan(0));
    expect(early.$2, greaterThan(0));
    expect(late.$1, lessThan(early.$1));
    expect(late.$2, greaterThan(early.$2));
    expect(tester.takeException(), isNull);
  });
}
