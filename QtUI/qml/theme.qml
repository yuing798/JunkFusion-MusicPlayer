// Theme.qml
pragma Singleton                          // 👈 这行声明它是全局单例
import QtQuick

QtObject {
    // ════════════════════════════════════════════════════════════════
    // 主题色定义（对照 UI/src/assets/css/theme.css 的 CSS 变量）
    //
    // 在 QML 中，property 声明一个带类型的属性。这些属性可以在整个
    // QML 文件中通过 root.colorMain 或直接 colorMain 来引用。
    //
    // 格式：property <类型> <名字>: <初始值>
    //
    // QML 和 CSS 的对比：
    //   CSS:  var(--color-main)  →  QML:  colorMain
    //   CSS:  需要 :root{} 声明   →  QML:  property 在 ApplicationWindow 上
    // ════════════════════════════════════════════════════════════════
    property color colorMain: "#f0f0f0"          // 主要背景色
    property color colorNav: "#e2e4e4"           // 导航窗背景色
    property color colorHover: "#c5c4c4"         // 鼠标悬浮色
    property color colorCell: "#f2fcff"          // 单元格背景色
    property color colorClicked: "#b0c4de"       // 鼠标点击色
    property color colorEdge: "#b2b1b1"          // 边框颜色
    property color colorTextMain: "#222222"      // 主要文字颜色
    property color colorTextSecond: "#555555"    // 次要文字颜色
    property color colorStress: "#00f2fe"        // 强调色（导航当前页高亮）
    property color colorSuperStress: "#4facfe"   // 超级强调（渐变用）
    property color colorShadow: "#2f76b4"        // 阴影色
    property color colorError: "#991f1f"         // 错误提示颜色
    property color colorInfo: "#991f1f"          // 信息提示颜色（与 error 共用）

    // ── 字体大小（对照 theme.css 的 --big-font, --mid-font, --little-font） ──
    property int bigFontSize: 25
    property int midFontSize: 18
    property int littleFontSize: 15

    // ── 圆角半径和过渡时间 ──
    property int borderRadius: 6
    property int easeTime: 150                     // 过渡时间（毫秒），QML 中毫秒更直观
}
