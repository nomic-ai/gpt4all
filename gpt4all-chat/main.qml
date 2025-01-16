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
import Qt.labs.platform

Window {
    id: window
    width: 1440
    height: 810
    minimumWidth: 658 + 470 * theme.fontScale
    minimumHeight: 384 + 160 * theme.fontScale
    visible: true
    title: qsTr("GPT4All v%1").arg(Qt.application.version)

    SystemTrayIcon {
        id: systemTrayIcon
        property bool shouldClose: false
        visible: MySettings.systemTray && !shouldClose
        icon.source: "qrc:/gpt4all/icons/gpt4all.svg"

        function restore() {
            LLM.showDockIcon();
            window.show();
            window.raise();
            window.requestActivate();
        }
        onActivated: function(reason) {
            if (reason === SystemTrayIcon.Context && Qt.platform.os !== "osx")
                menu.open();
            else if (reason === SystemTrayIcon.Trigger)
                restore();
        }

        menu: Menu {
            MenuItem {
                text: qsTr("Restore")
                onTriggered: systemTrayIcon.restore()
            }
            MenuItem {
                text: qsTr("Quit")
                onTriggered: {
                    systemTrayIcon.restore();
                    systemTrayIcon.shouldClose = true;
                    window.shouldClose = true;
                    savingPopup.open();
                    ChatListModel.saveChatsForQuit();
                }
            }
        }
    }

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

    property bool hasCheckedFirstStart: false
    property bool hasShownSettingsAccess: false
    property var currentChat: ChatListModel.currentChat

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
        text: qsTr("<h3>Encountered an error starting up:</h3><br>"
              + "<i>\"Incompatible hardware detected.\"</i>"
              + "<br><br>Unfortunately, your CPU does not meet the minimal requirements to run "
              + "this program. In particular, it does not support AVX intrinsics which this "
              + "program requires to successfully run a modern large language model. "
              + "The only solution at this time is to upgrade your hardware to a more modern CPU."
              + "<br><br>See here for more information: <a href=\"https://en.wikipedia.org/wiki/Advanced_Vector_Extensions\">"
              + "https://en.wikipedia.org/wiki/Advanced_Vector_Extensions</a>");
    }

    PopupDialog {
        id: errorSettingsAccess
        anchors.centerIn: parent
        shouldTimeOut: false
        shouldShowBusy: false
        modal: true
        text: qsTr("<h3>Encountered an error starting up:</h3><br>"
              + "<i>\"Inability to access settings file.\"</i>"
              + "<br><br>Unfortunately, something is preventing the program from accessing "
              + "the settings file. This could be caused by incorrect permissions in the local "
              + "app config directory where the settings file is located. "
              + "Check out our <a href=\"https://discord.gg/4M2QFmTt2k\">discord channel</a> for help.")
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

    property bool shouldClose: false

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

    onClosing: function(close) {
        if (systemTrayIcon.visible) {
            LLM.hideDockIcon();
            window.visible = false;
            ChatListModel.saveChats();
            close.accepted = false;
            return;
        }

        if (window.shouldClose)
            return;

        window.shouldClose = true;
        savingPopup.open();
        ChatListModel.saveChatsForQuit();
        close.accepted = false;
    }

    Connections {
        target: ChatListModel
        function onSaveChatsFinished() {
            savingPopup.close();
            if (window.shouldClose)
                window.close()
        }
    }

    color: theme.viewBarBackground

    Rectangle {
        id: viewBar
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 68 * theme.fontScale
        color: theme.viewBarBackground

        ColumnLayout {
            id: viewsLayout
            anchors.top: parent.top
            anchors.topMargin: 30
            anchors.horizontalCenter: parent.horizontalCenter
            Layout.margins: 0
            spacing: 16

            MyToolButton {
                id: homeButton
                backgroundColor: toggled ? theme.iconBackgroundViewBarHovered : theme.iconBackgroundViewBar
                backgroundColorHovered: theme.iconBackgroundViewBarHovered
                Layout.preferredWidth: 38 * theme.fontScale
                Layout.preferredHeight: 38 * theme.fontScale
                Layout.alignment: Qt.AlignCenter
                toggledWidth: 0
                toggled: homeView.isShown()
                toggledColor: theme.iconBackgroundViewBarToggled
                imageWidth: 25 * theme.fontScale
                imageHeight: 25 * theme.fontScale
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
                font.pixelSize: theme.fontSizeMedium
                font.bold: true
                color: homeButton.hovered ? homeButton.backgroundColorHovered : homeButton.backgroundColor
                Layout.preferredWidth: 38 * theme.fontScale
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
                Layout.preferredWidth: 38 * theme.fontScale
                Layout.preferredHeight: 38 * theme.fontScale
                Layout.alignment: Qt.AlignCenter
                toggledWidth: 0
                toggled: chatView.isShown()
                toggledColor: theme.iconBackgroundViewBarToggled
                imageWidth: 25 * theme.fontScale
                imageHeight: 25 * theme.fontScale
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
                font.pixelSize: theme.fontSizeMedium
                font.bold: true
                color: chatButton.hovered ? chatButton.backgroundColorHovered : chatButton.backgroundColor
                Layout.preferredWidth: 38 * theme.fontScale
                horizontalAlignment: Text.AlignHCenter
                TapHandler {
                    onTapped: function(eventPoint, button) {
                        chatView.show()
                    }
                }
            }

            MyToolButton {
                id: modelsButton
                backgroundColor: toggled ? theme.iconBackgroundViewBarHovered : theme.iconBackgroundViewBar
                backgroundColorHovered: theme.iconBackgroundViewBarHovered
                Layout.preferredWidth: 38 * theme.fontScale
                Layout.preferredHeight: 38 * theme.fontScale
                toggledWidth: 0
                toggled: modelsView.isShown()
                toggledColor: theme.iconBackgroundViewBarToggled
                imageWidth: 25 * theme.fontScale
                imageHeight: 25 * theme.fontScale
                source: "qrc:/gpt4all/icons/models.svg"
                Accessible.name: qsTr("Models")
                Accessible.description: qsTr("Models view for installed models")
                onClicked: {
                    modelsView.show()
                }
            }

            Text {
                Layout.topMargin: -20
                text: qsTr("Models")
                font.pixelSize: theme.fontSizeMedium
                font.bold: true
                color: modelsButton.hovered ? modelsButton.backgroundColorHovered : modelsButton.backgroundColor
                Layout.preferredWidth: 38 * theme.fontScale
                horizontalAlignment: Text.AlignHCenter
                TapHandler {
                    onTapped: function(eventPoint, button) {
                        modelsView.show()
                    }
                }
            }

            MyToolButton {
                id: localdocsButton
                backgroundColor: toggled ? theme.iconBackgroundViewBarHovered : theme.iconBackgroundViewBar
                backgroundColorHovered: theme.iconBackgroundViewBarHovered
                Layout.preferredWidth: 38 * theme.fontScale
                Layout.preferredHeight: 38 * theme.fontScale
                toggledWidth: 0
                toggledColor: theme.iconBackgroundViewBarToggled
                toggled: localDocsView.isShown()
                imageWidth: 25 * theme.fontScale
                imageHeight: 25 * theme.fontScale
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
                font.pixelSize: theme.fontSizeMedium
                font.bold: true
                color: localdocsButton.hovered ? localdocsButton.backgroundColorHovered : localdocsButton.backgroundColor
                Layout.preferredWidth: 38 * theme.fontScale
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
                Layout.preferredWidth: 38 * theme.fontScale
                Layout.preferredHeight: 38 * theme.fontScale
                toggledWidth: 0
                toggledColor: theme.iconBackgroundViewBarToggled
                toggled: settingsView.isShown()
                imageWidth: 25 * theme.fontScale
                imageHeight: 25 * theme.fontScale
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
                font.pixelSize: theme.fontSizeMedium
                font.bold: true
                color: settingsButton.hovered ? settingsButton.backgroundColorHovered : settingsButton.backgroundColor
                Layout.preferredWidth: 38 * theme.fontScale
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

            Item {
                id: antennaItem
                Layout.alignment: Qt.AlignCenter
                Layout.preferredWidth: antennaImage.width
                Layout.preferredHeight: antennaImage.height
                Image {
                    id: antennaImage
                    sourceSize.width: 32
                    sourceSize.height: 32
                    visible: false
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/gpt4all/icons/antenna_3.svg"
                }

                ColorOverlay {
                    id: antennaColored
                    visible: ModelList.selectableModels.count !== 0 && (currentChat.isServer || currentChat.modelInfo.isOnline || MySettings.networkIsActive)
                    anchors.fill: antennaImage
                    source: antennaImage
                    color: theme.styledTextColor
                    ToolTip.text: {
                        if (MySettings.networkIsActive)
                            return qsTr("The datalake is enabled")
                        else if (currentChat.modelInfo.isOnline)
                            return qsTr("Using a network model")
                        else if (currentChat.isServer)
                            return qsTr("Server mode is enabled")
                        return ""
                    }
                    ToolTip.visible: maAntenna.containsMouse
                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    MouseArea {
                        id: maAntenna
                        anchors.fill: antennaColored
                        hoverEnabled: true
                    }
                }

                SequentialAnimation {
                    running: true
                    loops: Animation.Infinite

                    PropertyAnimation {
                        target: antennaImage
                        property: "source"
                        duration: 500
                        from: "qrc:/gpt4all/icons/antenna_1.svg"
                        to: "qrc:/gpt4all/icons/antenna_2.svg"
                    }

                    PauseAnimation {
                        duration: 1500
                    }

                    PropertyAnimation {
                        target: antennaImage
                        property: "source"
                        duration: 500
                        from: "qrc:/gpt4all/icons/antenna_2.svg"
                        to: "qrc:/gpt4all/icons/antenna_3.svg"
                    }

                    PauseAnimation {
                        duration: 1500
                    }

                    PropertyAnimation {
                        target: antennaImage
                        property: "source"
                        duration: 500
                        from: "qrc:/gpt4all/icons/antenna_3.svg"
                        to: "qrc:/gpt4all/icons/antenna_2.svg"
                    }

                    PauseAnimation {
                        duration: 1500
                    }

                    PropertyAnimation {
                        target: antennaImage
                        property: "source"
                        duration: 1500
                        from: "qrc:/gpt4all/icons/antenna_2.svg"
                        to: "qrc:/gpt4all/icons/antenna_1.svg"
                    }

                    PauseAnimation {
                        duration: 500
                    }
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                Layout.preferredWidth: image.width
                Layout.preferredHeight: image.height
                color: "transparent"

                Image {
                    id: image
                    anchors.centerIn: parent
                    sourceSize: Qt.size(48 * theme.fontScale, 32 * theme.fontScale)
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
                    visible: false
                    source: "qrc:/gpt4all/icons/nomic_logo.svg"
                }

                ColorOverlay {
                    anchors.fill: image
                    source: image
                    color: image.hovered ? theme.mutedDarkTextColorHovered : theme.mutedDarkTextColor
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
        border.width: 1
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
                function onAddModelViewRequested() {
                    addModelView.show();
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
                function onAddCollectionViewRequested() {
                    addCollectionView.show();
                }
                function onAddModelViewRequested() {
                    addModelView.show();
                }
            }
        }

        ModelsView {
            id: modelsView
            Layout.fillWidth: true
            Layout.fillHeight: true

            function show() {
                stackLayout.currentIndex = 2;
            }

            function isShown() {
                return stackLayout.currentIndex === 2
            }

            Item {
                Accessible.name: qsTr("Installed models")
                Accessible.description: qsTr("View of installed models")
            }

            Connections {
                target: modelsView
                function onAddModelViewRequested() {
                    addModelView.show();
                }
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

            Connections {
                target: localDocsView
                function onAddCollectionViewRequested() {
                    addCollectionView.show();
                }
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
        }

        AddCollectionView {
            id: addCollectionView
            Layout.fillWidth: true
            Layout.fillHeight: true

            function show() {
                stackLayout.currentIndex = 5;
            }
            function isShown() {
                return stackLayout.currentIndex === 5
            }

            Connections {
                target: addCollectionView
                function onLocalDocsViewRequested() {
                    localDocsView.show();
                }
            }
        }

        AddModelView {
            id: addModelView
            Layout.fillWidth: true
            Layout.fillHeight: true

            function show() {
                stackLayout.currentIndex = 6;
            }
            function isShown() {
                return stackLayout.currentIndex === 6
            }

            Connections {
                target: addModelView
                function onModelsViewRequested() {
                    modelsView.show();
                }
            }
        }
    }
}
