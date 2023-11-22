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

    signal addRemoveClicked
    property var currentChat: ChatListModel.currentChat

    Label {
        id: listLabel
        anchors.top: parent.top
        anchors.left: parent.left
        text: qsTr("Local Documents:")
        font.pixelSize: theme.fontSizeLarge
        color: theme.textColor
    }

    ScrollView {
        id: scrollView
        anchors.top: listLabel.bottom
        anchors.topMargin: 20
        anchors.bottom: collectionSettings.top
        anchors.bottomMargin: 20
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
                    ToolTip.text: qsTr("Warning: searching collections while indexing can return incomplete results")
                    ToolTip.visible: hovered && model.indexing
                }
                Text {
                    id: collectionId
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: checkBox.right
                    anchors.margins: 20
                    anchors.leftMargin: 10
                    text: collection
                    font.pixelSize: theme.fontSizeLarge
                    elide: Text.ElideRight
                    color: theme.textColor
                }
                ProgressBar {
                    id: itemProgressBar
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: collectionId.right
                    anchors.right: parent.right
                    anchors.margins: 20
                    anchors.leftMargin: 40
                    visible: model.indexing
                    value: (model.totalBytesToIndex - model.currentBytesToIndex) / model.totalBytesToIndex
                    background: Rectangle {
                        implicitHeight: 45
                        color: theme.backgroundDarkest
                        radius: 3
                    }
                    contentItem: Item {
                        implicitHeight: 40

                        Rectangle {
                            width: itemProgressBar.visualPosition * parent.width
                            height: parent.height
                            radius: 2
                            color: theme.assistantColor
                        }
                    }
                    Accessible.role: Accessible.ProgressBar
                    Accessible.name: qsTr("Indexing progressBar")
                    Accessible.description: qsTr("Shows the progress made in the indexing")
                }
                Label {
                    id: speedLabel
                    color: theme.textColor
                    visible: model.indexing
                    anchors.verticalCenter: itemProgressBar.verticalCenter
                    anchors.left: itemProgressBar.left
                    anchors.right: itemProgressBar.right
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("indexing...")
                    elide: Text.ElideRight
                    font.pixelSize: theme.fontSizeLarge
                }
            }
        }
    }

    MyButton {
        id: collectionSettings
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        text: qsTr("Add & Remove")
        font.pixelSize: theme.fontSizeLarger
        onClicked: {
            addRemoveClicked()
        }
    }
}
