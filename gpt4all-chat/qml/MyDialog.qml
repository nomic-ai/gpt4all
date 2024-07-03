import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts

Dialog {
    id: myDialog
    parent: Overlay.overlay
    property alias closeButtonVisible: myCloseButton.visible
    background: Rectangle {
        width: parent.width
        height: parent.height
        color: theme.containerBackground
        border.width: 1
        border.color: theme.dialogBorder
        radius: 10
    }

    Rectangle {
        id: closeBackground
        visible: myCloseButton.visible
        z: 299
        anchors.centerIn: myCloseButton
        width: myCloseButton.width + 10
        height: myCloseButton.height + 10
        color: theme.containerBackground
    }

    MyToolButton {
        id: myCloseButton
        x: 0 + myDialog.width - myDialog.padding - width - 15
        y: 0 - myDialog.padding + 15
        z: 300
        visible: myDialog.closePolicy != Popup.NoAutoClose
        width: 24
        height: 24
        imageWidth: 24
        imageHeight: 24
        padding: 0
        source: "qrc:/gpt4all/icons/close.svg"
        fillMode: Image.PreserveAspectFit
        onClicked: {
            myDialog.close();
        }
    }
}
