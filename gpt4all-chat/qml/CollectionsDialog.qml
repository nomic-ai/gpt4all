import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import chatlistmodel
import localdocs
import llm

MyDialog {
    id: collectionsDialog
    modal: true
    padding: 20
    width: 480
    height: 640

    property var currentChat: ChatListModel.currentChat

    Label {
        id: listLabel
        anchors.top: parent.top
        anchors.left: parent.left
        text: "Available LocalDocs Collections:"
        font.pixelSize: theme.fontSizeLarge
        color: theme.textColor
    }

    ScrollView {
        id: scrollView
        anchors.top: listLabel.bottom
        anchors.topMargin: 20
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        contentHeight: 300
        ScrollBar.vertical.policy: ScrollBar.AlwaysOn

        background: Rectangle {
            color: theme.backgroundLighter
        }

        ListView {
            id: listView
            model: LocalDocs.localDocsModel
            boundsBehavior: Flickable.StopAtBounds
            delegate: Rectangle {
                id: item
                width: listView.width
                height: collectionId.height + 40
                color: index % 2 === 0 ? theme.backgroundLight : theme.backgroundLighter
                MyCheckBox {
                    id: checkBox
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.margins: 20
                    checked: currentChat.hasCollection(collection)
                    onClicked: {
                        if (checkBox.checked) {
                            currentChat.addCollection(collection)
                        } else {
                            currentChat.removeCollection(collection)
                        }
                    }
                }
                Text {
                    id: collectionId
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: checkBox.right
                    anchors.margins: 20
                    text: collection
                    font.pixelSize: theme.fontSizeLarge
                    elide: Text.ElideRight
                    color: theme.textColor
                }
            }
        }
    }
}
