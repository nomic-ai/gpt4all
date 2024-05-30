import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.folderlistmodel
import Qt5Compat.GraphicalEffects
import llm
import chatlistmodel
import download
import modellist
import network
import gpt4all
import mysettings
import localdocs

Rectangle {
    id: addCollectionView

    Theme {
        id: theme
    }

    color: theme.viewBackground
    signal localDocsViewRequested()

    ColumnLayout {
        id: mainArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 30
        spacing: 50

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: 50

            MyButton {
                id: backButton
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                text: qsTr("\u2190 Existing Collections")

                borderWidth: 0
                backgroundColor: theme.collectionButtonBackground
                backgroundColorHovered: theme.collectionButtonBackgroundHovered
                backgroundRadius: 5
                padding: 15
                topPadding: 8
                bottomPadding: 8
                textColor: theme.green600
                fontPixelSize: theme.fontSizeLarge
                fontPixelBold: true

                background: Rectangle {
                    radius: backButton.backgroundRadius
                    color: backButton.toggled ? backButton.backgroundColorHovered : collectionsButton.backgroundColor
                }

                onClicked: {
                    localDocsViewRequested()
                }
            }
        }

        ColumnLayout {
            id: root
            Layout.alignment: Qt.AlignTop | Qt.AlignCenter
            spacing: 50

            property alias collection: collection.text
            property alias folder_path: folderEdit.text

            FolderDialog {
                id: folderDialog
                title: qsTr("Please choose a directory")
            }

            function openFolderDialog(currentFolder, onAccepted) {
                folderDialog.currentFolder = currentFolder;
                folderDialog.accepted.connect(function() { onAccepted(folderDialog.currentFolder); });
                folderDialog.open();
            }

            Text {
                horizontalAlignment: Qt.AlignHCenter
                text: qsTr("New Local Doc\nCollection")
                font.pixelSize: theme.fontSizeBanner
                color: theme.titleTextColor
            }

            MyTextField {
                id: collection
                Layout.alignment: Qt.AlignCenter
                Layout.minimumWidth: 400
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

            RowLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.minimumWidth: 400
                Layout.maximumWidth: 400
                spacing: 10
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

                MySettingsButton {
                    id: browseButton
                    text: qsTr("Browse")
                    onClicked: {
                        root.openFolderDialog(StandardPaths.writableLocation(StandardPaths.HomeLocation), function(selectedFolder) {
                            root.folder_path = selectedFolder
                        })
                    }
                }
            }

            MyButton {
                Layout.alignment: Qt.AlignCenter
                Layout.minimumWidth: 400
                text: qsTr("Create Collection")
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
                    localDocsViewRequested()
                }
            }

            // FIXME: Must include advanced settings
        }
    }
}
