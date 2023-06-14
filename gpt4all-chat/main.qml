import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import llm
import download
import network
import gpt4all

Window {
    id: window
    width: 1280
    height: 720
    minimumWidth: 720
    minimumHeight: 480
    visible: true
    title: qsTr("GPT4All v") + Qt.application.version

    Theme {
        id: theme
    }

    property var currentChat: LLM.chatListModel.currentChat
    property var chatModel: currentChat.chatModel

    color: theme.backgroundDarkest

    // Startup code
    Component.onCompleted: {
        if (!LLM.compatHardware) {
            Network.sendNonCompatHardware();
            errorCompatHardware.open();
        } else
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
            if (Network.isActive && !currentChat.responseInProgress)
                Network.sendConversation(currentChat.id, getConversationJson());
        }
        function onModelLoadingErrorChanged() {
            if (currentChat.modelLoadingError !== "")
                modelLoadingErrorPopup.open()
        }
    }

    function startupDialogs() {
        // check for first time start of this version
        if (Download.isFirstStart()) {
            firstStartDialog.open();
            return;
        }

        // check for any current models and if not, open download dialog
        if (currentChat.modelList.length === 0 && !firstStartDialog.opened) {
            downloadNewModels.open();
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
        text: qsTr("Incompatible hardware detected. Your hardware does not meet the minimal requirements to run GPT4All. In particular, it does not seem to support AVX intrinsics. See here for more: https://en.wikipedia.org/wiki/Advanced_Vector_Extensions")
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
        text: currentChat.modelLoadingError
    }

    Rectangle {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 100
        color: theme.backgroundDarkest
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
                    color: theme.backgroundDarkest
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
                model: currentChat.modelList
                contentItem: Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    leftPadding: 10
                    rightPadding: 20
                    text: currentChat.modelLoadingError !== "" ? "Model loading error..." : comboBox.displayText
                    font: comboBox.font
                    color: theme.textColor
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }
                delegate: ItemDelegate {
                    width: comboBox.width
                    contentItem: Text {
                        text: modelData
                        color: theme.textColor
                        font: comboBox.font
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: highlighted ? theme.backgroundLight : theme.backgroundDark
                    }
                    highlighted: comboBox.highlightedIndex === index
                }
                Accessible.role: Accessible.ComboBox
                Accessible.name: qsTr("ComboBox for displaying/picking the current model")
                Accessible.description: qsTr("Use this for picking the current model to use; the first item is the current model")
                onActivated: {
                    currentChat.stopGenerating()
                    currentChat.reset();
                    currentChat.modelName = comboBox.currentText
                }
            }
        }

        Item {
            anchors.centerIn: parent
            visible: !currentChat.isModelLoaded && currentChat.modelLoadingError === "" && !currentChat.isServer
            width: childrenRect.width
            height: childrenRect.height
            Row {
                spacing: 5
                MyBusyIndicator {
                    anchors.verticalCenter: parent.verticalCenter
                    running: parent.visible
                    Accessible.role: Accessible.Animation
                    Accessible.name: qsTr("Busy indicator")
                    Accessible.description: qsTr("Displayed when the model is loading")
                }

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Loading model...")
                    color: theme.textAccent
                }
            }
        }
    }

    SettingsDialog {
        id: settingsDialog
        anchors.centerIn: parent
        width: Math.min(1024, window.width - (window.width * .2))
        height: Math.min(600, window.height - (window.height * .2))
    }

    Button {
        id: drawerButton
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.leftMargin: 30
        width: 40
        height: 40
        z: 200
        padding: 15

        Accessible.role: Accessible.ButtonMenu
        Accessible.name: qsTr("Hamburger button")
        Accessible.description: qsTr("Hamburger button that reveals a drawer on the left of the application")

        background: Item {
            anchors.centerIn: parent
            width: 30
            height: 30

            Rectangle {
                id: bar1
                color: drawerButton.hovered ? theme.textColor : theme.backgroundLightest
                width: parent.width
                height: 6
                radius: 2
                antialiasing: true
            }

            Rectangle {
                id: bar2
                anchors.centerIn: parent
                color: drawerButton.hovered ? theme.textColor : theme.backgroundLightest
                width: parent.width
                height: 6
                radius: 2
                antialiasing: true
            }

            Rectangle {
                id: bar3
                anchors.bottom: parent.bottom
                color: drawerButton.hovered ? theme.textColor : theme.backgroundLightest
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
            Accessible.description: qsTr("Dialog for opt-in to sharing feedback/conversations")
        }
    }

    MyToolButton {
        id: networkButton
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.rightMargin: 30
        width: 40
        height: 40
        z: 200
        padding: 15
        toggled: Network.isActive
        source: "qrc:/gpt4all/icons/network.svg"
        Accessible.name: qsTr("Network button")
        Accessible.description: qsTr("Reveals a dialogue where you can opt-in for sharing data over network")

        onClicked: {
            if (Network.isActive) {
                Network.isActive = false
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
    }

    MyToolButton {
        id: collectionsButton
        anchors.right: networkButton.left
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.rightMargin: 10
        width: 40
        height: 40
        z: 200
        padding: 15
        toggled: currentChat.collectionList.length
        source: "qrc:/gpt4all/icons/db.svg"
        Accessible.name: qsTr("Add collections of documents to the chat")
        Accessible.description: qsTr("Provides a button to add collections of documents to the chat")

        onClicked: {
            collectionsDialog.open()
        }
    }

    MyToolButton {
        id: settingsButton
        anchors.right: collectionsButton.left
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.rightMargin: 10
        width: 40
        height: 40
        z: 200
        padding: 15
        source: "qrc:/gpt4all/icons/settings.svg"
        Accessible.name: qsTr("Settings button")
        Accessible.description: qsTr("Reveals a dialogue where you can change various settings")

        onClicked: {
            settingsDialog.open()
        }
    }

    PopupDialog {
        id: copyMessage
        anchors.centerIn: parent
        text: qsTr("Conversation copied to clipboard.")
    }

    PopupDialog {
        id: copyCodeMessage
        anchors.centerIn: parent
        text: qsTr("Code copied to clipboard.")
    }

    PopupDialog {
        id: healthCheckFailed
        anchors.centerIn: parent
        text: qsTr("Connection to datalake failed.")
    }

    PopupDialog {
        id: recalcPopup
        anchors.centerIn: parent
        shouldTimeOut: false
        shouldShowBusy: true
        text: qsTr("Recalculating context.")

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

    MyToolButton {
        id: copyButton
        anchors.right: settingsButton.left
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.rightMargin: 10
        width: 40
        height: 40
        z: 200
        padding: 15
        source: "qrc:/gpt4all/icons/copy.svg"
        Accessible.name: qsTr("Copy button")
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
        anchors.right: copyButton.left
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.rightMargin: 10
        width: 40
        height: 40
        z: 200
        padding: 15
        source: "qrc:/gpt4all/icons/regenerate.svg"

        Accessible.name: text
        Accessible.description: qsTr("Reset the context which erases current conversation")

        onClicked: {
            Network.sendResetContext(chatModel.count)
            currentChat.reset();
        }
    }

    Dialog {
        id: checkForUpdatesError
        anchors.centerIn: parent
        modal: false
        opacity: 0.9
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
            color: theme.textColor
            Accessible.role: Accessible.Dialog
            Accessible.name: text
            Accessible.description: qsTr("Dialog indicating an error")
        }
        background: Rectangle {
            anchors.fill: parent
            color: theme.backgroundDarkest
            border.width: 1
            border.color: theme.dialogBorder
            radius: 10
        }
    }

    ModelDownloaderDialog {
        id: downloadNewModels
        anchors.centerIn: parent
        width: Math.min(1024, window.width - (window.width * .2))
        height: Math.min(600, window.height - (window.height * .2))
        Item {
            Accessible.role: Accessible.Dialog
            Accessible.name: qsTr("Download new models dialog")
            Accessible.description: qsTr("Dialog for downloading new models")
        }
    }

    ChatDrawer {
        id: drawer
        y: header.height
        width: 0.3 * window.width
        height: window.height - y
        onDownloadClicked: {
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
        color: theme.backgroundLight
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.top: header.bottom

        ScrollView {
            id: scrollView
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: !currentChat.isServer ? textInputView.top : parent.bottom
            anchors.bottomMargin: !currentChat.isServer ? 30 : 0
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn

            Rectangle {
                anchors.fill: parent
                color: currentChat.isServer ? theme.backgroundDark : theme.backgroundLight

                ListView {
                    id: listView
                    anchors.fill: parent
                    model: chatModel

                    Accessible.role: Accessible.List
                    Accessible.name: qsTr("List of prompt/response pairs")
                    Accessible.description: qsTr("This is the list of prompt/response pairs comprising the actual conversation with the model")

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
                                ? (currentChat.isServer ? theme.backgroundDarkest : theme.backgroundLighter)
                                : (currentChat.isServer ? theme.backgroundDark : theme.backgroundLight)
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
                            responseText.setHeaderColor(name === qsTr("Response: ") ? theme.backgroundLight : theme.backgroundLighter);
                            responseText.textDocument = textDocument
                        }

                        Accessible.role: Accessible.Paragraph
                        Accessible.name: name
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
                                    Accessible.description: qsTr("Displayed when the model is thinking")
                                }
                                Label {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: currentChat.responseState + "..."
                                    color: theme.textAccent
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
                                (!currentResponse || !currentChat.responseInProgress) && Network.isActive
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
                    visible: currentChat.isServer || currentChat.modelName.startsWith("chatgpt-")
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
            visible: chatModel.count && !currentChat.isServer
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
                            currentChat.prompt(listElement.prompt, settingsDialog.promptTemplate,
                                       settingsDialog.maxLength,
                                       settingsDialog.topK, settingsDialog.topP,
                                       settingsDialog.temperature,
                                       settingsDialog.promptBatchSize,
                                       settingsDialog.repeatPenalty,
                                       settingsDialog.repeatPenaltyTokens)
                        }
                    }
                }
            }
            anchors.bottom: textInputView.top
            anchors.horizontalCenter: textInputView.horizontalCenter
            anchors.bottomMargin: 20
            padding: 15
            text: currentChat.responseInProgress ? qsTr("Stop generating") : qsTr("Regenerate response")
            Accessible.description: qsTr("Controls generation of the response")
        }

        RectangularGlow {
            id: effect
            anchors.fill: textInputView
            glowRadius: 50
            spread: 0
            color: theme.backgroundDark
            cornerRadius: 10
            opacity: 0.2
        }

        ScrollView {
            id: textInputView
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 30
            height: Math.min(contentHeight, 200)
            visible: !currentChat.isServer
            TextArea {
                id: textInput
                color: theme.textColor
                topPadding: 30
                bottomPadding: 30
                leftPadding: 20
                rightPadding: 40
                enabled: currentChat.isModelLoaded && !currentChat.isServer
                wrapMode: Text.WordWrap
                font.pixelSize: theme.fontSizeLarger
                placeholderText: qsTr("Send a message...")
                placeholderTextColor: theme.mutedTextColor
                background: Rectangle {
                    color: theme.backgroundAccent
                    radius: 10
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: placeholderText
                Accessible.description: qsTr("Textfield for sending messages/prompts to the model")
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
                    currentChat.prompt(textInput.text, settingsDialog.promptTemplate,
                               settingsDialog.maxLength,
                               settingsDialog.topK,
                               settingsDialog.topP,
                               settingsDialog.temperature,
                               settingsDialog.promptBatchSize,
                               settingsDialog.repeatPenalty,
                               settingsDialog.repeatPenaltyTokens)
                    textInput.text = ""
                }
            }
        }

        MyToolButton {
            anchors.right: textInputView.right
            anchors.verticalCenter: textInputView.verticalCenter
            anchors.rightMargin: 15
            width: 30
            height: 30
            visible: !currentChat.isServer
            source: "qrc:/gpt4all/icons/send_message.svg"
            Accessible.name: qsTr("Send the message button")
            Accessible.description: qsTr("Sends the message/prompt contained in textfield to the model")

            onClicked: {
                textInput.sendMessage()
            }
        }
    }
}
