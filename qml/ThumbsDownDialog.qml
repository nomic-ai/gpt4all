import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import download
import network
import llm

Dialog {
    id: thumbsDownDialog
    modal: true
    opacity: 0.9
    padding: 20
    width: 900
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
                source: "qrc:/gpt4all-chat/icons/thumbs_down.svg"
            }
            Text {
                anchors.left: img.right
                anchors.leftMargin: 30
                anchors.verticalCenter: img.verticalCenter
                text: qsTr("Provide feedback for negative rating")
                color: "#d1d5db"
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
                color: "#dadadc"
                padding: 20
                width: parent.width
                height: 300
                wrapMode: Text.Wrap
                font.pixelSize: 24
                placeholderText: qsTr("Please provide a better response...")
                placeholderTextColor: "#7d7d8e"
                background: Rectangle {
                    color: "#40414f"
                    radius: 10
                }
            }
        }
    }

    background: Rectangle {
        anchors.fill: parent
        color: "#202123"
        border.width: 1
        border.color: "white"
        radius: 10
    }

    footer: DialogButtonBox {
        padding: 20
        alignment: Qt.AlignRight
        spacing: 10
        Button {
            text: qsTr("Submit")
            background: Rectangle {
                border.color: "#7d7d8e"
                border.width: 1
                radius: 10
                color: "#343541"
            }
            padding: 15
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: qsTr("Cancel")
            background: Rectangle {
                border.color: "#7d7d8e"
                border.width: 1
                radius: 10
                color: "#343541"
            }
            padding: 15
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        background: Rectangle {
            color: "transparent"
        }
    }
}