import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import localdocs

Item {
    id: root

    property alias collection: collection.text
    property alias folder_path: folderEdit.text

    property int defaultChunkSize: 256
    property int defaultRetrievalSize: 3

    property alias chunkSize: settings.chunkSize
    property alias retrievalSize: settings.retrievalSize

    Settings {
        id: settings
        category: "localdocs"
        property int chunkSize: root.defaultChunkSize
        property int retrievalSize: root.defaultRetrievalSize
    }

    function restoreLocalDocsDefaults() {
        settings.chunkSize = root.defaultChunkSize
        settings.retrievalSize = root.defaultRetrievalSize
        LocalDocs.chunkSize = settings.chunkSize
        LocalDocs.retrievalSize = settings.retrievalSize
        settings.sync()
    }

    FolderDialog {
        id: folderDialog
        title: "Please choose a directory"
        currentFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        onAccepted: {
            root.folder_path = selectedFolder
        }
    }

    Rectangle {
        id: addCollection
        anchors.left: parent.left
        anchors.right: parent.right
        height: collection.height + 20
        color: theme.backgroundDark

        MyTextField {
            id: collection
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            width: 225
            horizontalAlignment: Text.AlignJustify
            color: theme.textColor
            placeholderText: qsTr("Collection name...")
            placeholderTextColor: theme.mutedTextColor
            ToolTip.text: qsTr("Name of the collection to add (Required)")
            ToolTip.visible: hovered
            Accessible.role: Accessible.EditableText
            Accessible.name: collection.text
            Accessible.description: ToolTip.text
        }

        MyDirectoryField {
            id: folderEdit
            anchors.left: collection.right
            anchors.leftMargin: 10
            anchors.right: browseButton.left
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            text: root.folder_path
            placeholderText: qsTr("Folder path...")
            placeholderTextColor: theme.mutedTextColor
            ToolTip.text: qsTr("Folder path to documents (Required)")
            ToolTip.visible: hovered
        }

        MyButton {
            id: browseButton
            text: qsTr("Browse")
            anchors.right: addButton.left
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            onClicked: {
                folderDialog.open();
            }
        }

        MyButton {
            id: addButton
            text: qsTr("Add")
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            enabled: root.collection !== "" && root.folder_path !== "" && folderEdit.isValid
            Accessible.role: Accessible.Button
            Accessible.name: text
            Accessible.description: qsTr("Add button")
            onClicked: {
                LocalDocs.addFolder(root.collection, root.folder_path)
                root.collection = ""
                root.folder_path = ""
                collection.clear()
            }
        }
    }

    ScrollView {
        id: scrollView
        anchors.top: addCollection.bottom
        anchors.bottom: gridLayout.top
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
                    width: Math.max(removeButton.width, busyIndicator.width)
                    height: Math.max(removeButton.height, busyIndicator.height)
                    MyButton {
                        id: removeButton
                        anchors.centerIn: parent
                        text: qsTr("Remove")
                        visible: !item.removing && installed
                        onClicked: {
                            item.removing = true
                            LocalDocs.removeFolder(collection, folder_path)
                        }
                    }
                    MyBusyIndicator {
                        id: busyIndicator
                        anchors.centerIn: parent
                        visible: item.removing || !installed
                    }
                }
            }
        }
    }

    GridLayout {
        id: gridLayout
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        columns: 3
        rowSpacing: 10
        columnSpacing: 10

        Label {
            id: chunkLabel
            Layout.row: 0
            Layout.column: 0
            color: theme.textColor
            text: qsTr("Document snippet size (characters):")
        }

        MyTextField {
            id: chunkSizeTextField
            Layout.row: 0
            Layout.column: 1
            ToolTip.text: qsTr("Number of characters per document snippet.\nNOTE: larger numbers increase likelihood of factual responses, but also result in slower generation.")
            ToolTip.visible: hovered
            text: settings.chunkSize.toString()
            validator: IntValidator {
                bottom: 1
            }
            onEditingFinished: {
                var val = parseInt(text)
                if (!isNaN(val)) {
                    settings.chunkSize = val
                    settings.sync()
                    focus = false
                    LocalDocs.chunkSize = settings.chunkSize
                } else {
                    text = settings.chunkSize.toString()
                }
            }
        }

        Label {
            id: contextItemsPerPrompt
            Layout.row: 1
            Layout.column: 0
            color: theme.textColor
            text: qsTr("Document snippets to process per prompt:")
        }

        MyTextField {
            Layout.row: 1
            Layout.column: 1
            ToolTip.text: qsTr("Best N matches of retrieved document snippets to add to the context for prompt.\nNOTE: larger numbers increase likelihood of factual responses, but also result in slower generation.")
            ToolTip.visible: hovered
            text: settings.retrievalSize.toString()
            validator: IntValidator {
                bottom: 1
            }
            onEditingFinished: {
                var val = parseInt(text)
                if (!isNaN(val)) {
                    settings.retrievalSize = val
                    settings.sync()
                    focus = false
                    LocalDocs.retrievalSize = settings.retrievalSize
                } else {
                    text = settings.retrievalSize.toString()
                }
            }
        }

        MyButton {
            id: restoreDefaultsButton
            Layout.row: 2
            Layout.column: 1
            Layout.fillWidth: true
            text: qsTr("Restore Defaults")
            Accessible.role: Accessible.Button
            Accessible.name: text
            Accessible.description: qsTr("Restores the settings dialog to a default state")
            onClicked: {
                root.restoreLocalDocsDefaults();
            }
        }
    }
}
