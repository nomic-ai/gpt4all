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
    property color  textColor: theme.lightButtonText
    property color  mutedTextColor: theme.lightButtonMutedText
    property color  backgroundColor: theme.lightButtonBackground
    property color  backgroundColorHovered: enabled ? theme.lightButtonBackgroundHovered : backgroundColor
    property real   borderWidth: 0
    property color  borderColor: "transparent"
    property real   fontPixelSize: theme.fontSizeLarge
    property string toolTip
    property alias backgroundRadius: background.radius

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
        id: background
        radius: 10
        border.width: borderWidth
        border.color: borderColor
        color: myButton.hovered ? backgroundColorHovered : backgroundColor
    }
    Accessible.role: Accessible.Button
    Accessible.name: text
    ToolTip.text: toolTip
    ToolTip.visible: toolTip !== "" && myButton.hovered
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
}
