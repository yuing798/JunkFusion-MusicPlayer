import QtQuick

Rectangle {
    id: titleBar
    height: 40
    color: Theme.colorMain
    // ===== 三个窗口控制按钮（靠右放置） =====
    Row {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        // ---------- 1. 最小化按钮 ----------
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 55
            color: miniMa.containsMouse ? "#3A3A3A" : "transparent"
            //containsMouse用来指示当前鼠标是否悬停在某个MouseArea上面
            Text {
                anchors.centerIn: parent
                text: "-"
                color: Theme.colorTextMain
            }
            MouseArea {
                id: miniMa
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    Window.window.showMinimized();
                }
            }
            Behavior on color {
                ColorAnimation {
                    duration: 150 // 毫秒，相当于 CSS transition-duration: 0.15s
                    easing.type: Easing.InOutQuad // 缓动曲线，相当于 CSS ease-in-out
                }
            }
        }

        // ---------- 2. 最大化/还原按钮 ----------
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 55
            color: normalManMa.containsMouse ? Theme.colorHover : "transparent"
            //containsMouse用来指示当前鼠标是否悬停在某个MouseArea上面
            Text {
                anchors.centerIn: parent
                text: Window.window.visibility === Window.Maximized ? "❐" : "☐"
                color: Theme.colorTextMain
            }
            MouseArea {
                id: normalManMa
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (Window.window.visibility === Window.Maximized) {
                        Window.window.showNormal(); // 最大 -> 还原
                    } else {
                        Window.window.showMaximized(); // 非最大 -> 最大化
                    }
                }
            }
            Behavior on color {
                ColorAnimation {
                    duration: 150 // 毫秒，相当于 CSS transition-duration: 0.15s
                    easing.type: Easing.InOutQuad // 缓动曲线，相当于 CSS ease-in-out
                }
            }
        }

        // ---------- 3. 关闭按钮 ----------
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 55
            color: xMa.containsMouse ? '#fe0000' : "transparent"

            //containsMouse用来指示当前鼠标是否悬停在某个MouseArea上面

            Text {
                anchors.centerIn: parent
                text: "X"
                color: Theme.colorTextMain
            }
            MouseArea {
                id: xMa
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    Window.window.close();
                }
            }
            Behavior on color {
                ColorAnimation {
                    duration: 150 // 毫秒，相当于 CSS transition-duration: 0.15s
                    easing.type: Easing.InOutQuad // 缓动曲线，相当于 CSS ease-in-out
                }
            }
        }
    }

    // ===== 窗口拖拽移动功能（点击标题栏） =====
    MouseArea {
        id: dragArea
        anchors.fill: parent
        onPressed: Window.window.startSystemMove()
    }
}
