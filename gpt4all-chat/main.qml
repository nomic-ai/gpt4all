import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import llm
import chatlistmodel
import download
import modellist
import network
import gpt4all
import mysettings

Window {
    id: window
    width: 1920
    height: 1080
    minimumWidth: 720
    minimumHeight: 480
    visible: true
    title: qsTr("GPT4All v") + Qt.application.version

    Settings {
        property alias x: window.x
        property alias y: window.y
        property alias width: window.width
        property alias height: window.height
    }

    Theme {
        id: theme
    }

    property bool hasSaved: false

    PopupDialog {
        id: savingPopup
        anchors.centerIn: parent
        shouldTimeOut: false
        shouldShowBusy: true
        text: qsTr("Saving chats.")
        font.pixelSize: theme.fontSizeLarge
    }

    onClosing: function(close) {
        if (window.hasSaved)
            return;

        savingPopup.open();
        ChatListModel.saveChats();
        close.accepted = false
    }

    Connections {
        target: ChatListModel
        function onSaveChatsFinished() {
            window.hasSaved = true;
            savingPopup.close();
            window.close()
        }
    }

    color: theme.black

    ChatView {
        anchors.fill: parent
    }
}
