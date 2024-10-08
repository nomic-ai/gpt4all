import QtCore
import QtQuick
import QtQuick.Dialogs

FileDialog {
    id: fileDialog
    title: qsTr("Please choose a file")
    property var acceptedConnection: null

    function openFileDialog(currentFolder, onAccepted) {
        fileDialog.currentFolder = currentFolder;
        if (acceptedConnection !== null) {
            fileDialog.accepted.disconnect(acceptedConnection);
        }
        acceptedConnection = function() { onAccepted(fileDialog.selectedFile); };
        fileDialog.accepted.connect(acceptedConnection);
        fileDialog.open();
    }
}
