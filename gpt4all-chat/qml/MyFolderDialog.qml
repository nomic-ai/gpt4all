import QtCore
import QtQuick
import QtQuick.Dialogs

FolderDialog {
    id: folderDialog
    title: qsTr("Please choose a directory")

    function openFolderDialog(currentFolder, onAccepted) {
        folderDialog.currentFolder = currentFolder;
        folderDialog.accepted.connect(function() { onAccepted(folderDialog.selectedFolder); });
        folderDialog.open();
    }
}
