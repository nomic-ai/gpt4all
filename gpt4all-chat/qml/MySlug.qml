import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

Label {
    id: mySlug
    padding: 3
    rightPadding: 9
    leftPadding: 9
    font.pixelSize: theme.fontSizeSmall
    background: Rectangle {
        radius: 6
        border.width: 1
        border.color: mySlug.color
        color: theme.slugBackground
    }
    ToolTip.visible: ma.containsMouse && ToolTip.text !== ""
    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
    }
}
