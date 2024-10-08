import Qt5Compat.GraphicalEffects
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

MenuItem {
    id: item
    background: Rectangle {
        radius: 10
        width: parent.width -20
        color: item.highlighted ? theme.menuHighlightColor : theme.menuBackgroundColor
    }

    contentItem: RowLayout {
        spacing: 0
        Item {
            visible: item.icon.source.toString() !== ""
            Layout.leftMargin: 6
            Layout.preferredWidth: item.icon.width
            Layout.preferredHeight: item.icon.height
            Image {
                id: image
                anchors.centerIn: parent
                visible: false
                fillMode: Image.PreserveAspectFit
                mipmap: true
                sourceSize.width: item.icon.width
                sourceSize.height: item.icon.height
                source: item.icon.source
            }
            ColorOverlay {
                anchors.fill: image
                source: image
                color: theme.textColor
            }
        }
        Text {
            Layout.alignment: Qt.AlignLeft
            padding: 5
            text: item.text
            color: theme.textColor
            font.pixelSize: theme.fontSizeLarge
        }
        Rectangle {
            color: "transparent"
            Layout.fillWidth: true
            height: 1
        }
    }
}
