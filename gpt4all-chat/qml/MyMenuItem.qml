import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

MenuItem {
    id: item
    background: Rectangle {
        radius: 10
        width: parent.width -20
        color: item.highlighted ? theme.menuHighlightColor : theme.menuBackgroundColor
    }

    contentItem: Text {
        leftPadding: 10
        rightPadding: 10
        padding: 5
        text: item.text
        color: theme.textColor
        font.pixelSize: theme.fontSizeLarge
    }
}
