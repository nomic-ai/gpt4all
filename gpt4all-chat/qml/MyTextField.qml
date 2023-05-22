import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

TextField {
    id: myTextField
    padding: 10
    background: Rectangle {
        implicitWidth: 150
        color: theme.backgroundLighter
        radius: 10
    }
}