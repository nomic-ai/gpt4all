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
                backgroundColorHovered: toggled ? backgroundColor : theme.iconBackgroundViewBarHovered
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                Layout.alignment: Qt.AlignCenter
                toggledWidth: 0
                toggled: stackLayout.currentIndex === 0
                toggledColor: theme.iconBackgroundViewBarToggled
                scale: 1.5
                source: "qrc:/gpt4all/icons/chat.svg"
                Accessible.name: qsTr("Chat view")
                Accessible.description: qsTr("Chat view to interact with models")
                onClicked: {
                    stackLayout.currentIndex = 0
                }
            }

            MyToolButton {
                id: searchButton
                backgroundColor: toggled ? theme.iconBackgroundViewBarToggled : theme.iconBackgroundViewBar
                backgroundColorHovered: toggled ? backgroundColor : theme.iconBackgroundViewBarHovered
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                toggledWidth: 0
                toggled: stackLayout.currentIndex === 1
                toggledColor: theme.iconBackgroundViewBarToggled
                scale: 1.5
                source: "qrc:/gpt4all/icons/models.svg"
                Accessible.name: qsTr("Search")
                Accessible.description: qsTr("Launch a dialog to download new models")
                onClicked: {
                    stackLayout.currentIndex = 1
                }
            }

            MyToolButton {
                id: settingsButton
                backgroundColor: toggled ? theme.iconBackgroundViewBarToggled : theme.iconBackgroundViewBar
                backgroundColorHovered: toggled ? backgroundColor : theme.iconBackgroundViewBarHovered
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                toggledWidth: 0
                toggledColor: theme.iconBackgroundViewBarToggled
                toggled: stackLayout.currentIndex === 2
                scale: 1.5
                source: "qrc:/gpt4all/icons/settings.svg"
                Accessible.name: qsTr("Settings")
                Accessible.description: qsTr("Reveals a dialogue with settings")

                onClicked: {
                    stackLayout.currentIndex = 2
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
                backgroundColor: toggled ? theme.iconBackgroundViewBarToggled : theme.iconBackgroundViewBar
                backgroundColorHovered: toggled ? backgroundColor : theme.iconBackgroundViewBarHovered
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

    StackLayout {
        id: stackLayout
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: viewBar.right
        anchors.right: parent.right

        ChatView {
            id: chatView
            Layout.fillWidth: true
            Layout.fillHeight: true

            Connections {
                target: chatView
                function onDownloadViewRequested(showEmbeddingModels) {
                    console.log("onDownloadViewRequested")
                    stackLayout.currentIndex = 1;
                    if (showEmbeddingModels)
                        downloadView.showEmbeddingModels();
                }
                function onSettingsViewRequested(page) {
                    settingsDialog.pageToDisplay = page;
                    stackLayout.currentIndex = 2;
                }
            }
        }

        ModelDownloaderView {
            id: downloadView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Item {
                Accessible.role: Accessible.Dialog
                Accessible.name: qsTr("Download new models")
                Accessible.description: qsTr("View for downloading new models")
            }
        }

        SettingsView {
            id: settingsDialog
            Layout.fillWidth: true
            Layout.fillHeight: true
            onDownloadClicked: {
                stackLayout.currentIndex = 1
                downloadView.showEmbeddingModels()
            }
        }
    }
}
