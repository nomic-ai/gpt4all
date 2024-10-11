import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import mysettings
import mysettingsenums

Button {
    id: myButton
    padding: 10
    rightPadding: 18
    leftPadding: 18
    property color textColor: theme.oppositeTextColor
    property color mutedTextColor: theme.oppositeMutedTextColor
    property color backgroundColor: theme.buttonBackground
    property color backgroundColorHovered: theme.buttonBackgroundHovered
    property real  backgroundRadius: 10
    property real  borderWidth: MySettings.chatTheme === MySettingsEnums.ChatTheme.LegacyDark ? 1 : 0
    property color borderColor: theme.buttonBorder
    property real  fontPixelSize: theme.fontSizeLarge
    property bool  fontPixelBold: false
    property alias textAlignment: textContent.horizontalAlignment

    contentItem: Text {
        id: textContent
        text: myButton.text
        horizontalAlignment: myButton.textAlignment
        color: myButton.enabled ? textColor : mutedTextColor
        font.pixelSize: fontPixelSize
        font.bold: fontPixelBold
        Accessible.role: Accessible.Button
        Accessible.name: text
    }
    background: Rectangle {
        radius: myButton.backgroundRadius
        border.width: myButton.borderWidth
        border.color: myButton.borderColor
        color: !myButton.enabled ? theme.mutedTextColor : myButton.hovered ? backgroundColorHovered : backgroundColor
    }
    Accessible.role: Accessible.Button
    Accessible.name: text
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
}
