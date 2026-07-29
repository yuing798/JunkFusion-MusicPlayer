// ==============================================================================
// AllMusicPage.qml — "所有音乐"页面（对应 UI/src/components/pages/AllMusic.vue）
// ==============================================================================
//
// 功能：
//   - 标题行："所有音乐" + "共 N 首"
//   - 工具栏行：导入文件按钮 + 排序下拉框 + 升降序切换
//   - 歌曲列表区域（当前为占位状态）
//
// QML 组合框（ComboBox）说明：
//   ComboBox 是 Qt Quick Controls 的下拉选择组件。
//   model 属性接受数组或 ListModel，定义下拉选项。
//   currentIndex 绑定到当前选中项的索引。
//
// QML 和 Vue 状态管理的对比：
//   Vue:   ref(0) + watch + localStorage
//   QML:   property + onXxxChanged 信号处理器 + Settings（Qt 的持久化存储）

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// ════════════════════════════════════════════════════════════════════
// Item — 不可见容器（类似 <div>）
// ════════════════════════════════════════════════════════════════════
Item {
    id: allMusicRoot

    // ── 本地状态（对照 AllMusic.vue 的 ref 变量） ──

    // 歌曲总数（后续从数据库获取）
    property int songCount: 0

    // 排序方式：0=添加时间, 1=歌曲名称, 2=播放次数
    // 对照 AllMusic.vue 的 selectedSort: ref(0)
    property int selectedSort: 0

    // 是否升序排列（true=升序, false=降序）
    // 对照 AllMusic.vue 的 isAscending: ref(true)
    property bool isAscending: true

    // 是否正在导入中（按钮禁用状态）
    property bool isImporting: false

    // ── 信号：向父组件请求显示信息弹窗 ──
    // QML 的 signal 类似 Vue 的 emit
    // 父组件通过 onShowInfoRequest 处理器来接收到这个信号
    signal showInfoRequest(string message, int holdTime)

    // ── 排序方式的数据源 ──
    // 这是一个 JavaScript 数组，QML 中可以直接用 JS 语法
    // 格式：[{ value: 0, label: "显示文本" }, ...]
    // ComboBox 会把它作为 model 来渲染
    // ── 从父组件（main.qml）传入的主题色和字体 ──
    // QML 中 id 是文件作用域的，子 QML 文件无法直接访问 main.qml 的 root id。
    // 必须在子组件上声明对应的 property，然后在父组件中通过属性绑定传入值。
    property color colorMain
    property color colorHover
    property color colorClicked
    property color colorTextMain
    property color colorTextSecond
    property color colorEdge
    property int bigFontSize
    property int midFontSize
    property int littleFontSize
    property int borderRadius
    property int easeTime

    readonly property var sortOptions: [
        { value: 0, label: "添加时间" },
        { value: 1, label: "歌曲名称" },
        { value: 2, label: "播放次数" }
    ]

    // ════════════════════════════════════════════════════════════════
    // 整体布局：从上到下排列（对照 AllMusic.vue 的 flex-direction: column）
    // ════════════════════════════════════════════════════════════════
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15       // 内边距（对照 CSS padding: 15px 20px）
        spacing: 0

        // ============================================================
        // 第 1 行：标题行（对照 AllMusic.vue 的 header-row）
        //
        // RowLayout 类似 CSS 的 display: flex; flex-direction: row
        // ============================================================
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            spacing: 15

            // "所有音乐" 大标题
            Text {
                text: "所有音乐"
                font.pixelSize: allMusicRoot.bigFontSize
                font.bold: true
                color: allMusicRoot.colorTextMain
            }

            // "共 N 首" 小字
            Text {
                text: "共 " + songCount + " 首"
                font.pixelSize: allMusicRoot.littleFontSize
                color: allMusicRoot.colorTextSecond
            }
        }

        // ============================================================
        // 第 2 行：工具栏（对照 AllMusic.vue 的 toolbar-row）
        //
        // 使用 RowLayout + Layout.fillWidth 实现左右两端对齐
        // ============================================================
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            spacing: 6

            // ── 左侧：导入文件按钮 ──
            Button {
                id: importButton

                // Layout.alignment: Qt.AlignLeft 让按钮靠左
                // 配合右边的 Item { Layout.fillWidth: true } 实现左右分离
                Layout.alignment: Qt.AlignLeft

                // 按钮尺寸
                implicitWidth: 200
                implicitHeight: 44

                // 文字
                text: isImporting ? "导入中..." : "导入文件/扫描文件夹"

                // 禁用态
                enabled: !isImporting

                // ── 按钮背景（自定义样式） ──
                background: Rectangle {
                    radius: allMusicRoot.borderRadius
                    color: parent.pressed ? allMusicRoot.colorHover
                         : parent.hovered ? allMusicRoot.colorHover
                         : allMusicRoot.colorClicked
                    // 等价于 CSS: background-color 的三态

                    // 按下和悬浮时的颜色过渡
                    Behavior on color {
                        ColorAnimation { duration: allMusicRoot.easeTime }
                    }
                }

                // ── 按钮文字 ──
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: allMusicRoot.midFontSize
                    color: allMusicRoot.colorTextMain
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                // ── 点击事件 ──
                onClicked: {
                    // TODO: 后续通过 juceBackend 调用导入功能
                    // 当前为占位：发射信号通知父组件显示提示
                    console.log("导入文件按钮被点击（功能尚未实现）");
                    // 发射 showInfoRequest 信号 → main.qml 处理 → infoPopup.showInfo()
                    allMusicRoot.showInfoRequest("导入功能即将上线", 2000);
                }
            }

            // ── 弹性空间（把左右两组分开） ──
            Item {
                Layout.fillWidth: true  // 独占剩余空间，把右边推到底
            }

            // ── 右侧：排序方式下拉框 + 升降序按钮 ──
            // ComboBox 是 Qt Quick Controls 的下拉选择组件
            ComboBox {
                id: sortCombo

                Layout.preferredWidth: 200
                Layout.preferredHeight: 44

                // ── 数据绑定 ──
                // textRole: "label" 告诉 ComboBox 用 model 中每个对象的 "label" 字段作为显示文字
                // 对照 Vue 的 <option v-for="opt in sortOptions" :value="opt.value">{{ opt.label }}</option>
                textRole: "label"
                valueRole: "value"
                model: allMusicRoot.sortOptions

                // currentIndex 绑定到 selectedSort
                // 初始为 0（"添加时间"）
                currentIndex: allMusicRoot.selectedSort

                // ── 选中项改变时 ──
                // onCurrentIndexChanged 是 currentIndex 变化时自动调用的信号处理器
                onCurrentIndexChanged: {
                    allMusicRoot.selectedSort = currentIndex;
                    // TODO: 后续持久化到 QSettings（类似 localStorage）
                    // 对照 AllMusic.vue: localStorage.setItem('AllMusic_sortMode', String(value))
                }
            }

            // ── 升降序切换按钮 ──
            Button {
                id: ascendingButton
                implicitWidth: 44
                implicitHeight: 44

                background: Rectangle {
                    radius: allMusicRoot.borderRadius
                    color: parent.pressed ? allMusicRoot.colorClicked
                         : parent.hovered ? allMusicRoot.colorHover
                         : "transparent"
                }

                contentItem: Text {
                    // 升序显示 ↑，降序显示 ↓
                    text: allMusicRoot.isAscending ? "↑" : "↓"
                    font.pixelSize: allMusicRoot.bigFontSize
                    color: allMusicRoot.colorTextMain
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    // 切换升降序
                    allMusicRoot.isAscending = !allMusicRoot.isAscending;
                    // TODO: 后续持久化到 QSettings
                    // 对照 AllMusic.vue: localStorage.setItem('AllMusic_isascending', ...)
                }
            }
        }

        // ============================================================
        // 第 3 行：歌曲列表区域（占位）
        //
        // 后续将替换为 ListView + 虚拟滚动模型
        // 对照 Vue 的 useVirtualizer + EachSong 组件
        // ============================================================
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: allMusicRoot.colorMain

            // 占位文字（居中）
            Text {
                anchors.centerIn: parent
                text: "歌曲列表区域（尚未实现）\n\n" +
                      "后续将使用 ListView + C++ 数据模型\n" +
                      "配合虚拟滚动渲染所有歌曲"
                font.pixelSize: allMusicRoot.midFontSize
                color: allMusicRoot.colorTextSecond
                horizontalAlignment: Text.AlignHCenter
                // wrapMode 自动换行
                wrapMode: Text.WordWrap
            }
        }
    }

    // ════════════════════════════════════════════════════════════════
    // 生命周期钩子（类似 Vue 的 onMounted）
    //
    // Component.onCompleted 在组件创建完成后自动调用
    // 对应 AllMusic.vue 中 onMounted() 的内容
    // ════════════════════════════════════════════════════════════════
    Component.onCompleted: {
        // TODO: 后续从持久化存储恢复排序设置
        // 对照 AllMusic.vue:
        //   isAscending.value = localStorage.getItem('AllMusic_isascending') !== 'false'
        //   const sortWaysLoad = localStorage.getItem('AllMusic_sortMode')
        console.log("AllMusicPage 已加载");
    }
}
