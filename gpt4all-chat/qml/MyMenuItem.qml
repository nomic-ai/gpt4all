import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

MenuItem {
    id: item
    background: Rectangle {
        color: item.highlighted ? theme.lightContrast : theme.darkContrast
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
