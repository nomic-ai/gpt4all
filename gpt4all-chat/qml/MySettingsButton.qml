import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import mysettings

Button {
    id: myButton
    padding: 10
    rightPadding: 18
    leftPadding: 18
    property color textColor: MySettings.chatTheme === "Dark" ? theme.green800 : theme.green600
    property color mutedTextColor: textColor
    property color backgroundColor: MySettings.chatTheme === "Dark" ? theme.green400 : theme.green200
    property color backgroundColorHovered: theme.green300
    property real  borderWidth: 0
    property color borderColor: "transparent"
    property real fontPixelSize: theme.fontSizeLarge

    contentItem: Text {
        text: myButton.text
        horizontalAlignment: Text.AlignHCenter
        color: myButton.enabled ? textColor : mutedTextColor
        font.pixelSize: fontPixelSize
        font.bold: true
        Accessible.role: Accessible.Button
        Accessible.name: text
    }
    background: Rectangle {
        radius: 10
        border.width: borderWidth
        border.color: borderColor
        color: myButton.hovered ? backgroundColorHovered : backgroundColor
    }
    Accessible.role: Accessible.Button
    Accessible.name: text
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
}
