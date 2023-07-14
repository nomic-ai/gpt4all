import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

TextField {
    id: myTextField
    padding: 10
    background: Rectangle {
        implicitWidth: 150
        color: theme.backgroundDark
        radius: 10
    }
    color: theme.textColor
}