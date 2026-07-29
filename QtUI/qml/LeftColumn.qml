// ==============================================================================
// LeftColumn.qml — 左侧导航栏（对应 UI/src/components/LeftColumn.vue）
// ==============================================================================
//
// 功能：
//   - 显示应用 Logo
//   - 三个导航分组（曲库浏览、分类浏览、功能板块）
//   - 点击按钮时通知父组件切换页面（单选行为）
//
// Vue → QML 对照：
//   v-for="section in sections"  →  ListView + Repeater
//   :class="{ active: ... }"      →  属性绑定 + 三元表达式
//   @click="handleSelect(id)"    →  onClicked: { ... }
//   emit('selection-changed', id) →  signal pageSelected(int pageId)
//
// QML 关键概念：
//   signal — 自定义信号，用于向父组件通知事件
//   ListModel — 声明式列表数据，ListElement 定义每一条
//   delegate — 模板，定义列表中每一项如何渲染
//   required property — 组件必须由外部提供的属性（类似 Vue 的 props）

import QtQuick
import QtQuick.Controls

// ════════════════════════════════════════════════════════════════════
// Item — QML 中最基础的不可见容器
// 相当于 HTML 的 <div>（没有默认外观，纯布局用）
// 作用：把子组件组合在一起，提供统一的坐标系统
// ════════════════════════════════════════════════════════════════════
Item {
    id: leftColumnRoot

    // ── 公开信号：当用户点击导航按钮时发射（通知父组件切换页面） ──
    // 信号在 QML 中类似 Vue 的 emit
    // 父组件通过 on<SignalName> 语法监听：
    //   LeftColumn { onPageSelected: function(id) { ... } }
    signal pageSelected(int pageId)

    // ── 当前选中的页面 ID（默认选中"所有音乐"） ──
    // 这是一个公开/内部都可读写的属性
    property int currentIndex: 0

    // ── 从父组件（main.qml）传入的主题色 ──
    // 这些属性没有默认值，必须在创建 LeftColumn 时由父组件赋值
    // 如果忘记赋值，QML 运行时会打印警告（而不是崩溃）
    //
    // property 在 QML 中的含义：
    //   声明一个"属性"，它既可以被外部读取/写入，
    //   也可以绑定到其他属性（自动追踪依赖变化）
    property color colorNav
    property color colorEdge
    property color colorHover
    property color colorTextMain
    property color colorStress
    property color colorSuperStress
    property int bigFontSize
    property int midFontSize
    property int borderRadius
    property int easeTime

    // ════════════════════════════════════════════════════════════════
    // Rectangle — QML 中的矩形
    // 它是最常用的可见组件，类似 HTML 给 div 设了背景色
    //
    // anchors.fill: parent 意思是填满父组件（Item）的全部空间
    // ════════════════════════════════════════════════════════════════
    Rectangle {
        anchors.fill: parent
        color: leftColumnRoot.colorNav  // 导航窗背景色（通过属性绑定传入）

        // ── ScrollView 包裹滚动内容 ──
        // ScrollView 提供滚动条（类似 CSS overflow: auto）
        ScrollView {
            anchors.fill: parent
            // ScrollBar 的样式设置
            // 只在需要时显示垂直滚动条
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            // ── 滚动内容容器 ──
            Column {
                width: leftColumnRoot.width  // Column 宽度 = 导航栏宽度
                // height 自动由内容撑开
                // bottomPadding 在底部留空白（对照 Vue 的 padding-bottom: 100px）
                bottomPadding: 100

                // ============================================================
                // Logo 区域
                // ============================================================
                Rectangle {
                    width: parent.width     // 宽度与父组件（Column）相同
                    height: 60
                    color: "transparent"    // 透明背景

                    // Image 组件 — 自动加载图片
                    // source 如果是相对路径，QML 会在当前 qml 文件所在目录查找
                    // 也可以用 qrc:/ 前缀加载 Qt 资源文件中的图片
                    Image {
                        anchors.centerIn: parent       // 在父组件中居中
                        source: "../../image/JunkFusion.png"  // 图片路径（相对于 QtUI/ 目录）
                        fillMode: Image.PreserveAspectFit  // 等比缩放、不裁剪
                        // 类似 CSS object-fit: contain
                        sourceSize.height: 50          // 限制图片最大高度为 50px
                        // sourceSize 设定后，QML 会以此为上限等比缩放
                    }
                }

                // ============================================================
                // 导航数据模型 — ListModel
                //
                // 对照 LeftColumn.vue 中的 sections: NavSection[] 定义
                //
                // ListModel 是 QML 的列表数据模型（纯声明式）
                // ListElement 定义列表中每一项的键值对
                //
                // 每个 ListElement 的字段：
                //   - label：分组标签名（显示在分组头上的文字）
                //   - buttons：嵌套的 JavaScript 数组，每个元素是 { id, text }
                //
                // 注意：嵌套数组在 QML ListModel 中比较特殊，
                // 需要用 Qt.format() 或 JS 数组字面量的变通方式。
                // 这里我们使用更简单的方式——直接用一个包含所有按钮的列表，
                // 并添加 section 字段来分组。
                // ============================================================
                ListModel {
                    id: navModel

                    // 分组 1：曲库浏览（对照 LeftColumn.vue: label: '曲库浏览'）
                    ListElement { sectionLabel: "曲库浏览"; buttonId: 0; buttonText: "所有音乐" }
                    ListElement { sectionLabel: "曲库浏览"; buttonId: 1; buttonText: "我喜欢" }
                    ListElement { sectionLabel: "曲库浏览"; buttonId: 2; buttonText: "最近播放" }

                    // 分组 2：分类浏览（对照 LeftColumn.vue: label: '分类浏览'）
                    ListElement { sectionLabel: "分类浏览"; buttonId: 3; buttonText: "作者" }
                    ListElement { sectionLabel: "分类浏览"; buttonId: 4; buttonText: "专辑" }
                    ListElement { sectionLabel: "分类浏览"; buttonId: 5; buttonText: "歌单" }
                    ListElement { sectionLabel: "分类浏览"; buttonId: 6; buttonText: "风格" }

                    // 分组 3：功能板块（对照 LeftColumn.vue: label: '功能板块'）
                    ListElement { sectionLabel: "功能板块"; buttonId: 7; buttonText: "AI助手" }
                    ListElement { sectionLabel: "功能板块"; buttonId: 8; buttonText: "效果器" }
                    ListElement { sectionLabel: "功能板块"; buttonId: 9; buttonText: "均衡器" }
                    ListElement { sectionLabel: "功能板块"; buttonId: 10; buttonText: "音箱阵列" }
                    ListElement { sectionLabel: "功能板块"; buttonId: 11; buttonText: "设置" }
                }

                // ============================================================
                // ListView — QML 的高性能滚动列表
                //
                // ListView 是 Qt 的核心组件，类似网页的虚拟滚动列表。
                // 它只渲染可见区域内的项（+ overscan），极大节省内存。
                //
                // 参数说明：
                //   model    → 数据源（上面定义的 ListModel）
                //   delegate → 每个列表项的模板（如何渲染一个条目）
                //   section  → 分组相关配置（分组标签的渲染方式）
                //
                // 对照关系：
                //   Vue:  v-for="section in sections"          →  section.delegate
                //         v-for="button in section.buttons"     →  Repeater in delegate
                // ============================================================
                ListView {
                    id: navListView
                    width: parent.width
                    // height 设为内容高度，让 Column 来管理滚动
                    // 因为 ListView 放在 ScrollView 的 Column 里，
                    // 需要显式指定高度，否则 ListView 自己也会尝试滚动
                    // 这里用 childrenRect.height（所有子项的总高度）
                    height: contentHeight
                    // interactive: false 禁用 ListView 自己的滚动，
                    // 让外层的 ScrollView 统一管理滚动
                    interactive: false

                    model: navModel

                    // ── delegate：每个列表项的渲染模板 ──
                    // delegate 是 ListView 的核心概念：
                    // 它定义了一个"模具"，ListView 会用这个模具为
                    // model 中的每一条数据"浇筑"出一个组件实例
                    //
                    // 在 delegate 内部，可以直接访问 model 中的字段：
                    //   model.sectionLabel  → 当前条目所属分组名
                    //   model.buttonId      → 按钮的唯一 id
                    //   model.buttonText    → 按钮显示的文字
                    delegate: Item {
                        width: navListView.width
                        height: childrenRect.height  // 高度由子组件决定

                        // ── 检测分组变化 ──
                        // 当 sectionLabel 与上一条不同时，说明进入新分组
                        // 需要先渲染分组标签 (sectionLabel)，再渲染按钮
                        //
                        // index 是 ListView 自动提供的，表示当前项在列表中的序号
                        // 通过 model.index 访问（不是 index，避免与 delegate 的 index 冲突）
                        property bool isFirstInSection: {
                            if (model.index === 0) return true;
                            // 获取上一条记录的 sectionLabel
                            return navModel.get(model.index - 1).sectionLabel !== model.sectionLabel;
                        }

                        Column {
                            width: parent.width

                            // ── 分组标签（sectionLabel） ──
                            // 仅在该分组的第一个条目上显示
                            Rectangle {
                                width: parent.width
                                height: 50
                                color: "transparent"
                                visible: isFirstInSection  // 只在分组第一条显示

                                // 上下分隔线（对照 Vue 的 border-top + border-bottom）
                                Rectangle {
                                    anchors.top: parent.top
                                    width: parent.width
                                    height: 5
                                    color: leftColumnRoot.colorEdge
                                }
                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    width: parent.width
                                    height: 5
                                    color: leftColumnRoot.colorEdge
                                }

                                // 分组标签文字
                                Text {
                                    anchors.centerIn: parent
                                    text: model.sectionLabel
                                    font.pixelSize: leftColumnRoot.bigFontSize
                                    font.bold: true
                                    color: leftColumnRoot.colorTextMain
                                }
                            }

                            // ── 导航按钮 ──
                            // ItemDelegate 是 Qt Quick Controls 提供的可交互项
                            // 类似 HTML 的 <button>（但有更丰富的样式控制）
                            ItemDelegate {
                                width: parent.width - 10   // 留出左右各 5px 间距
                                height: 40
                                anchors.horizontalCenter: parent.horizontalCenter

                                // ── checked（选中状态）绑定 ──
                                // 当 leftColumnRoot.currentIndex === model.buttonId 时，
                                // 该按钮处于 checked 状态，背景高亮
                                // 这是属性绑定魔法：currentIndex 变了 → checked 自动更新
                                // CheckDelegate 不能自动单选（不像 RadioButton），
                                // 需要手动管理 checked 状态
                                highlighted: leftColumnRoot.currentIndex === model.buttonId

                                // ── 背景（用 contentItem 的 background 属性不行，直接包一个 Rectangle） ──
                                // Qt Quick Controls 2 的 ItemDelegate 通过各状态属性控制背景：
                                //   highlighted → 高亮态（这里用作"已选中"态）
                                //   hovered     → 鼠标悬浮态（自动设置）
                                //   pressed     → 鼠标按下去（自动设置）
                                background: Rectangle {
                                    color: {
                                        // 三元表达式根据 highlighted 状态切换颜色
                                        // 选中时透明（让渐变背景显示），未选中时透明
                                        if (parent.parent.highlighted) {
                                            return "transparent";
                                        }
                                        // hovered 是 ItemDelegate 的内置属性（鼠标悬浮时自动为 true）
                                        if (parent.parent.hovered) {
                                            return leftColumnRoot.colorHover;
                                        }
                                        return "transparent";
                                    }

                                    radius: leftColumnRoot.borderRadius

                                    // ── 选中态的外发光效果（对照 Vue 的 .nav-button.active::after） ──
                                    // QML 不支持 CSS 的 ::after 伪元素，但可以用多层 Rectangle 来模拟
                                    Rectangle {
                                        anchors.fill: parent
                                        anchors.margins: -8   // 向外扩展 8px 形成外发光
                                        z: -2                   // 放在按钮背景下面
                                        radius: leftColumnRoot.borderRadius + 8
                                        color: leftColumnRoot.colorStress
                                        opacity: parent.parent.highlighted ? 0.6 : 0  // 仅选中时可见
                                        visible: parent.parent.highlighted
                                    }

                                    // ── 悬浮态位移效果（对照 Vue 的 .nav-button:hover transform: translateY(-3px)） ──
                                    // 当鼠标悬浮时，按钮上移 3px
                                    // Behavior 提供过渡动画
                                    // 注意：需要在按钮级别处理 transform，这里只是准备
                                }

                                // ── 按钮文字 ──
                                contentItem: Text {
                                    text: model.buttonText
                                    font.pixelSize: leftColumnRoot.midFontSize
                                    // 选中时加粗（对照 Vue 的 .nav-button.active font-weight: bold）
                                    font.bold: parent.highlighted
                                    color: leftColumnRoot.colorTextMain
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                // ── 点击事件 ──
                                // onClicked 在用户点击并释放后触发
                                onClicked: {
                                    // 收音机行为（Radio Button）：点击已选中的不做任何事
                                    if (leftColumnRoot.currentIndex !== model.buttonId) {
                                        leftColumnRoot.currentIndex = model.buttonId;

                                        // 发射 pageSelected 信号 → 通知父组件（main.qml）
                                        // 父组件通过 onPageSelected 接收并更新 StackLayout
                                        leftColumnRoot.pageSelected(model.buttonId);
                                    }
                                }

                                // ── 悬浮态位移动画 ──
                                // Behavior 定义：当 y 属性变化时，用动画过渡
                                Behavior on y {
                                    NumberAnimation {
                                        duration: leftColumnRoot.easeTime
                                        easing.type: Easing.OutCubic
                                    }
                                }

                                // ── 悬浮时上移 3px（对照 Vue 的 translateY(-3px)） ──
                                // hoverEnabled 在 ItemDelegate 中默认为 true
                                // hovered 变化 → 三元表达式重新计算 → y 变化 → Behavior 动画
                                y: hovered ? -3 : 0
                            }
                        }
                    }
                }
            }
        }
    }
}
