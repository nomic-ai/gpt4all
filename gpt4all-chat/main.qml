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

    SettingsDialog {
        id: settingsDialog
        anchors.centerIn: parent
        width: Math.min(1920, window.width - (window.width * .1))
        height: window.height - (window.height * .1)
        onDownloadClicked: {
            downloadNewModels.showEmbeddingModels = true
            downloadNewModels.open()
        }
    }

    ModelDownloaderDialog {
        id: downloadNewModels
        anchors.centerIn: parent
        width: Math.min(1920, window.width - (window.width * .1))
        height: window.height - (window.height * .1)
        Item {
            Accessible.role: Accessible.Dialog
            Accessible.name: qsTr("Download new models")
            Accessible.description: qsTr("Dialog for downloading new models")
        }
    }

    NetworkDialog {
        id: networkDialog
        anchors.centerIn: parent
        width: Math.min(1024, window.width - (window.width * .2))
        height: Math.min(600, window.height - (window.height * .2))
        Item {
            Accessible.role: Accessible.Dialog
            Accessible.name: qsTr("Network dialog")
            Accessible.description: qsTr("opt-in to share feedback/conversations")
        }
    }

    AboutDialog {
        id: aboutDialog
        anchors.centerIn: parent
        width: Math.min(1024, window.width - (window.width * .2))
        height: Math.min(600, window.height - (window.height * .2))
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

    Rectangle {
        id: viewBar
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 80
        color: theme.viewBarBackground

        ColumnLayout {
            id: viewsLayout
            anchors.top: parent.top
            anchors.topMargin: 30
            anchors.horizontalCenter: parent.horizontalCenter
            Layout.margins: 0
            spacing: 25

            MyToolButton {
                id: chatButton
                backgroundColor: toggled ? theme.iconBackgroundViewBarToggled : theme.iconBackgroundViewBar
                backgroundColorHovered: theme.iconBackgroundViewBarHovered
                toggledWidth: 0
                toggled: true
                Layout.preferredWidth: 50
                Layout.preferredHeight: 50
                scale: 1.5
                source: "qrc:/gpt4all/icons/chat.svg"
                Accessible.name: qsTr("Chat view")
                Accessible.description: qsTr("Chat view to interact with models")
                onClicked: {
                }
            }

            MyToolButton {
                id: searchButton
                backgroundColor: theme.iconBackgroundViewBar
                backgroundColorHovered: theme.iconBackgroundViewBarHovered
                Layout.preferredWidth: 50
                Layout.preferredHeight: 50
                scale: 1.5
                source: "qrc:/gpt4all/icons/search.svg"
                Accessible.name: qsTr("Search")
                Accessible.description: qsTr("Launch a dialog to download new models")
                onClicked: {
                    downloadNewModels.showEmbeddingModels = false
                    downloadNewModels.open()
                }
            }

            MyToolButton {
                id: settingsButton
                backgroundColor: theme.iconBackgroundViewBar
                backgroundColorHovered: theme.iconBackgroundViewBarHovered
                Layout.preferredWidth: 50
                Layout.preferredHeight: 50
                scale: 1.5
                source: "qrc:/gpt4all/icons/settings.svg"
                Accessible.name: qsTr("Settings")
                Accessible.description: qsTr("Reveals a dialogue with settings")

                onClicked: {
                    settingsDialog.open()
                }
            }
        }

        ColumnLayout {
            id: buttonsLayout
            anchors.bottom: parent.bottom
            anchors.margins: 0
            anchors.bottomMargin: 25
            anchors.horizontalCenter: parent.horizontalCenter
            Layout.margins: 0
            spacing: 25

            MyToolButton {
                id: networkButton
                backgroundColor: theme.iconBackgroundViewBar
                backgroundColorHovered: theme.iconBackgroundViewBarHovered
                toggledColor: theme.iconBackgroundViewBar
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                scale: 1.2
                toggled: MySettings.networkIsActive
                source: "qrc:/gpt4all/icons/network.svg"
                Accessible.name: qsTr("Network")
                Accessible.description: qsTr("Reveals a dialogue where you can opt-in for sharing data over network")

                onClicked: {
                    if (MySettings.networkIsActive) {
                        MySettings.networkIsActive = false
                        Network.sendNetworkToggled(false);
                    } else
                        networkDialog.open()
                }
            }

            MyToolButton {
                id: infoButton
                backgroundColor: theme.iconBackgroundViewBar
                backgroundColorHovered: theme.iconBackgroundViewBarHovered
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                scale: 1.2
                source: "qrc:/gpt4all/icons/info.svg"
                Accessible.name: qsTr("About")
                Accessible.description: qsTr("Reveals an about dialog")
                onClicked: {
                    aboutDialog.open()
                }
            }
        }
    }

    ChatView {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: viewBar.right
        anchors.right: parent.right
    }
}
