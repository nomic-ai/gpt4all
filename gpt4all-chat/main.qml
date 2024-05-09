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
import localdocs
import mysettings

Window {
    id: window
    width: 1920
    height: 1080
    minimumWidth: 1280
    minimumHeight: 720
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

    Item {
        Accessible.role: Accessible.Window
        Accessible.name: title
    }

    // Startup code
    Component.onCompleted: {
        startupDialogs();
    }

    Component.onDestruction: {
        Network.trackEvent("session_end")
    }

    Connections {
        target: firstStartDialog
        function onClosed() {
            startupDialogs();
        }
    }

    Connections {
        target: Download
        function onHasNewerReleaseChanged() {
            startupDialogs();
        }
    }

    property bool hasShownModelDownload: false
    property bool hasCheckedFirstStart: false
    property bool hasShownSettingsAccess: false

    function startupDialogs() {
        if (!LLM.compatHardware()) {
            Network.trackEvent("noncompat_hardware")
            errorCompatHardware.open();
            return;
        }

        // check if we have access to settings and if not show an error
        if (!hasShownSettingsAccess && !LLM.hasSettingsAccess()) {
            errorSettingsAccess.open();
            hasShownSettingsAccess = true;
            return;
        }

        // check for first time start of this version
        if (!hasCheckedFirstStart) {
            if (Download.isFirstStart(/*writeVersion*/ true)) {
                firstStartDialog.open();
                return;
            }

            // send startup or opt-out now that the user has made their choice
            Network.sendStartup()
            // start localdocs
            LocalDocs.requestStart()

            hasCheckedFirstStart = true
        }

        // check for any current models and if not, open download view once
        if (!hasShownModelDownload && ModelList.installedModels.count === 0 && !firstStartDialog.opened) {
            downloadViewRequested();
            hasShownModelDownload = true;
            return;
        }

        // check for new version
        if (Download.hasNewerRelease && !firstStartDialog.opened) {
            newVersionDialog.open();
            return;
        }
    }

    PopupDialog {
        id: errorCompatHardware
        anchors.centerIn: parent
        shouldTimeOut: false
        shouldShowBusy: false
        closePolicy: Popup.NoAutoClose
        modal: true
        text: qsTr("<h3>Encountered an error starting up:</h3><br>")
              + qsTr("<i>\"Incompatible hardware detected.\"</i>")
              + qsTr("<br><br>Unfortunately, your CPU does not meet the minimal requirements to run ")
              + qsTr("this program. In particular, it does not support AVX intrinsics which this ")
              + qsTr("program requires to successfully run a modern large language model. ")
              + qsTr("The only solution at this time is to upgrade your hardware to a more modern CPU.")
              + qsTr("<br><br>See here for more information: <a href=\"https://en.wikipedia.org/wiki/Advanced_Vector_Extensions\">")
              + qsTr("https://en.wikipedia.org/wiki/Advanced_Vector_Extensions</a>")
    }

    PopupDialog {
        id: errorSettingsAccess
        anchors.centerIn: parent
        shouldTimeOut: false
        shouldShowBusy: false
        modal: true
        text: qsTr("<h3>Encountered an error starting up:</h3><br>")
              + qsTr("<i>\"Inability to access settings file.\"</i>")
              + qsTr("<br><br>Unfortunately, something is preventing the program from accessing ")
              + qsTr("the settings file. This could be caused by incorrect permissions in the local ")
              + qsTr("app config directory where the settings file is located. ")
              + qsTr("Check out our <a href=\"https://discord.gg/4M2QFmTt2k\">discord channel</a> for help.")
    }

    StartupDialog {
        id: firstStartDialog
        anchors.centerIn: parent
    }

    NewVersionDialog {
        id: newVersionDialog
        anchors.centerIn: parent
    }

    Connections {
        target: Network
        function onHealthCheckFailed(code) {
            healthCheckFailed.open();
        }
    }

    PopupDialog {
        id: healthCheckFailed
        anchors.centerIn: parent
        text: qsTr("Connection to datalake failed.")
        font.pixelSize: theme.fontSizeLarge
    }

    Dialog {
        id: checkForUpdatesError
        anchors.centerIn: parent
        modal: false
        padding: 20
        Text {
            horizontalAlignment: Text.AlignJustify
            text: qsTr("ERROR: Update system could not find the MaintenanceTool used<br>
                   to check for updates!<br><br>
                   Did you install this application using the online installer? If so,<br>
                   the MaintenanceTool executable should be located one directory<br>
                   above where this application resides on your filesystem.<br><br>
                   If you can't start it manually, then I'm afraid you'll have to<br>
                   reinstall.")
            color: theme.textErrorColor
            font.pixelSize: theme.fontSizeLarge
            Accessible.role: Accessible.Dialog
            Accessible.name: text
            Accessible.description: qsTr("Error dialog")
        }
        background: Rectangle {
            anchors.fill: parent
            color: theme.containerBackground
            border.width: 1
            border.color: theme.dialogBorder
            radius: 10
        }
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

    color: theme.viewBarBackground

    Rectangle {
        id: viewBar
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 120
        color: theme.viewBarBackground

        ColumnLayout {
            id: viewsLayout
            anchors.top: parent.top
            anchors.topMargin: 30
            anchors.horizontalCenter: parent.horizontalCenter
            Layout.margins: 0
            spacing: 22

            MyToolButton {
                id: homeButton
                backgroundColor: toggled ? theme.iconBackgroundViewBarHovered : theme.iconBackgroundViewBar
                backgroundColorHovered: theme.iconBackgroundViewBarHovered
                Layout.preferredWidth: 70
                Layout.preferredHeight: 70
                Layout.alignment: Qt.AlignCenter
                toggledWidth: 0
                toggled: homeView.isShown()
                toggledColor: theme.iconBackgroundViewBarToggled
                imageWidth: 50
                imageHeight: 50
                source: "qrc:/gpt4all/icons/home.svg"
                Accessible.name: qsTr("Home view")
                Accessible.description: qsTr("Home view of application")
                onClicked: {
                    homeView.show()
                }
            }

            Text {
                Layout.topMargin: -20
                text: qsTr("Home")
                font.pixelSize: theme.fontSizeFixedSmall
                font.bold: true
                color: homeButton.hovered ? homeButton.backgroundColorHovered : homeButton.backgroundColor
                Layout.preferredWidth: 70
                horizontalAlignment: Text.AlignHCenter
                TapHandler {
                    onTapped: function(eventPoint, button) {
                        homeView.show()
                    }
                }
            }

            MyToolButton {
                id: chatButton
                backgroundColor: toggled ? theme.iconBackgroundViewBarHovered : theme.iconBackgroundViewBar
                backgroundColorHovered: theme.iconBackgroundViewBarHovered
                Layout.preferredWidth: 70
                Layout.preferredHeight: 70
                Layout.alignment: Qt.AlignCenter
                toggledWidth: 0
                toggled: chatView.isShown()
                toggledColor: theme.iconBackgroundViewBarToggled
                imageWidth: 50
                imageHeight: 50
                source: "qrc:/gpt4all/icons/chat.svg"
                Accessible.name: qsTr("Chat view")
                Accessible.description: qsTr("Chat view to interact with models")
                onClicked: {
                    chatView.show()
                }
            }

            Text {
                Layout.topMargin: -20
                text: qsTr("Chats")
                font.pixelSize: theme.fontSizeFixedSmall
                font.bold: true
                color: chatButton.hovered ? chatButton.backgroundColorHovered : chatButton.backgroundColor
                Layout.preferredWidth: 70
                horizontalAlignment: Text.AlignHCenter
                TapHandler {
                    onTapped: function(eventPoint, button) {
                        chatView.show()
                    }
                }
            }

            MyToolButton {
                id: searchButton
                backgroundColor: toggled ? theme.iconBackgroundViewBarHovered : theme.iconBackgroundViewBar
                backgroundColorHovered: theme.iconBackgroundViewBarHovered
                Layout.preferredWidth: 70
                Layout.preferredHeight: 70
                toggledWidth: 0
                toggled: downloadView.isShown()
                toggledColor: theme.iconBackgroundViewBarToggled
                imageWidth: 50
                imageHeight: 50
                source: "qrc:/gpt4all/icons/models.svg"
                Accessible.name: qsTr("Search")
                Accessible.description: qsTr("Models view to explore and download models")
                onClicked: {
                    downloadView.show()
                }
            }

            Text {
                Layout.topMargin: -20
                text: qsTr("Models")
                font.pixelSize: theme.fontSizeFixedSmall
                font.bold: true
                color: searchButton.hovered ? searchButton.backgroundColorHovered : searchButton.backgroundColor
                Layout.preferredWidth: 70
                horizontalAlignment: Text.AlignHCenter
                TapHandler {
                    onTapped: function(eventPoint, button) {
                        downloadView.show()
                    }
                }
            }

            MyToolButton {
                id: localdocsButton
                backgroundColor: toggled ? theme.iconBackgroundViewBarHovered : theme.iconBackgroundViewBar
                backgroundColorHovered: theme.iconBackgroundViewBarHovered
                Layout.preferredWidth: 70
                Layout.preferredHeight: 70
                toggledWidth: 0
                toggledColor: theme.iconBackgroundViewBarToggled
                toggled: localDocsView.isShown()
                imageWidth: 50
                imageHeight: 50
                source: "qrc:/gpt4all/icons/db.svg"
                Accessible.name: qsTr("LocalDocs")
                Accessible.description: qsTr("LocalDocs view to configure and use local docs")
                onClicked: {
                    localDocsView.show()
                }
            }

            Text {
                Layout.topMargin: -20
                text: qsTr("LocalDocs")
                font.pixelSize: theme.fontSizeFixedSmall
                font.bold: true
                color: localdocsButton.hovered ? localdocsButton.backgroundColorHovered : localdocsButton.backgroundColor
                Layout.preferredWidth: 70
                horizontalAlignment: Text.AlignHCenter
                TapHandler {
                    onTapped: function(eventPoint, button) {
                        localDocsView.show()
                    }
                }
            }

            MyToolButton {
                id: settingsButton
                backgroundColor: toggled ? theme.iconBackgroundViewBarHovered : theme.iconBackgroundViewBar
                backgroundColorHovered: theme.iconBackgroundViewBarHovered
                Layout.preferredWidth: 70
                Layout.preferredHeight: 70
                toggledWidth: 0
                toggledColor: theme.iconBackgroundViewBarToggled
                toggled: settingsView.isShown()
                imageWidth: 50
                imageHeight: 50
                source: "qrc:/gpt4all/icons/settings.svg"
                Accessible.name: qsTr("Settings")
                Accessible.description: qsTr("Settings view for application configuration")
                onClicked: {
                    settingsView.show(0 /*pageToDisplay*/)
                }
            }

            Text {
                Layout.topMargin: -20
                text: qsTr("Settings")
                font.pixelSize: theme.fontSizeFixedSmall
                font.bold: true
                color: settingsButton.hovered ? settingsButton.backgroundColorHovered : settingsButton.backgroundColor
                Layout.preferredWidth: 70
                horizontalAlignment: Text.AlignHCenter
                TapHandler {
                    onTapped: function(eventPoint, button) {
                        settingsView.show(0 /*pageToDisplay*/)
                    }
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
            spacing: 22

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                Layout.preferredWidth: image.width
                Layout.preferredHeight: image.height
                color: "transparent"

                Image {
                    id: image
                    anchors.centerIn: parent
                    sourceSize.width: 80
                    sourceSize.height: 55
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
                    visible: false
                    source: "qrc:/gpt4all/icons/nomic_logo.svg"
                }

                ColorOverlay {
                    anchors.fill: image
                    source: image
                    color: image.hovered ? theme.logoColorHovered : theme.logoColor
                    TapHandler {
                        onTapped: function(eventPoint, button) {
                            Qt.openUrlExternally("https://nomic.ai")
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: roundedFrame
        z: 299
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: viewBar.right
        anchors.right: parent.right
        anchors.topMargin: 15
        anchors.bottomMargin: 15
        anchors.rightMargin: 15
        radius: 15
        border.width: 2
        border.color: theme.dividerColor
        color: "transparent"
        clip: true
    }

    RectangularGlow {
        id: effect
        anchors.fill: roundedFrame
        glowRadius: 15
        spread: 0
        color: theme.dividerColor
        cornerRadius: 10
        opacity: 0.5
    }

    StackLayout {
        id: stackLayout
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: viewBar.right
        anchors.right: parent.right
        anchors.topMargin: 15
        anchors.bottomMargin: 15
        anchors.rightMargin: 15

        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: Rectangle {
                width: roundedFrame.width
                height: roundedFrame.height
                radius: 15
            }
        }

        HomeView {
            id: homeView
            Layout.fillWidth: true
            Layout.fillHeight: true
            shouldShowFirstStart: !hasCheckedFirstStart

            function show() {
                stackLayout.currentIndex = 0;
            }

            function isShown() {
                return stackLayout.currentIndex === 0
            }

            Connections {
                target: homeView
                function onChatViewRequested() {
                    chatView.show();
                }
                function onLocalDocsViewRequested() {
                    localDocsView.show();
                }
                function onDownloadViewRequested(showEmbeddingModels) {
                    downloadView.show(showEmbeddingModels);
                }
                function onSettingsViewRequested(page) {
                    settingsView.show(page);
                }
            }
        }

        ChatView {
            id: chatView
            Layout.fillWidth: true
            Layout.fillHeight: true

            function show() {
                stackLayout.currentIndex = 1;
            }

            function isShown() {
                return stackLayout.currentIndex === 1
            }

            Connections {
                target: chatView
                function onDownloadViewRequested(showEmbeddingModels) {
                    downloadView.show(showEmbeddingModels);
                }
                function onSettingsViewRequested(page) {
                    settingsView.show(page);
                }
            }
        }

        ModelDownloaderView {
            id: downloadView
            Layout.fillWidth: true
            Layout.fillHeight: true

            function show(showEmbeddingModels) {
                stackLayout.currentIndex = 2;
                if (showEmbeddingModels)
                    downloadView.showEmbeddingModels();
            }

            function isShown() {
                return stackLayout.currentIndex === 2
            }

            Item {
                Accessible.name: qsTr("Download new models")
                Accessible.description: qsTr("View for downloading new models")
            }
        }

        LocalDocsView {
            id: localDocsView
            Layout.fillWidth: true
            Layout.fillHeight: true

            function show() {
                stackLayout.currentIndex = 3;
            }

            function isShown() {
                return stackLayout.currentIndex === 3
            }
        }

        SettingsView {
            id: settingsView
            Layout.fillWidth: true
            Layout.fillHeight: true

            function show(page) {
                settingsView.pageToDisplay = page;
                stackLayout.currentIndex = 4;
            }

            function isShown() {
                return stackLayout.currentIndex === 4
            }

            onDownloadClicked: {
                downloadView.show(true /*showEmbeddingModels*/)
            }
        }
    }
}
