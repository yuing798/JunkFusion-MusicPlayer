// ==============================================================================
// main.qml — JunkFusion 主界面（对应 UI/src/App.vue）
// ==============================================================================
//
// 这个文件是整个 QML 应用的根节点。它被 mainUI.cpp 通过以下代码加载：
//   engine.loadFromModule("JunkFusion", "main");
//
// App.vue → main.qml 结构对应关系：
//
//   App.vue                    main.qml
//   ─────────                  ────────
//   <div class="app-layout">   ApplicationWindow + RowLayout
//     <LeftColumn />           LeftColumn.qml
//     <InfoWindow />           （Qt 中没有 Teleport，改用 Popup/Overlay）
//     <main>                   ColumnLayout（主内容区域）
//       <AllMusic />           StackLayout 中的 AllMusicPage.qml
//     </main>
//   </div>
//   <playBar />                PlayBar（固定底部）
//
// QML 基础概念（阅读本文件前了解）：
//
// ┌─ 属性绑定（Property Binding） ───────────────────────────────────┐
// │ 这是 QML 最强大的特性。属性值不是一个"赋值"，而是一个"公式"。        │
// │                                                                     │
// │   property int a: 5                                                 │
// │   property int b: a * 2   // b 是 10，而且是响应式的      │
// │                                                                     │
// │ 相当于 Vue 的 computed，但 QML 是引擎级别自动追踪依赖的。          │
// └────────────────────────────────────────────────────────────────────┘
//
// ┌─ 信号处理器（Signal Handler） ───────────────────────────────────┐
// │ 当某个事件发生时自动执行的代码块。命名规则：on + 信号名。           │
// │                                                                     │
// │   Button { onClicked: console.log("被点击了") }                     │
// │   Component.onCompleted: { /* 组件创建完毕后执行 */ }               │
// └────────────────────────────────────────────────────────────────────┘
//
// ┌─ Anchor 布局 vs Layout 布局 ─────────────────────────────────────┐
// │ Qt Quick 提供两种布局方式：                                         │
// │   anchors: 经典方式，子组件相对于父组件或兄弟组件定位                │
// │   Layouts: Qt 6 新增，类似 CSS Flexbox（RowLayout/ColumnLayout）    │
// │                                                                     │
// │   anchors.left: parent.left   ← "贴到父组件左边"                    │
// │   anchors.centerIn: parent    ← "在父组件中居中"                    │
// │                                                                     │
// │ 本文件使用 Layouts（更接近 CSS Flexbox 的思维习惯）                 │
// └────────────────────────────────────────────────────────────────────┘

import QtQuick           // Qt Quick 基础组件（Rectangle, Text, Image, MouseArea 等）
import QtQuick.Layouts   // Qt Quick 布局组件（RowLayout, ColumnLayout, StackLayout 等）
import QtQuick.Controls  // Qt Quick 控件组件（Button, ComboBox, ScrollView, Popup 等）

// ════════════════════════════════════════════════════════════════════
// ApplicationWindow — QML 的顶层窗口
// 相当于浏览器的 <html> 标签，一个 QML 应用只能有一个
// ════════════════════════════════════════════════════════════════════
ApplicationWindow {
    id: root  // id 是 QML 中的唯一标识符，类似 HTML 的 id 属性，
              // 但更强：任何地方都可以通过 root 来访问这个对象的所有属性

    // ── 窗口基本属性 ──
    visible: true                           // 窗口是否可见（必须设为 true）
    width: Screen.width                     // 窗口宽度 = 屏幕宽度（全屏宽）
    height: Screen.height                   // 窗口高度 = 屏幕高度（全屏高）
    title: "JunkFusion"                     // 窗口标题（显示在任务栏）
    color: colorMain                        // 窗口背景色（引用下面定义的主题变量）

    // ── 窗口样式 ──
    // Qt.FramelessWindowHint 去掉标题栏和边框，实现全屏无边框效果
    // 配合 width/height = 屏幕尺寸，实现"伪全屏"
    // 注：真正的全屏用 visibility: Window.FullScreen
    flags: Qt.Window | Qt.FramelessWindowHint

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

    // ── 当前选中的页面 ID（对应 App.vue 的 currentPageId） ──
    // 0=所有音乐, 1=我喜欢, 2=最近播放, 3=作者, 4=专辑, 5=歌单
    // 6=风格, 7=AI助手, 8=效果器, 9=均衡器, 10=音箱阵列, 11=设置
    property int currentPageId: 0

    // ════════════════════════════════════════════════════════════════
    // 主布局：左导航栏 + 右主内容区
    //
    // RowLayout 类似 CSS 的 display: flex; flex-direction: row
    // - 子组件从左到右排列
    // - Layout.fillHeight/Layout.fillWidth 控制子组件的伸缩行为
    // ════════════════════════════════════════════════════════════════
    RowLayout {
        anchors.fill: parent       // 填满整个父窗口（ApplicationWindow）
        spacing: 0                 // 子组件之间的间距（等同于 CSS 的 gap）

        // ── 左侧导航栏 ──
        // LeftColumn 是从 LeftColumn.qml 文件加载的组件
        // QML 的文件名即组件名：LeftColumn.qml → LeftColumn { }
        LeftColumn {
            id: leftColumn

            Layout.preferredWidth: 220
            Layout.fillHeight: true

            // ── 传递主题色 ──
            // QML 中 id 是文件作用域的，子 QML 文件无法直接访问 main.qml 的 root id。
            // 必须在子组件上声明对应的 property（属性），然后在父组件中通过属性绑定传值。
            // 属性绑定的语法：property名: 值或表达式
            // 这里的值（如 root.colorNav）会在 root.colorNav 变化时自动同步到 LeftColumn
            //
            // 对照关系：
            //   QML:  property color colorNav  (在 LeftColumn.qml 中声明)
            //          colorNav: root.colorNav  (在 main.qml 中绑定)
            //          → 等价于 LeftColumn 有一个"插座"，main.qml 把"电源"接上去
            //
            //   Vue:  <LeftColumn :color-nav="colorNav" />
            //          → props 传值，语法不同但思想一致
            colorNav: root.colorNav
            colorEdge: root.colorEdge
            colorHover: root.colorHover
            colorTextMain: root.colorTextMain
            colorStress: root.colorStress
            colorSuperStress: root.colorSuperStress
            bigFontSize: root.bigFontSize
            midFontSize: root.midFontSize
            borderRadius: root.borderRadius
            easeTime: root.easeTime

            // ── 导航点击回调 ──
            onPageSelected: function(pageId) {
                root.currentPageId = pageId;
            }
        }

        // ── 右侧主内容区 ──
        // ColumnLayout 类似 CSS 的 display: flex; flex-direction: column
        ColumnLayout {
            Layout.fillWidth: true      // 宽度自动填充剩余空间（类似 CSS flex: 1）
            Layout.fillHeight: true
            spacing: 0

            // StackLayout — QML 的页面切换容器（类似 Vue 的 <component :is="...">）
            // 同一时间只显示 currentIndex 对应的子组件
            // 非当前页面的子组件完全不可见（不渲染、不占布局空间）
            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: root.currentPageId  // 绑定到 currentPageId，自动响应变化

                // ── 页面 0：所有音乐 ──
                AllMusicPage {
                    id: allMusicPage

                    // ── 传递主题色和字体 ──
                    // 子 QML 文件无法直接访问 main.qml 的 root id，
                    // 必须通过属性绑定将数据"注入"到子组件
                    colorMain: root.colorMain
                    colorHover: root.colorHover
                    colorClicked: root.colorClicked
                    colorTextMain: root.colorTextMain
                    colorTextSecond: root.colorTextSecond
                    colorEdge: root.colorEdge
                    bigFontSize: root.bigFontSize
                    midFontSize: root.midFontSize
                    littleFontSize: root.littleFontSize
                    borderRadius: root.borderRadius
                    easeTime: root.easeTime

                    // ── 信号连接：子组件请求显示信息弹窗 ──
                    // AllMusicPage 发射 showInfoRequest 信号时，
                    // 调用 infoPopup.showInfo() 来显示弹窗
                    onShowInfoRequest: function(message, holdTime) {
                        infoPopup.showInfo(message, holdTime);
                    }
                }

                // ── 页面 1-11：未开发的占位页面 ──
                // Repeater 是 QML 的循环生成器（类似 Vue 的 v-for）
                // model: 11 表示生成 11 个组件，从 1 到 11
                Repeater {
                    model: 11
                    delegate: Rectangle {
                        // index 是 Repeater 自动提供的变量（0-based，这里从 0 开始）
                        // 但我们的页面 id 从 1 到 11，所以显示时用 index + 1
                        required property int index
                        // required 关键字：表示这个属性必须由外部提供
                        // Repeater 的 delegate 会自动接收 index 属性

                        property int pageId: index + 1  // 页面 id，index+1 = 1~11

                        color: root.colorMain
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Text {
                            anchors.centerIn: parent
                            text: "页面 " + parent.pageId + "（尚未开发）"
                            font.pixelSize: root.bigFontSize
                            color: root.colorTextSecond
                        }
                    }
                }
            }
        }
    }

    // ════════════════════════════════════════════════════════════════
    // 底部播放栏 — 占位组件
    //
    // 作为 ApplicationWindow 的直接子组件（不在 RowLayout 内），
    // 它会浮在所有内容的上层（z 值最高的在最上面）
    //
    // 使用 anchors 固定在底部，位于 RowLayout 之上
    // visible 绑定到条件（当前没有歌曲时不显示）
    // ════════════════════════════════════════════════════════════════
    Rectangle {
        id: playBar

        // anchors 是 Qt Quick 的经典定位方式
        // bottom: parent.bottom  — 贴到底部
        // left: parent.left      — 贴到左边
        // right: parent.right    — 贴到右边（配合 left 实现宽度填充）
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        height: 90                     // 固定高度 90px（对照 playBar.vue 的 90px）
        color: root.colorHover         // 半透明背景（hover 色）

        z: 1000                        // z 值（层级），越大越在上层
        // 对照 playBar.vue 的 z-index: 1000

        // 暂时不可见（后续完成 playBar 组件后改为条件显示）
        visible: false

        // Behavior 定义动画：当某个属性改变时，自动执行过渡动画
        // 这里当 y（垂直位置）改变时，用 500ms 的缓动曲线过渡
        // 后续实现播放栏的滑入/滑出动画时会用到
        Behavior on y {
            NumberAnimation {
                duration: 500
                easing.type: Easing.InOutCubic
            }
        }
    }

    // ════════════════════════════════════════════════════════════════
    // 信息弹窗（Info Popup）— 对应 App.vue 中的 InfoWindow
    //
    // Popup 是 Qt Quick Controls 提供的弹窗组件，类似网页的模态框
    // 使用 closePolicy 控制关闭行为
    // ════════════════════════════════════════════════════════════════
    Popup {
        id: infoPopup

        // ── Popup 定位：居中 ──
        x: (root.width - width) / 2
        y: (root.height - height) / 2

        // ── Popup 尺寸 ──
        width: 360
        // height 自动由内容撑开（QML 的默认行为）

        // ── Popup 外观 ──
        // 设置 modal: true 会创建半透明遮罩层
        modal: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        // 背景 Rectangle
        background: Rectangle {
            color: Qt.rgba(root.colorHover.r, root.colorHover.g, root.colorHover.b, 0.7)
            // Qt.rgba(r, g, b, a) 创建带透明度的颜色
            // 对照 InfoWindow 的 CSS: background-color: color-mix(in srgb, var(--color-hover), transparent 30%)
            // 这里用 Qt.rgba 手动降低不透明度
            radius: root.borderRadius
            border.width: 1
            border.color: root.colorEdge
        }

        // ── 弹窗内容 ──
        contentItem: ColumnLayout {
            spacing: 10

            // 消息文本
            Text {
                id: popupMessage
                Layout.fillWidth: true
                Layout.margins: 20
                text: ""                                // 初始为空，由 showInfo() 函数设置
                color: root.colorInfo
                font.pixelSize: root.midFontSize
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere  // 自动换行
                horizontalAlignment: Text.AlignHCenter
            }

            // 关闭按钮
            Button {
                Layout.alignment: Qt.AlignHCenter
                text: "关闭"
                onClicked: infoPopup.close()
            }
        }

        // ── 公开方法：显示信息弹窗 ──
        // QML 中的 function 定义可调用的方法
        // C++ 或 QML 其他组件可以通过 infoPopup.showInfo("消息") 调用
        function showInfo(message, holdTime) {
            if (holdTime === undefined) holdTime = 3000;  // 默认保持 3 秒
            popupMessage.text = message;
            open();  // Popup 内置 open() 方法，显示弹窗

            // Timer 是 QML 内置的定时器
            // 等到 holdTime 毫秒后自动关闭
            autoCloseTimer.interval = holdTime;
            autoCloseTimer.restart();
        }
    }

    // ── 自动关闭定时器 ──
    // 不属于任何可视组件，是纯逻辑对象
    // 当 interval 到期后，触发 onTriggered 回调
    Timer {
        id: autoCloseTimer
        repeat: false           // 不重复，只触发一次
        onTriggered: {
            infoPopup.close();  // 到期后关闭弹窗
        }
    }
}
