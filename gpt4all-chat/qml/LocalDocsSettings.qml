import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import localdocs
import mysettings
import network

MySettingsTab {
    onRestoreDefaultsClicked: {
        MySettings.restoreLocalDocsDefaults();
    }

    title: qsTr("LocalDocs Plugin (BETA)")
    contentItem: ColumnLayout {
        id: root
        spacing: 10

        property alias collection: collection.text
        property alias folder_path: folderEdit.text

        Item {
            Layout.fillWidth: true
            height: row.height
            RowLayout {
                id: row
                anchors.left: parent.left
                anchors.right: parent.right
                height: collection.height
                spacing: 10
                MyTextField {
                    id: collection
                    width: 225
                    horizontalAlignment: Text.AlignJustify
                    color: theme.textColor
                    font.pixelSize: theme.fontSizeLarge
                    placeholderText: qsTr("Collection name...")
                    placeholderTextColor: theme.mutedTextColor
                    ToolTip.text: qsTr("Name of the collection to add (Required)")
                    ToolTip.visible: hovered
                    Accessible.role: Accessible.EditableText
                    Accessible.name: collection.text
                    Accessible.description: ToolTip.text
                    function showError() {
                        collection.placeholderTextColor = theme.textErrorColor
                    }
                    onTextChanged: {
                        collection.placeholderTextColor = theme.mutedTextColor
                    }
                }

                MyDirectoryField {
                    id: folderEdit
                    Layout.fillWidth: true
                    text: root.folder_path
                    placeholderText: qsTr("Folder path...")
                    font.pixelSize: theme.fontSizeLarge
                    placeholderTextColor: theme.mutedTextColor
                    ToolTip.text: qsTr("Folder path to documents (Required)")
                    ToolTip.visible: hovered
                    function showError() {
                        folderEdit.placeholderTextColor = theme.textErrorColor
                    }
                    onTextChanged: {
                        folderEdit.placeholderTextColor = theme.mutedTextColor
                    }
                }

                MyButton {
                    id: browseButton
                    text: qsTr("Browse")
                    onClicked: {
                        openFolderDialog(StandardPaths.writableLocation(StandardPaths.HomeLocation), function(selectedFolder) {
                            root.folder_path = selectedFolder
                        })
                    }
                }

                MyButton {
                    id: addButton
                    text: qsTr("Add")
                    Accessible.role: Accessible.Button
                    Accessible.name: text
                    Accessible.description: qsTr("Add button")
                    onClicked: {
                        var isError = false;
                        if (root.collection === "") {
                            isError = true;
                            collection.showError();
                        }
                        if (root.folder_path === "" || !folderEdit.isValid) {
                            isError = true;
                            folderEdit.showError();
                        }
                        if (isError)
                            return;
                        LocalDocs.addFolder(root.collection, root.folder_path)
                        root.collection = ""
                        root.folder_path = ""
                        collection.clear()
                    }
                }
            }
        }

        ColumnLayout {
            spacing: 0
            Repeater {
                model: LocalDocs.localDocsModel
                delegate: Rectangle {
                    id: item
                    Layout.fillWidth: true
                    height: buttons.height + 20
                    color: index % 2 === 0 ? theme.backgroundDark : theme.backgroundDarker
                    property bool removing: false

                    Text {
                        id: collectionId
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.margins: 20
                        text: collection
                        elide: Text.ElideRight
                        color: theme.textColor
                        font.pixelSize: theme.fontSizeLarge
                        width: 200
                    }

                    Text {
                        id: folderId
                        anchors.left: collectionId.right
                        anchors.right: buttons.left
                        anchors.margins: 20
                        anchors.verticalCenter: parent.verticalCenter
                        text: folder_path
                        elide: Text.ElideRight
                        color: theme.textColor
                        font.pixelSize: theme.fontSizeLarge
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

        RowLayout {
            Label {
                id: showReferencesLabel
                text: qsTr("Show references:")
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
            }
            MyCheckBox {
                id: showReferencesBox
                checked: MySettings.localDocsShowReferences
                onClicked: {
                    MySettings.localDocsShowReferences = !MySettings.localDocsShowReferences
                }
                ToolTip.text: qsTr("Shows any references in GUI generated by localdocs")
                ToolTip.visible: hovered
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: theme.tabBorder
        }
    }
    advancedSettings: GridLayout {
        id: gridLayout
        columns: 3
        rowSpacing: 10
        columnSpacing: 10

        Rectangle {
            Layout.row: 3
            Layout.column: 0
            Layout.fillWidth: true
            Layout.columnSpan: 3
            height: 1
            color: theme.tabBorder
        }

        Label {
            id: chunkLabel
            Layout.row: 1
            Layout.column: 0
            color: theme.textColor
            font.pixelSize: theme.fontSizeLarge
            text: qsTr("Document snippet size (characters):")
        }

        MyTextField {
            id: chunkSizeTextField
            Layout.row: 1
            Layout.column: 1
            ToolTip.text: qsTr("Number of characters per document snippet.\nNOTE: larger numbers increase likelihood of factual responses, but also result in slower generation.")
            ToolTip.visible: hovered
            text: MySettings.localDocsChunkSize
            validator: IntValidator {
                bottom: 1
            }
            onEditingFinished: {
                var val = parseInt(text)
                if (!isNaN(val)) {
                    MySettings.localDocsChunkSize = val
                    focus = false
                } else {
                    text = MySettings.localDocsChunkSize
                }
            }
        }

        Label {
            id: contextItemsPerPrompt
            Layout.row: 2
            Layout.column: 0
            color: theme.textColor
            font.pixelSize: theme.fontSizeLarge
            text: qsTr("Document snippets per prompt:")
        }

        MyTextField {
            Layout.row: 2
            Layout.column: 1
            ToolTip.text: qsTr("Best N matches of retrieved document snippets to add to the context for prompt.\nNOTE: larger numbers increase likelihood of factual responses, but also result in slower generation.")
            ToolTip.visible: hovered
            text: MySettings.localDocsRetrievalSize
            validator: IntValidator {
                bottom: 1
            }
            onEditingFinished: {
                var val = parseInt(text)
                if (!isNaN(val)) {
                    MySettings.localDocsRetrievalSize = val
                    focus = false
                } else {
                    text = MySettings.localDocsRetrievalSize
                }
            }
        }

        Item {
            Layout.row: 1
            Layout.column: 2
            Layout.rowSpan: 2
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            Layout.minimumHeight: warningLabel.height
            Label {
                id: warningLabel
                width: parent.width
                color: theme.textErrorColor
                wrapMode: Text.WordWrap
                text: qsTr("Warning: Advanced usage only. Values too large may cause localdocs failure, extremely slow responses or failure to respond at all. Roughly speaking, the {N chars x N snippets} are added to the model's context window. More info <a href=\"https://docs.gpt4all.io/gpt4all_chat.html#localdocs-beta-plugin-chat-with-your-data\">here.</a>")
                onLinkActivated: function(link) { Qt.openUrlExternally(link) }
            }
        }
    }
}
