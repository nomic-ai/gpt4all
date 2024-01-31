import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import download
import network
import llm

MyDialog {
    id: thumbsDownDialog
    modal: true
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
                font.pixelSize: theme.fontSizeLarge
            }
        }

        ScrollView {
            clip: true
            height: 300
            width: parent.width
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            MyTextArea {
                id: thumbsDownNewResponse
                placeholderText: qsTr("Please provide a better response...")
            }
        }
    }

    footer: DialogButtonBox {
        padding: 20
        alignment: Qt.AlignRight
        spacing: 10
        MySettingsButton {
            text: qsTr("Submit")
            Accessible.description: qsTr("Submits the user's response")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        MySettingsButton {
            text: qsTr("Cancel")
            Accessible.description: qsTr("Closes the response dialog")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        background: Rectangle {
            color: "transparent"
        }
    }
}
