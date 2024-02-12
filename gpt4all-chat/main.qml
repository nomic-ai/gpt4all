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

    Theme {
        id: theme
    }

    property var currentChat: ChatListModel.currentChat
    property var chatModel: currentChat.chatModel
    property bool hasSaved: false

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

    // Startup code
    Component.onCompleted: {
        startupDialogs();
    }

    Connections {
        target: firstStartDialog
        function onClosed() {
            startupDialogs();
        }
    }

    Connections {
        target: downloadNewModels
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

    Connections {
        target: currentChat
        function onResponseInProgressChanged() {
            if (MySettings.networkIsActive && !currentChat.responseInProgress)
                Network.sendConversation(currentChat.id, getConversationJson());
        }
        function onModelLoadingErrorChanged() {
            if (currentChat.modelLoadingError !== "")
                modelLoadingErrorPopup.open()
        }
    }

    property bool hasShownModelDownload: false
    property bool hasShownFirstStart: false
    property bool hasShownSettingsAccess: false

    function startupDialogs() {
        if (!LLM.compatHardware()) {
            Network.sendNonCompatHardware();
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
        if (!hasShownFirstStart && Download.isFirstStart()) {
            firstStartDialog.open();
            hasShownFirstStart = true;
            return;
        }

        // check for any current models and if not, open download dialog once
        if (!hasShownModelDownload && ModelList.installedModels.count === 0 && !firstStartDialog.opened) {
            downloadNewModels.open();
            hasShownModelDownload = true;
            return;
        }

        // check for new version
        if (Download.hasNewerRelease && !firstStartDialog.opened && !downloadNewModels.opened) {
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

    AboutDialog {
        id: aboutDialog
        anchors.centerIn: parent
        width: Math.min(1024, window.width - (window.width * .2))
        height: Math.min(600, window.height - (window.height * .2))
    }

    Item {
        Accessible.role: Accessible.Window
        Accessible.name: title
    }

    PopupDialog {
        id: modelLoadingErrorPopup
        anchors.centerIn: parent
        shouldTimeOut: false
        text: qsTr("<h3>Encountered an error loading model:</h3><br>")
            + "<i>\"" + currentChat.modelLoadingError + "\"</i>"
            + qsTr("<br><br>Model loading failures can happen for a variety of reasons, but the most common "
            + "causes include a bad file format, an incomplete or corrupted download, the wrong file "
            + "type, not enough system RAM or an incompatible model type. Here are some suggestions for resolving the problem:"
            + "<br><ul>"
            + "<li>Ensure the model file has a compatible format and type"
            + "<li>Check the model file is complete in the download folder"
            + "<li>You can find the download folder in the settings dialog"
            + "<li>If you've sideloaded the model ensure the file is not corrupt by checking md5sum"
            + "<li>Read more about what models are supported in our <a href=\"https://docs.gpt4all.io/gpt4all_chat.html\">documentation</a> for the gui"
            + "<li>Check out our <a href=\"https://discord.gg/4M2QFmTt2k\">discord channel</a> for help")
    }

    Rectangle {
        id: accentRibbon
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        height: 3
        color: theme.accentColor
    }

    Rectangle {
        id: titleBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 5
        z: 200
        height: 25
        color: "transparent"

        RowLayout {
            anchors.left: parent.left
            anchors.leftMargin: 30

            Text {
                textFormat: Text.StyledText
                text: "<a href=\"https://gpt4all.io\">gpt4all.io</a> |"
                horizontalAlignment: Text.AlignLeft
                font.pixelSize: theme.fontSizeFixedSmall
                color: theme.iconBackgroundLight
                linkColor: hoverHandler1.hovered ? theme.iconBackgroundHovered : theme.iconBackgroundLight
                HoverHandler { id: hoverHandler1 }
                onLinkActivated: { Qt.openUrlExternally("https://gpt4all.io") }
            }

            Text {
                textFormat: Text.StyledText
                text: "<a href=\"https://github.com/nomic-ai/gpt4all\">github</a>"
                horizontalAlignment: Text.AlignLeft
                font.pixelSize: theme.fontSizeFixedSmall
                color: theme.iconBackgroundLight
                linkColor: hoverHandler2.hovered ? theme.iconBackgroundHovered : theme.iconBackgroundLight
                HoverHandler { id: hoverHandler2 }
                onLinkActivated: { Qt.openUrlExternally("https://github.com/nomic-ai/gpt4all") }
            }
        }

        RowLayout {
            anchors.right: parent.right
            anchors.rightMargin: 30

            Text {
                textFormat: Text.StyledText
                text: "<a href=\"https://nomic.ai\">nomic.ai</a> |"
                horizontalAlignment: Text.AlignRight
                font.pixelSize: theme.fontSizeFixedSmall
                color: theme.iconBackgroundLight
                linkColor: hoverHandler3.hovered ? theme.iconBackgroundHovered : theme.iconBackgroundLight
                HoverHandler { id: hoverHandler3 }
                onLinkActivated: { Qt.openUrlExternally("https://nomic.ai") }
            }

            Text {
                textFormat: Text.StyledText
                text: "<a href=\"https://twitter.com/nomic_ai\">twitter</a> |"
                horizontalAlignment: Text.AlignRight
                font.pixelSize: theme.fontSizeFixedSmall
                color: theme.iconBackgroundLight
                linkColor: hoverHandler4.hovered ? theme.iconBackgroundHovered : theme.iconBackgroundLight
                HoverHandler { id: hoverHandler4 }
                onLinkActivated: { Qt.openUrlExternally("https://twitter.com/nomic_ai") }
            }

            Text {
                textFormat: Text.StyledText
                text: "<a href=\"https://discord.gg/4M2QFmTt2k\">discord</a>"
                horizontalAlignment: Text.AlignRight
                font.pixelSize: theme.fontSizeFixedSmall
                color: theme.iconBackgroundLight
                linkColor: hoverHandler5.hovered ? theme.iconBackgroundHovered : theme.iconBackgroundLight
                HoverHandler { id: hoverHandler5 }
                onLinkActivated: { Qt.openUrlExternally("https://discord.gg/4M2QFmTt2k") }
            }
        }
    }

    Rectangle {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 100
        color: theme.mainHeader
        Item {
            anchors.centerIn: parent
            height: childrenRect.height
            visible: currentChat.isModelLoaded || currentChat.modelLoadingError !== "" || currentChat.isServer

            Label {
                id: modelLabel
                color: theme.textColor
                padding: 20
                font.pixelSize: theme.fontSizeLarger
                text: ""
                background: Rectangle {
                    color: theme.mainHeader
                }
                horizontalAlignment: TextInput.AlignRight
            }

            MyComboBox {
                id: comboBox
                implicitWidth: 375
                width: window.width >= 750 ? implicitWidth : implicitWidth - ((750 - window.width))
                anchors.top: modelLabel.top
                anchors.bottom: modelLabel.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.horizontalCenterOffset: window.width >= 950 ? 0 : Math.max(-((950 - window.width) / 2), -99.5)
                enabled: !currentChat.isServer
                model: ModelList.installedModels
                valueRole: "id"
                textRole: "name"
                property string currentModelName: ""
                function updateCurrentModelName() {
                    var info = ModelList.modelInfo(currentChat.modelInfo.id);
                    comboBox.currentModelName = info.name;
                }
                Connections {
                    target: currentChat
                    function onModelInfoChanged() {
                        comboBox.updateCurrentModelName();
                    }
                }
                Connections {
                    target: window
                    function onCurrentChatChanged() {
                        comboBox.updateCurrentModelName();
                    }
                }
                background: Rectangle {
                    color: theme.mainComboBackground
                    radius: 10
                }
                contentItem: Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    leftPadding: 10
                    rightPadding: 20
                    text: currentChat.modelLoadingError !== ""
                        ? qsTr("Model loading error...")
                        : comboBox.currentModelName
                    font.pixelSize: theme.fontSizeLarger
                    color: theme.white
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }
                delegate: ItemDelegate {
                    width: comboBox.width
                    contentItem: Text {
                        text: name
                        color: theme.textColor
                        font: comboBox.font
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: (index % 2 === 0 ? theme.darkContrast : theme.lightContrast)
                        border.width: highlighted
                        border.color: theme.accentColor
                    }
                    highlighted: comboBox.highlightedIndex === index
                }
                Accessible.role: Accessible.ComboBox
                Accessible.name: comboBox.currentModelName
                Accessible.description: qsTr("The top item is the current model")
                onActivated: function (index) {
                    currentChat.stopGenerating()
                    currentChat.reset();
                    currentChat.modelInfo = ModelList.modelInfo(comboBox.valueAt(index))
                }
            }
        }

        Item {
            anchors.centerIn: parent
            visible: ModelList.installedModels.count
                && !currentChat.isModelLoaded
                && currentChat.modelLoadingError === ""
                && !currentChat.isServer
            width: childrenRect.width
            height: childrenRect.height
            Row {
                spacing: 5
                MyBusyIndicator {
                    anchors.verticalCenter: parent.verticalCenter
                    running: parent.visible
                    Accessible.role: Accessible.Animation
                    Accessible.name: qsTr("Busy indicator")
                    Accessible.description: qsTr("loading model...")
                }

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Loading model...")
                    font.pixelSize: theme.fontSizeLarge
                    color: theme.oppositeTextColor
                }
            }
        }
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

    Button {
        id: drawerButton
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 42.5
        anchors.leftMargin: 30
        width: 40
        height: 40
        z: 200
        padding: 15

        Accessible.role: Accessible.ButtonMenu
        Accessible.name: qsTr("Main menu")
        Accessible.description: qsTr("Navigation drawer with options")

        background: Item {
            anchors.centerIn: parent
            width: 30
            height: 30

            Rectangle {
                id: bar1
                color: drawerButton.hovered ? theme.iconBackgroundHovered : theme.iconBackgroundLight
                width: parent.width
                height: 6
                radius: 2
                antialiasing: true
            }

            Rectangle {
                id: bar2
                anchors.centerIn: parent
                color: drawerButton.hovered ? theme.iconBackgroundHovered : theme.iconBackgroundLight
                width: parent.width
                height: 6
                radius: 2
                antialiasing: true
            }

            Rectangle {
                id: bar3
                anchors.bottom: parent.bottom
                color: drawerButton.hovered ? theme.iconBackgroundHovered : theme.iconBackgroundLight
                width: parent.width
                height: 6
                radius: 2
                antialiasing: true
            }
        }
        onClicked: {
            drawer.visible = !drawer.visible
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

    MyToolButton {
        id: networkButton
        backgroundColor: theme.iconBackgroundLight
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 42.5
        anchors.rightMargin: 30
        width: 40
        height: 40
        z: 200
        padding: 15
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

    Connections {
        target: Network
        function onHealthCheckFailed(code) {
            healthCheckFailed.open();
        }
    }

    CollectionsDialog {
        id: collectionsDialog
        anchors.centerIn: parent
        onAddRemoveClicked: {
            settingsDialog.pageToDisplay = 2;
            settingsDialog.open();
        }
    }

    MyToolButton {
        id: collectionsButton
        backgroundColor: theme.iconBackgroundLight
        anchors.right: networkButton.left
        anchors.top: parent.top
        anchors.topMargin: 42.5
        anchors.rightMargin: 10
        width: 40
        height: 42.5
        z: 200
        padding: 15
        toggled: currentChat.collectionList.length
        source: "qrc:/gpt4all/icons/db.svg"
        Accessible.name: qsTr("Add documents")
        Accessible.description: qsTr("add collections of documents to the chat")

        onClicked: {
            collectionsDialog.open()
        }
    }

    MyToolButton {
        id: settingsButton
        backgroundColor: theme.iconBackgroundLight
        anchors.right: collectionsButton.left
        anchors.top: parent.top
        anchors.topMargin: 42.5
        anchors.rightMargin: 10
        width: 40
        height: 40
        z: 200
        padding: 15
        source: "qrc:/gpt4all/icons/settings.svg"
        Accessible.name: qsTr("Settings")
        Accessible.description: qsTr("Reveals a dialogue with settings")

        onClicked: {
            settingsDialog.open()
        }
    }

    PopupDialog {
        id: copyMessage
        anchors.centerIn: parent
        text: qsTr("Conversation copied to clipboard.")
        font.pixelSize: theme.fontSizeLarge
    }

    PopupDialog {
        id: copyCodeMessage
        anchors.centerIn: parent
        text: qsTr("Code copied to clipboard.")
        font.pixelSize: theme.fontSizeLarge
    }

    PopupDialog {
        id: healthCheckFailed
        anchors.centerIn: parent
        text: qsTr("Connection to datalake failed.")
        font.pixelSize: theme.fontSizeLarge
    }

    PopupDialog {
        id: recalcPopup
        anchors.centerIn: parent
        shouldTimeOut: false
        shouldShowBusy: true
        text: qsTr("Recalculating context.")
        font.pixelSize: theme.fontSizeLarge

        Connections {
            target: currentChat
            function onRecalcChanged() {
                if (currentChat.isRecalc)
                    recalcPopup.open()
                else
                    recalcPopup.close()
            }
        }
    }

    PopupDialog {
        id: savingPopup
        anchors.centerIn: parent
        shouldTimeOut: false
        shouldShowBusy: true
        text: qsTr("Saving chats.")
        font.pixelSize: theme.fontSizeLarge
    }

    MyToolButton {
        id: copyButton
        backgroundColor: theme.iconBackgroundLight
        anchors.right: settingsButton.left
        anchors.top: parent.top
        anchors.topMargin: 42.5
        anchors.rightMargin: 10
        width: 40
        height: 40
        z: 200
        padding: 15
        source: "qrc:/gpt4all/icons/copy.svg"
        Accessible.name: qsTr("Copy")
        Accessible.description: qsTr("Copy the conversation to the clipboard")

        TextEdit{
            id: copyEdit
            visible: false
        }

        onClicked: {
            var conversation = getConversation()
            copyEdit.text = conversation
            copyEdit.selectAll()
            copyEdit.copy()
            copyMessage.open()
        }
    }

    function getConversation() {
        var conversation = "";
        for (var i = 0; i < chatModel.count; i++) {
            var item = chatModel.get(i)
            var string = item.name;
            var isResponse = item.name === qsTr("Response: ")
            string += chatModel.get(i).value
            if (isResponse && item.stopped)
                string += " <stopped>"
            string += "\n"
            conversation += string
        }
        return conversation
    }

    function getConversationJson() {
        var str = "{\"conversation\": [";
        for (var i = 0; i < chatModel.count; i++) {
            var item = chatModel.get(i)
            var isResponse = item.name === qsTr("Response: ")
            str += "{\"content\": ";
            str += JSON.stringify(item.value)
            str += ", \"role\": \"" + (isResponse ? "assistant" : "user") + "\"";
            if (isResponse && item.thumbsUpState !== item.thumbsDownState)
                str += ", \"rating\": \"" + (item.thumbsUpState ? "positive" : "negative") + "\"";
            if (isResponse && item.newResponse !== "")
                str += ", \"edited_content\": " + JSON.stringify(item.newResponse);
            if (isResponse && item.stopped)
                str += ", \"stopped\": \"true\""
            if (!isResponse)
                str += "},"
            else
                str += ((i < chatModel.count - 1) ? "}," : "}")
        }
        return str + "]}"
    }

    MyToolButton {
        id: resetContextButton
        backgroundColor: theme.iconBackgroundLight
        anchors.right: copyButton.left
        anchors.top: parent.top
        anchors.topMargin: 42.5
        anchors.rightMargin: 10
        width: 40
        height: 40
        z: 200
        padding: 15
        source: "qrc:/gpt4all/icons/regenerate.svg"

        Accessible.name: text
        Accessible.description: qsTr("Reset the context and erase current conversation")

        onClicked: {
            Network.sendResetContext(chatModel.count)
            currentChat.reset();
            currentChat.processSystemPrompt();
        }
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

    ChatDrawer {
        id: drawer
        y: header.height + accentRibbon.height
        width: Math.min(600, 0.3 * window.width)
        height: window.height - y
        onDownloadClicked: {
            downloadNewModels.showEmbeddingModels = false
            downloadNewModels.open()
        }
        onAboutClicked: {
            aboutDialog.open()
        }
    }

    PopupDialog {
        id: referenceContextDialog
        anchors.centerIn: parent
        shouldTimeOut: false
        shouldShowBusy: false
        modal: true
    }

    Rectangle {
        id: conversation
        color: theme.conversationBackground
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.top: accentRibbon.bottom

        ScrollView {
            id: scrollView
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: !currentChat.isServer ? textInputView.top : parent.bottom
            anchors.bottomMargin: !currentChat.isServer ? 30 : 0
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff

            Rectangle {
                anchors.fill: parent
                color: currentChat.isServer ? theme.black : theme.conversationBackground

                Rectangle {
                    id: homePage
                    color: "transparent"//theme.green200
                    anchors.fill: parent
                    visible: (ModelList.installedModels.count === 0 || chatModel.count === 0) && !currentChat.isServer

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 0

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("GPT4All")
                            color: theme.titleTextColor
                            font.pixelSize: theme.fontSizeLargest + 15
                            font.bold: true
                            horizontalAlignment: Qt.AlignHCenter
                            wrapMode: Text.WordWrap
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            textFormat: Text.StyledText
                            text: qsTr(
                            "<ul>
                            <li>Run privacy-aware local chatbots.
                            <li>No internet required to use.
                            <li>CPU and GPU acceleration.
                            <li>Chat with your local data and documents.
                            <li>Built by Nomic AI and forever open-source.
                            </ul>
                            ")
                            color: theme.textColor
                            font.pixelSize: theme.fontSizeLarge
                            wrapMode: Text.WordWrap
                        }

                        RowLayout {
                            spacing: 10
                            Layout.alignment: Qt.AlignHCenter
                            Layout.topMargin: 30
                            MySlug {
                               text: "MISTRAL"
                               color: theme.red600
                            }
                            MySlug {
                               text: "FALCON"
                               color: theme.green600
                            }
                            MySlug {
                               text: "LLAMA"
                               color: theme.purple500
                            }
                            MySlug {
                               text: "LLAMA2"
                               color: theme.red400
                            }
                            MySlug {
                               text: "MPT"
                               color: theme.green700
                            }
                            MySlug {
                               text: "REPLIT"
                               color: theme.yellow700
                            }
                            MySlug {
                               text: "STARCODER"
                               color: theme.purple400
                            }
                            MySlug {
                               text: "SBERT"
                               color: theme.yellow600
                            }
                            MySlug {
                               text: "GPT-J"
                               color: theme.gray600
                            }
                        }

                        MyButton {
                            id: downloadButton
                            visible: LLM.isNetworkOnline
                            Layout.alignment: Qt.AlignHCenter
                            Layout.topMargin: 40
                            text: qsTr("Download models")
                            fontPixelSize: theme.fontSizeLargest + 10
                            padding: 18
                            leftPadding: 50
                            Image {
                                id: image
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 15
                                width: 24
                                height: 24
                                mipmap: true
                                source: "qrc:/gpt4all/icons/download.svg"
                            }
                            ColorOverlay {
                                anchors.fill: image
                                source: image
                                color: theme.accentColor
                            }
                            onClicked: {
                                downloadNewModels.open();
                            }
                        }
                    }
                }

                ListView {
                    id: listView
                    visible: ModelList.installedModels.count !== 0 && chatModel.count !== 0
                    anchors.fill: parent
                    model: chatModel

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }

                    Accessible.role: Accessible.List
                    Accessible.name: qsTr("Conversation with the model")
                    Accessible.description: qsTr("prompt / response pairs from the conversation")

                    delegate: TextArea {
                        id: myTextArea
                        text: value + references
                        width: listView.width
                        color: theme.textColor
                        wrapMode: Text.WordWrap
                        textFormat: TextEdit.PlainText
                        focus: false
                        readOnly: true
                        font.pixelSize: theme.fontSizeLarge
                        cursorVisible: currentResponse ? currentChat.responseInProgress : false
                        cursorPosition: text.length
                        background: Rectangle {
                            opacity: 1.0
                            color: name === qsTr("Response: ")
                                ? (currentChat.isServer ? theme.black : theme.lightContrast)
                                : (currentChat.isServer ? theme.white : theme.darkContrast)
                        }
                        TapHandler {
                            id: tapHandler
                            onTapped: function(eventPoint, button) {
                                var clickedPos = myTextArea.positionAt(eventPoint.position.x, eventPoint.position.y);
                                var link = responseText.getLinkAtPosition(clickedPos);
                                if (link.startsWith("context://")) {
                                    var integer = parseInt(link.split("://")[1]);
                                    referenceContextDialog.text = referencesContext[integer - 1];
                                    referenceContextDialog.open();
                                } else {
                                    var success = responseText.tryCopyAtPosition(clickedPos);
                                    if (success)
                                        copyCodeMessage.open();
                                }
                            }
                        }

                        ResponseText {
                            id: responseText
                        }

                        Component.onCompleted: {
                            responseText.setLinkColor(theme.linkColor);
                            responseText.setHeaderColor(name === qsTr("Response: ") ? theme.darkContrast : theme.lightContrast);
                            responseText.textDocument = textDocument
                        }

                        Accessible.role: Accessible.Paragraph
                        Accessible.name: text
                        Accessible.description: name === qsTr("Response: ") ? "The response by the model" : "The prompt by the user"

                        topPadding: 20
                        bottomPadding: 20
                        leftPadding: 70
                        rightPadding: 100

                        Item {
                            anchors.left: parent.left
                            anchors.leftMargin: 60
                            y: parent.topPadding + (parent.positionToRectangle(0).height / 2) - (height / 2)
                            visible: (currentResponse ? true : false) && value === "" && currentChat.responseInProgress
                            width: childrenRect.width
                            height: childrenRect.height
                            Row {
                                spacing: 5
                                MyBusyIndicator {
                                    anchors.verticalCenter: parent.verticalCenter
                                    running: (currentResponse ? true : false) && value === "" && currentChat.responseInProgress
                                    Accessible.role: Accessible.Animation
                                    Accessible.name: qsTr("Busy indicator")
                                    Accessible.description: qsTr("The model is thinking")
                                }
                                Label {
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: theme.mutedTextColor
                                    text: {
                                        switch (currentChat.responseState) {
                                        case Chat.ResponseStopped: return qsTr("response stopped ...");
                                        case Chat.LocalDocsRetrieval: return qsTr("retrieving localdocs: ") + currentChat.collectionList.join(", ") + " ...";
                                        case Chat.LocalDocsProcessing: return qsTr("searching localdocs: ") + currentChat.collectionList.join(", ") + " ...";
                                        case Chat.PromptProcessing: return qsTr("processing ...")
                                        case Chat.ResponseGeneration: return qsTr("generating response ...");
                                        default: return ""; // handle unexpected values
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.leftMargin: 20
                            y: parent.topPadding + (parent.positionToRectangle(0).height / 2) - (height / 2)
                            width: 30
                            height: 30
                            radius: 5
                            color: name === qsTr("Response: ") ? theme.assistantColor : theme.userColor

                            Text {
                                anchors.centerIn: parent
                                text: name === qsTr("Response: ") ? "R" : "P"
                                color: "white"
                            }
                        }

                        ThumbsDownDialog {
                            id: thumbsDownDialog
                            property point globalPoint: mapFromItem(window,
                                window.width / 2 - width / 2,
                                window.height / 2 - height / 2)
                            x: globalPoint.x
                            y: globalPoint.y
                            property string text: value
                            response: newResponse === undefined || newResponse === "" ? text : newResponse
                            onAccepted: {
                                var responseHasChanged = response !== text && response !== newResponse
                                if (thumbsDownState && !thumbsUpState && !responseHasChanged)
                                    return

                                chatModel.updateNewResponse(index, response)
                                chatModel.updateThumbsUpState(index, false)
                                chatModel.updateThumbsDownState(index, true)
                                Network.sendConversation(currentChat.id, getConversationJson());
                            }
                        }

                        Column {
                            visible: name === qsTr("Response: ") &&
                                (!currentResponse || !currentChat.responseInProgress) && MySettings.networkIsActive
                            anchors.right: parent.right
                            anchors.rightMargin: 20
                            y: parent.topPadding + (parent.positionToRectangle(0).height / 2) - (height / 2)
                            spacing: 10

                            Item {
                                width: childrenRect.width
                                height: childrenRect.height
                                MyToolButton {
                                    id: thumbsUp
                                    width: 30
                                    height: 30
                                    opacity: thumbsUpState || thumbsUpState == thumbsDownState ? 1.0 : 0.2
                                    source: "qrc:/gpt4all/icons/thumbs_up.svg"
                                    Accessible.name: qsTr("Thumbs up")
                                    Accessible.description: qsTr("Gives a thumbs up to the response")
                                    onClicked: {
                                        if (thumbsUpState && !thumbsDownState)
                                            return

                                        chatModel.updateNewResponse(index, "")
                                        chatModel.updateThumbsUpState(index, true)
                                        chatModel.updateThumbsDownState(index, false)
                                        Network.sendConversation(currentChat.id, getConversationJson());
                                    }
                                }

                                MyToolButton {
                                    id: thumbsDown
                                    anchors.top: thumbsUp.top
                                    anchors.topMargin: 10
                                    anchors.left: thumbsUp.right
                                    anchors.leftMargin: 2
                                    width: 30
                                    height: 30
                                    checked: thumbsDownState
                                    opacity: thumbsDownState || thumbsUpState == thumbsDownState ? 1.0 : 0.2
                                    transform: [
                                      Matrix4x4 {
                                        matrix: Qt.matrix4x4(-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
                                      },
                                      Translate {
                                        x: thumbsDown.width
                                      }
                                    ]
                                    source: "qrc:/gpt4all/icons/thumbs_down.svg"
                                    Accessible.name: qsTr("Thumbs down")
                                    Accessible.description: qsTr("Opens thumbs down dialog")
                                    onClicked: {
                                        thumbsDownDialog.open()
                                    }
                                }
                            }
                        }
                    }

                    property bool shouldAutoScroll: true
                    property bool isAutoScrolling: false

                    Connections {
                        target: currentChat
                        function onResponseChanged() {
                            if (listView.shouldAutoScroll) {
                                listView.isAutoScrolling = true
                                listView.positionViewAtEnd()
                                listView.isAutoScrolling = false
                            }
                        }
                    }

                    onContentYChanged: {
                        if (!isAutoScrolling)
                            shouldAutoScroll = atYEnd
                    }

                    Component.onCompleted: {
                        shouldAutoScroll = true
                        positionViewAtEnd()
                    }

                    footer: Item {
                        id: bottomPadding
                        width: parent.width
                        height: 60
                    }
                }

                Image {
                    visible: currentChat.isServer || currentChat.modelInfo.isOnline
                    anchors.fill: parent
                    sourceSize.width: 1024
                    sourceSize.height: 1024
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.15
                    source: "qrc:/gpt4all/icons/network.svg"
                }
            }
        }

        MyButton {
            id: myButton
            visible: chatModel.count && !currentChat.isServer
            textColor: theme.textColor
            Image {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 15
                source: currentChat.responseInProgress ? "qrc:/gpt4all/icons/stop_generating.svg" : "qrc:/gpt4all/icons/regenerate.svg"
            }
            leftPadding: 50
            onClicked: {
                var index = Math.max(0, chatModel.count - 1);
                var listElement = chatModel.get(index);

                if (currentChat.responseInProgress) {
                    listElement.stopped = true
                    currentChat.stopGenerating()
                } else {
                    currentChat.regenerateResponse()
                    if (chatModel.count) {
                        if (listElement.name === qsTr("Response: ")) {
                            chatModel.updateCurrentResponse(index, true);
                            chatModel.updateStopped(index, false);
                            chatModel.updateThumbsUpState(index, false);
                            chatModel.updateThumbsDownState(index, false);
                            chatModel.updateNewResponse(index, "");
                            currentChat.prompt(listElement.prompt)
                        }
                    }
                }
            }
            background: Rectangle {
                border.color: theme.conversationButtonBorder
                border.width: 2
                radius: 10
                color: myButton.hovered ? theme.conversationButtonBackgroundHovered : theme.conversationButtonBackground
            }
            anchors.bottom: textInputView.top
            anchors.horizontalCenter: textInputView.horizontalCenter
            anchors.bottomMargin: 20
            padding: 15
            text: currentChat.responseInProgress ? qsTr("Stop generating") : qsTr("Regenerate response")
            Accessible.description: qsTr("Controls generation of the response")
        }

        Text {
            id: device
            anchors.bottom: textInputView.top
            anchors.bottomMargin: 20
            anchors.right: parent.right
            anchors.rightMargin: 30
            color: theme.mutedTextColor
            visible: currentChat.tokenSpeed !== ""
            text: qsTr("Speed: ") + currentChat.tokenSpeed + "<br>" + qsTr("Device: ") + currentChat.device + currentChat.fallbackReason
            font.pixelSize: theme.fontSizeLarge
        }

        RectangularGlow {
            id: effect
            visible: !currentChat.isServer
            anchors.fill: textInputView
            glowRadius: 50
            spread: 0
            color: theme.sendGlow
            cornerRadius: 10
            opacity: 0.1
        }

        ScrollView {
            id: textInputView
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 30
            height: Math.min(contentHeight, 200)
            visible: !currentChat.isServer
            MyTextArea {
                id: textInput
                color: theme.textColor
                topPadding: 30
                bottomPadding: 30
                leftPadding: 20
                rightPadding: 40
                enabled: currentChat.isModelLoaded && !currentChat.isServer
                font.pixelSize: theme.fontSizeLarger
                placeholderText: qsTr("Send a message...")
                Accessible.role: Accessible.EditableText
                Accessible.name: placeholderText
                Accessible.description: qsTr("Send messages/prompts to the model")
                Keys.onReturnPressed: (event)=> {
                    if (event.modifiers & Qt.ControlModifier || event.modifiers & Qt.ShiftModifier)
                        event.accepted = false;
                    else {
                        editingFinished();
                        sendMessage()
                    }
                }
                function sendMessage() {
                    if (textInput.text === "")
                        return

                    currentChat.stopGenerating()
                    currentChat.newPromptResponsePair(textInput.text);
                    currentChat.prompt(textInput.text,
                               MySettings.promptTemplate,
                               MySettings.maxLength,
                               MySettings.topK,
                               MySettings.topP,
                               MySettings.temperature,
                               MySettings.promptBatchSize,
                               MySettings.repeatPenalty,
                               MySettings.repeatPenaltyTokens)
                    textInput.text = ""
                }
            }
        }

        MyToolButton {
            backgroundColor: theme.sendButtonBackground
            backgroundColorHovered: theme.sendButtonBackgroundHovered
            anchors.right: textInputView.right
            anchors.verticalCenter: textInputView.verticalCenter
            anchors.rightMargin: 15
            width: 30
            height: 30
            visible: !currentChat.isServer
            source: "qrc:/gpt4all/icons/send_message.svg"
            Accessible.name: qsTr("Send message")
            Accessible.description: qsTr("Sends the message/prompt contained in textfield to the model")

            onClicked: {
                textInput.sendMessage()
            }
        }
    }
}
