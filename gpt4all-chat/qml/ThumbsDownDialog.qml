import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import download
import network
import llm

Dialog {
    id: thumbsDownDialog
    modal: true
    opacity: 0.9
    padding: 20

    Theme {
        id: theme
    }

    property alias response: thumbsDownNewResponse.text

    Column {
        anchors.fill: parent
        spacing: 20
        Item {
            width: childrenRect.width
            height: childrenRect.height
            Image {
                id: img
                anchors.top: parent.top
                anchors.left: parent.left
                width: 60
                height: 60
                source: "qrc:/gpt4all/icons/thumbs_down.svg"
            }
            Text {
                anchors.left: img.right
                anchors.leftMargin: 30
                anchors.verticalCenter: img.verticalCenter
                text: qsTr("Please edit the text below to provide a better response. (optional)")
                color: theme.textColor
            }
        }

        ScrollView {
            clip: true
            height: 300
            width: parent.width
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            TextArea {
                id: thumbsDownNewResponse
                color: theme.textColor
                padding: 20
                wrapMode: Text.Wrap
                font.pixelSize: theme.fontSizeLarge
                placeholderText: qsTr("Please provide a better response...")
                placeholderTextColor: theme.backgroundLightest
                background: Rectangle {
                    color: theme.backgroundLighter
                    radius: 10
                }
            }
        }
    }

    background: Rectangle {
        anchors.fill: parent
        color: theme.backgroundDarkest
        border.width: 1
        border.color: theme.dialogBorder
        radius: 10
    }

    footer: DialogButtonBox {
        padding: 20
        alignment: Qt.AlignRight
        spacing: 10
        Button {
            contentItem: Text {
                color: theme.textColor
                text: qsTr("Submit")
            }
            background: Rectangle {
                border.color: theme.backgroundLightest
                border.width: 1
                radius: 10
                color: theme.backgroundLight
            }
            padding: 15
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            contentItem: Text {
                color: theme.textColor
                text: qsTr("Cancel")
            }
            background: Rectangle {
                border.color: theme.backgroundLightest
                border.width: 1
                radius: 10
                color: theme.backgroundLight
            }
            padding: 15
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        background: Rectangle {
            color: "transparent"
        }
    }
}