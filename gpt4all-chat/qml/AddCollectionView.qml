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
        spacing: 20

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: 50

            MyButton {
                id: backButton
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                text: qsTr("\u2190 Existing Collections")

                borderWidth: 0
                backgroundColor: theme.lighterButtonBackground
                backgroundColorHovered: theme.lighterButtonBackgroundHovered
                backgroundRadius: 5
                padding: 15
                topPadding: 8
                bottomPadding: 8
                textColor: theme.lighterButtonForeground
                fontPixelSize: theme.fontSizeLarge
                fontPixelBold: true

                onClicked: {
                    localDocsViewRequested()
                }
            }
        }

        Text {
            id: addDocBanner
            Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
            horizontalAlignment: Qt.AlignHCenter
            text: qsTr("Add Document Collection")
            font.pixelSize: theme.fontSizeBanner
            color: theme.titleTextColor
        }

        Text {
            Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
            Layout.maximumWidth: addDocBanner.width
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignJustify
            text: qsTr("Add a folder containing plain text files, PDFs, or Markdown. Configure additional extensions in Settings.")
            font.pixelSize: theme.fontSizeLarger
            color: theme.titleInfoTextColor
        }

        GridLayout {
            id: root
            Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
            rowSpacing: 50
            columnSpacing: 20

            property alias collection: collection.text
            property alias folder_path: folderEdit.text

            MyFolderDialog {
                id: folderDialog
            }

            Label {
                Layout.row: 2
                Layout.column: 0
                text: qsTr("Name")
                font.bold: true
                font.pixelSize: theme.fontSizeLarger
                color: theme.settingsTitleTextColor
            }

            MyTextField {
                id: collection
                Layout.row: 2
                Layout.column: 1
                Layout.minimumWidth: 400
                Layout.alignment: Qt.AlignRight
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

            Label {
                Layout.row: 3
                Layout.column: 0
                text: qsTr("Folder")
                font.bold: true
                font.pixelSize: theme.fontSizeLarger
                color: theme.settingsTitleTextColor
            }

            RowLayout {
                Layout.row: 3
                Layout.column: 1
                Layout.minimumWidth: 400
                Layout.maximumWidth: 400
                Layout.alignment: Qt.AlignRight
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
                        folderDialog.openFolderDialog(StandardPaths.writableLocation(StandardPaths.HomeLocation), function(selectedFolder) {
                            root.folder_path = selectedFolder
                        })
                    }
                }
            }

            MyButton {
                Layout.row: 4
                Layout.column: 1
                Layout.alignment: Qt.AlignRight
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
        }
    }
}
