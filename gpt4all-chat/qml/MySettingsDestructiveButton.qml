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
    font.pixelSize: theme.fontSizeLarge
    property color textColor: MySettings.chatTheme === "Dark" ? theme.red800 : theme.red600
    property color mutedTextColor: MySettings.chatTheme === "Dark" ? theme.red400 : theme.red300
    property color backgroundColor: enabled ? (MySettings.chatTheme === "Dark" ? theme.red400 : theme.red200) :
        (MySettings.chatTheme === "Dark" ? theme.red200 : theme.red100)
    property color backgroundColorHovered: enabled ? (MySettings.chatTheme === "Dark" ? theme.red500 : theme.red300) : backgroundColor
    property real  borderWidth: 0
    property color borderColor: "transparent"
    contentItem: Text {
        text: myButton.text
        horizontalAlignment: Text.AlignHCenter
        color: myButton.enabled ? textColor : mutedTextColor
        font.pixelSize: theme.fontSizeLarge
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
