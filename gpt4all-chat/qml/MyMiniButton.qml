import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import Qt5Compat.GraphicalEffects

Button {
    id: myButton
    padding: 0
    property color backgroundColor: theme.iconBackgroundDark
    property color backgroundColorHovered: theme.iconBackgroundHovered
    property alias source: image.source
    property alias fillMode: image.fillMode
    implicitWidth: 30
    implicitHeight: 30
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
        }
        Image {
            id: image
            anchors.centerIn: parent
            mipmap: true
            width: 20
            height: 20
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
