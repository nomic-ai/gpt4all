import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import localdocs

Item {
    AddCollectionDialog {
        id: addCollectionDialog
    }

    Connections {
        target: addCollectionDialog
        function onAccepted() {
            LocalDocs.addFolder(addCollectionDialog.collection, addCollectionDialog.folder_path)
        }
    }

    GridLayout {
        id: gridLayout
        columns: 2
        rowSpacing: 10
        columnSpacing: 10

        Label {
            id: contextItemsPerPrompt
            text: qsTr("Context items per prompt:")
            color: theme.textColor
            Layout.row: 0
            Layout.column: 0
        }

        MyTextField {
            Layout.row: 0
            Layout.column: 1
        }

        Label {
            id: chunkLabel
            text: qsTr("Chunksize:")
            color: theme.textColor
            Layout.row: 1
            Layout.column: 0
        }

        MyTextField {
            id: chunkSizeTextField
            Layout.row: 1
            Layout.column: 1
        }
    }

    ScrollView {
        id: scrollView
        anchors.top: gridLayout.bottom
        anchors.topMargin: 20
        anchors.bottom: newCollectionButton.top
        anchors.bottomMargin: 10
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
            headerPositioning: ListView.InlineHeader
            header: Rectangle {
                width: listView.width
                height: collectionLabel.height + 40
                color: theme.backgroundDark
                Label {
                    id: collectionLabel
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.margins: 20
                    text: "Collection"
                    color: theme.textColor
                    font.bold: true
                    width: 200
                }

                Label {
                    anchors.left: collectionLabel.right
                    anchors.margins: 20
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Folder"
                    color: theme.textColor
                    font.bold: true
                }
            }

            delegate: Rectangle {
                id: item
                width: listView.width
                height: buttons.height + 20
                color: index % 2 === 0 ? theme.backgroundLight : theme.backgroundLighter
                property bool removing: false

                Text {
                    id: collectionId
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.margins: 20
                    text: collection
                    elide: Text.ElideRight
                    color: theme.textColor
                    width: 200
                }

                Text {
                    id: folderId
                    anchors.left: collectionId.right
                    anchors.margins: 20
                    anchors.verticalCenter: parent.verticalCenter
                    text: folder_path
                    elide: Text.ElideRight
                    color: theme.textColor
                }

                Item {
                    id: buttons
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: 20
                    width: childrenRect.width
                    height: Math.max(removeButton.height, busyIndicator.height)
                    MyButton {
                        id: removeButton
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Remove")
                        visible: !item.removing
                        onClicked: {
                            item.removing = true
                            LocalDocs.removeFolder(collection, folder_path)
                        }
                    }
                    BusyIndicator {
                        id: busyIndicator
                        anchors.verticalCenter: parent.verticalCenter
                        visible: item.removing
                    }
                }
            }
        }
    }

    MyButton {
        id: newCollectionButton
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        text: qsTr("New collection")
        onClicked: {
            addCollectionDialog.open();
        }
    }

    MyButton {
        id: restoreDefaultsButton
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        text: qsTr("Restore Defaults")
        Accessible.role: Accessible.Button
        Accessible.name: text
        Accessible.description: qsTr("Restores the settings dialog to a default state")
        onClicked: {
            //                settingsDialog.restoreGenerationDefaults()
        }
    }
}
