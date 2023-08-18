import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

Button {
    id: myButton
    padding: 10
    contentItem: Text {
        text: myButton.text
        horizontalAlignment: Text.AlignHCenter
        color: myButton.enabled ? theme.textColor : theme.mutedTextColor
        font.pixelSize: theme.fontSizeLarge
        Accessible.role: Accessible.Button
        Accessible.name: text
    }
    background: Rectangle {
        border.color: myButton.down ? theme.backgroundLightest : theme.buttonBorder
        border.width: 2
        radius: 10
        color: myButton.hovered ? theme.backgroundDark : theme.backgroundDarkest
    }
    Accessible.role: Accessible.Button
    Accessible.name: text
}
