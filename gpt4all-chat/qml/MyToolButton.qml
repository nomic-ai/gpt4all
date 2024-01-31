import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import Qt5Compat.GraphicalEffects

Button {
    id: myButton
    padding: 10
    property color backgroundColor: theme.iconBackgroundDark
    property color backgroundColorHovered: theme.iconBackgroundHovered
    property bool toggled: false
    property alias source: image.source
    property alias fillMode: image.fillMode
    contentItem: Text {
        text: myButton.text
        horizontalAlignment: Text.AlignHCenter
        color: myButton.enabled ? theme.textColor : theme.mutedTextColor
        font.pixelSize: theme.fontSizeLarge
        Accessible.role: Accessible.Button
        Accessible.name: text
    }

    background: Item {
        anchors.fill: parent
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            visible: myButton.toggled
            border.color: theme.accentColor
            border.width: 1
            radius: 10
        }
        Image {
            id: image
            anchors.centerIn: parent
            mipmap: true
            width: 30
            height: 30
        }
        ColorOverlay {
            anchors.fill: image
            source: image
            color: myButton.hovered ? backgroundColorHovered : backgroundColor
        }
    }
    Accessible.role: Accessible.Button
    Accessible.name: text
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
}
