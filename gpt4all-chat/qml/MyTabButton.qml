import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import mysettings
import mysettingsenums

MySettingsButton {
    property bool isSelected: false
    contentItem: Text {
        text: parent.text
        horizontalAlignment: Qt.AlignCenter
        color: isSelected ? theme.titleTextColor : theme.styledTextColor
        font.pixelSize: theme.fontSizeLarger
    }
    background: Item {
        visible: isSelected || hovered
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 3
            color: isSelected ? theme.titleTextColor : theme.styledTextColorLighter
        }
    }
}
