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
    property color textColor: theme.darkButtonText
    property color mutedTextColor: theme.darkButtonMutedText
    property color backgroundColor: theme.darkButtonBackground
    property color backgroundColorHovered: enabled ? theme.darkButtonBackgroundHovered : backgroundColor
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
