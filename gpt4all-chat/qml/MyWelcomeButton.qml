import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import Qt5Compat.GraphicalEffects
import QtQuick.Layouts
import mysettings

Button {
    id: myButton
    property alias imageSource: myimage.source
    property alias description: description.text

    contentItem: Item {
        id: item
        anchors.centerIn: parent

        RowLayout {
            anchors.fill: parent
            Rectangle {
                id: rec
                color: "transparent"
                Layout.preferredWidth: item.width * 1/5.5
                Layout.preferredHeight: item.width * 1/5.5
                Layout.alignment: Qt.AlignCenter
                Image {
                    id: myimage
                    anchors.centerIn: parent
                    sourceSize.width: rec.width
                    sourceSize.height: rec.height
                    mipmap: true
                    visible: false
                }

                ColorOverlay {
                    anchors.fill: myimage
                    source: myimage
                    color: theme.welcomeButtonBorder
                }
            }

            ColumnLayout {
                Layout.preferredWidth: childrenRect.width
                Text {
                    text: myButton.text
                    horizontalAlignment: Text.AlignHCenter
                    color: myButton.hovered ? theme.welcomeButtonTextHovered : theme.welcomeButtonText
                    font.pixelSize: theme.fontSizeBannerSmall
                    font.bold: true
                    Accessible.role: Accessible.Button
                    Accessible.name: text
                }

                Text {
                    id: description
                    horizontalAlignment: Text.AlignHCenter
                    color: myButton.hovered ? theme.welcomeButtonTextHovered : theme.welcomeButtonText
                    font.pixelSize: theme.fontSizeSmall
                    font.bold: false
                    Accessible.role: Accessible.Button
                    Accessible.name: text
                }
            }
        }
    }

    background: Rectangle {
        radius: 10
        border.width: 1
        border.color: myButton.hovered ? theme.welcomeButtonBorderHovered : theme.welcomeButtonBorder
        color: theme.welcomeButtonBackground
    }

    Accessible.role: Accessible.Button
    Accessible.name: text
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
}
