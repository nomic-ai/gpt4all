import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts

Dialog {
    id: addCollectionDialog
    anchors.centerIn: parent
    opacity: 0.9
    padding: 20
    modal: true

    Theme {
        id: theme
    }

    property string collection: ""
    property string folder_path: ""

    FolderDialog {
        id: folderDialog
        title: "Please choose a directory"
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: {
            addCollectionDialog.folder_path = selectedFolder
        }
    }

    Row {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: childrenRect.height
        spacing: 20

        TextField {
            id: collection
            implicitWidth: 200
            horizontalAlignment: Text.AlignJustify
            color: theme.textColor
            background: Rectangle {
                implicitWidth: 150
                color: theme.backgroundLighter
                radius: 10
            }
            padding: 10
            placeholderText: qsTr("Collection name...")
            placeholderTextColor: theme.mutedTextColor
            ToolTip.text: qsTr("Name of the collection to add (Required)")
            ToolTip.visible: hovered
            onEditingFinished: {
                addCollectionDialog.collection = text
            }
            Accessible.role: Accessible.EditableText
            Accessible.name: collection.text
            Accessible.description: ToolTip.text
        }

        MyTextField {
            id: folderLabel
            text: folder_path
            readOnly: true
            color: theme.textColor
            implicitWidth: 300
            padding: 10
            placeholderText: qsTr("Folder path...")
            placeholderTextColor: theme.mutedTextColor
            ToolTip.text: qsTr("Folder path to documents (Required)")
            ToolTip.visible: hovered
        }

        MyButton {
            text: qsTr("Browse")
            onClicked: {
                folderDialog.open();
            }
        }

        MyButton {
            text: qsTr("Add")
            enabled: addCollectionDialog.collection !== "" && addCollectionDialog.folder_path != ""
            Accessible.role: Accessible.Button
            Accessible.name: text
            Accessible.description: qsTr("Add button")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    background: Rectangle {
        anchors.fill: parent
        color: theme.backgroundDarkest
        border.width: 1
        border.color: theme.dialogBorder
        radius: 10
    }
}