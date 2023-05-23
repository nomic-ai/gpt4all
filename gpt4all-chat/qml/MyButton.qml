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
        Accessible.role: Accessible.Button
        Accessible.name: text
    }
    background: Rectangle {
        opacity: .5
        border.color: theme.backgroundLightest
        border.width: 1
        radius: 10
        color: theme.backgroundLight
    }
}