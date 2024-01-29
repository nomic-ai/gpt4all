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
    property color textColor: theme.oppositeTextColor
    property color mutedTextColor: theme.oppositeMutedTextColor
    property color backgroundColor: theme.buttonBackground
    property color backgroundColorHovered: theme.buttonBackgroundHovered
    property real  borderWidth: MySettings.chatTheme === "LegacyDark" ? 1 : 0
    property color borderColor: theme.buttonBorder
    property real fontPixelSize: theme.fontSizeLarge
    contentItem: Text {
        text: myButton.text
        horizontalAlignment: Text.AlignHCenter
        color: myButton.enabled ? textColor : mutedTextColor
        font.pixelSize: fontPixelSize
        Accessible.role: Accessible.Button
        Accessible.name: text
    }
    background: Rectangle {
        radius: 10
        border.width: myButton.borderWidth
        border.color: myButton.borderColor
        color: myButton.hovered ? backgroundColorHovered : backgroundColor
    }
    Accessible.role: Accessible.Button
    Accessible.name: text
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
}
