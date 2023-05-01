import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import llm
import download
import network

Window {
    id: window
    width: 1280
    height: 720
    visible: true
    title: qsTr("GPT4All v") + Qt.application.version

    Theme {
        id: theme
    }

    property var chatModel: LLM.currentChat.chatModel

    color: theme.textColor

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

    function startupDialogs() {
        // check for first time start of this version
        if (Download.isFirstStart()) {
            firstStartDialog.open();
            return;
        }

        // check for any current models and if not, open download dialog
        if (LLM.modelList.length === 0 && !firstStartDialog.opened) {
            downloadNewModels.open();
            return;
        }

        // check for new version
        if (Download.hasNewerRelease && !firstStartDialog.opened && !downloadNewModels.opened) {
            newVersionDialog.open();
            return;
        }
    }

    StartupDialog {
        id: firstStartDialog
        anchors.centerIn: parent
    }

    NewVersionDialog {
        id: newVersionDialog
        anchors.centerIn: parent
    }

    Item {
        Accessible.role: Accessible.Window
        Accessible.name: title
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
            visible: LLM.isModelLoaded

            Label {
                id: modelLabel
                color: theme.textColor
                padding: 20
                font.pixelSize: 24
                text: ""
                background: Rectangle {
                    color: theme.backgroundDarkest
                }
                horizontalAlignment: TextInput.AlignRight
            }

            ComboBox {
                id: comboBox
                width: 350
                anchors.top: modelLabel.top
                anchors.bottom: modelLabel.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: theme.fontSizeLarge
                spacing: 0
                model: LLM.modelList
                Accessible.role: Accessible.ComboBox
                Accessible.name: qsTr("ComboBox for displaying/picking the current model")
                Accessible.description: qsTr("Use this for picking the current model to use; the first item is the current model")
                contentItem: Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    leftPadding: 10
                    rightPadding: 10
                    text: comboBox.displayText
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
                popup: Popup {
                    y: comboBox.height - 1
                    width: comboBox.width
                    implicitHeight: contentItem.implicitHeight
                    padding: 0

                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: comboBox.popup.visible ? comboBox.delegateModel : null
                        currentIndex: comboBox.highlightedIndex
                        ScrollIndicator.vertical: ScrollIndicator { }
                    }

                    background: Rectangle {
                        color: theme.backgroundDark
                    }
                }

                background: Rectangle {
                    color: theme.backgroundDark
                }

                onActivated: {
                    LLM.stopGenerating()
                    LLM.modelName = comboBox.currentText
                    LLM.currentChat.reset();
                }
            }
        }

        BusyIndicator {
            anchors.centerIn: parent
            visible: !LLM.isModelLoaded
            running: !LLM.isModelLoaded
            Accessible.role: Accessible.Animation
            Accessible.name: qsTr("Busy indicator")
            Accessible.description: qsTr("Displayed when the model is loading")
        }
    }

    SettingsDialog {
        id: settingsDialog
        anchors.centerIn: parent
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
                color: theme.backgroundLightest
                width: parent.width
                height: 6
                radius: 2
                antialiasing: true
            }

            Rectangle {
                id: bar2
                anchors.centerIn: parent
                color: theme.backgroundLightest
                width: parent.width
                height: 6
                radius: 2
                antialiasing: true
            }

            Rectangle {
                id: bar3
                anchors.bottom: parent.bottom
                color: theme.backgroundLightest
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
        Item {
            Accessible.role: Accessible.Dialog
            Accessible.name: qsTr("Network dialog")
            Accessible.description: qsTr("Dialog for opt-in to sharing feedback/conversations")
        }
    }

    Button {
        id: networkButton
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.rightMargin: 30
        width: 40
        height: 40
        z: 200
        padding: 15

        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Network button")
        Accessible.description: qsTr("Reveals a dialogue where you can opt-in for sharing data over network")

        background: Item {
            anchors.fill: parent
            Rectangle {
                anchors.fill: parent
                color: "transparent"
                visible: Network.isActive
                border.color: theme.backgroundLightest
                border.width: 1
                radius: 10
            }
            Image {
                anchors.centerIn: parent
                width: 30
                height: 30
                source: "qrc:/gpt4all/icons/network.svg"
            }
        }

        onClicked: {
            if (Network.isActive)
                Network.isActive = false
            else
                networkDialog.open()
        }
    }

    Connections {
        target: Network
        function onHealthCheckFailed(code) {
            healthCheckFailed.open();
        }
    }

    Button {
        id: settingsButton
        anchors.right: networkButton.left
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.rightMargin: 30
        width: 40
        height: 40
        z: 200
        padding: 15

        background: Item {
            anchors.fill: parent
            Image {
                anchors.centerIn: parent
                width: 30
                height: 30
                source: "qrc:/gpt4all/icons/settings.svg"
            }
        }

        Accessible.role: Accessible.Button
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
            target: LLM
            function onRecalcChanged() {
                if (LLM.isRecalc)
                    recalcPopup.open()
                else
                    recalcPopup.close()
            }
        }
    }

    Button {
        id: copyButton
        anchors.right: settingsButton.left
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.rightMargin: 30
        width: 40
        height: 40
        z: 200
        padding: 15

        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Copy button")
        Accessible.description: qsTr("Copy the conversation to the clipboard")

        background: Item {
            anchors.fill: parent
            Image {
                anchors.centerIn: parent
                width: 30
                height: 30
                source: "qrc:/gpt4all/icons/copy.svg"
            }
        }

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
            if (item.currentResponse)
                string += LLM.response
            else
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
            if (item.currentResponse)
                str += JSON.stringify(LLM.response)
            else
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

    Button {
        id: resetContextButton
        anchors.right: copyButton.left
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.rightMargin: 30
        width: 40
        height: 40
        z: 200
        padding: 15

        Accessible.role: Accessible.Button
        Accessible.name: text
        Accessible.description: qsTr("Reset the context which erases current conversation")

        background: Item {
            anchors.fill: parent
            Image {
                anchors.centerIn: parent
                width: 30
                height: 30
                source: "qrc:/gpt4all/icons/regenerate.svg"
            }
        }

        onClicked: {
            LLM.stopGenerating()
            LLM.resetContext()
            LLM.currentChat.reset();
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
        Item {
            Accessible.role: Accessible.Dialog
            Accessible.name: qsTr("Download new models dialog")
            Accessible.description: qsTr("Dialog for downloading new models")
        }
    }

    Drawer {
        id: drawer
        y: header.height
        width: 0.3 * window.width
        height: window.height - y
        modal: false
        opacity: 0.9

        background: Rectangle {
            height: parent.height
            color: theme.backgroundDarkest
        }

        Item {
            anchors.fill: parent
            anchors.margins: 30

            Accessible.role: Accessible.Pane
            Accessible.name: qsTr("Drawer on the left of the application")
            Accessible.description: qsTr("Drawer that is revealed by pressing the hamburger button")

            Label {
                id: conversationList
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                wrapMode: Text.WordWrap
                text: qsTr("Chat lists of specific conversations coming soon! Check back often for new features :)")
                color: theme.textColor

                Accessible.role: Accessible.Paragraph
                Accessible.name: qsTr("Coming soon")
                Accessible.description: text
            }

            Label {
                id: discordLink
                textFormat: Text.RichText
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: conversationList.bottom
                anchors.topMargin: 20
                wrapMode: Text.WordWrap
                text: qsTr("Check out our discord channel <a href=\"https://discord.gg/4M2QFmTt2k\">https://discord.gg/4M2QFmTt2k</a>")
                onLinkActivated: { Qt.openUrlExternally("https://discord.gg/4M2QFmTt2k") }
                color: theme.textColor
                linkColor: theme.linkColor

                Accessible.role: Accessible.Link
                Accessible.name: qsTr("Discord link")
            }

            Label {
                id: nomicProps
                textFormat: Text.RichText
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: discordLink.bottom
                anchors.topMargin: 20
                wrapMode: Text.WordWrap
                text: qsTr("Thanks to <a href=\"https://home.nomic.ai\">Nomic AI</a> and the community for contributing so much great data and energy!")
                onLinkActivated: { Qt.openUrlExternally("https://home.nomic.ai") }
                color: theme.textColor
                linkColor: theme.linkColor

                Accessible.role: Accessible.Paragraph
                Accessible.name: qsTr("Thank you blurb")
                Accessible.description: qsTr("Contains embedded link to https://home.nomic.ai")
            }

            Button {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: downloadButton.top
                anchors.bottomMargin: 20
                padding: 15
                contentItem: Text {
                    text: qsTr("Check for updates...")
                    horizontalAlignment: Text.AlignHCenter
                    color: theme.textColor

                    Accessible.role: Accessible.Button
                    Accessible.name: text
                    Accessible.description: qsTr("Use this to launch an external application that will check for updates to the installer")
                }

                background: Rectangle {
                    opacity: .5
                    border.color: theme.backgroundLightest
                    border.width: 1
                    radius: 10
                    color: theme.backgroundLight
                }

                onClicked: {
                    if (!LLM.checkForUpdates())
                        checkForUpdatesError.open()
                }
            }

            Button {
                id: downloadButton
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                padding: 15
                contentItem: Text {
                    text: qsTr("Download new models...")
                    horizontalAlignment: Text.AlignHCenter
                    color: theme.textColor

                    Accessible.role: Accessible.Button
                    Accessible.name: text
                    Accessible.description: qsTr("Use this to launch a dialog to download new models")
                }

                background: Rectangle {
                    opacity: .5
                    border.color: theme.backgroundLightest
                    border.width: 1
                    radius: 10
                    color: theme.backgroundLight
                }

                onClicked: {
                    downloadNewModels.open()
                }
            }

        }
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
            anchors.bottom: textInputView.top
            anchors.bottomMargin: 30
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn

            Rectangle {
                anchors.fill: parent
                color: theme.backgroundLighter

                ListView {
                    id: listView
                    anchors.fill: parent
                    model: chatModel

                    Accessible.role: Accessible.List
                    Accessible.name: qsTr("List of prompt/response pairs")
                    Accessible.description: qsTr("This is the list of prompt/response pairs comprising the actual conversation with the model")

                    delegate: TextArea {
                        text: currentResponse ? LLM.response : (value ? value : "")
                        width: listView.width
                        color: theme.textColor
                        wrapMode: Text.WordWrap
                        focus: false
                        readOnly: true
                        font.pixelSize: theme.fontSizeLarge
                        cursorVisible: currentResponse ? (LLM.response !== "" ? LLM.responseInProgress : false) : false
                        cursorPosition: text.length
                        background: Rectangle {
                            color: name === qsTr("Response: ") ? theme.backgroundLighter : theme.backgroundLight
                        }

                        Accessible.role: Accessible.Paragraph
                        Accessible.name: name
                        Accessible.description: name === qsTr("Response: ") ? "The response by the model" : "The prompt by the user"

                        topPadding: 20
                        bottomPadding: 20
                        leftPadding: 100
                        rightPadding: 100

                        BusyIndicator {
                            anchors.left: parent.left
                            anchors.leftMargin: 90
                            anchors.top: parent.top
                            anchors.topMargin: 5
                            visible: (currentResponse ? true : false) && LLM.response === "" && LLM.responseInProgress
                            running: (currentResponse ? true : false) && LLM.response === "" && LLM.responseInProgress

                            Accessible.role: Accessible.Animation
                            Accessible.name: qsTr("Busy indicator")
                            Accessible.description: qsTr("Displayed when the model is thinking")
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.leftMargin: 20
                            anchors.topMargin: 20
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
                            property string text: currentResponse ? LLM.response : (value ? value : "")
                            response: newResponse === undefined || newResponse === "" ? text : newResponse
                            onAccepted: {
                                var responseHasChanged = response !== text && response !== newResponse
                                if (thumbsDownState && !thumbsUpState && !responseHasChanged)
                                    return

                                chatModel.updateNewResponse(index, response)
                                chatModel.updateThumbsUpState(index, false)
                                chatModel.updateThumbsDownState(index, true)
                                Network.sendConversation(LLM.currentChat.id, getConversationJson());
                            }
                        }

                        Column {
                            visible: name === qsTr("Response: ") &&
                                (!currentResponse || !LLM.responseInProgress) && Network.isActive
                            anchors.right: parent.right
                            anchors.rightMargin: 20
                            anchors.top: parent.top
                            anchors.topMargin: 20
                            spacing: 10

                            Item {
                                width: childrenRect.width
                                height: childrenRect.height
                                Button {
                                    id: thumbsUp
                                    width: 30
                                    height: 30
                                    opacity: thumbsUpState || thumbsUpState == thumbsDownState ? 1.0 : 0.2
                                    background: Image {
                                        anchors.fill: parent
                                        source: "qrc:/gpt4all/icons/thumbs_up.svg"
                                    }
                                    onClicked: {
                                        if (thumbsUpState && !thumbsDownState)
                                            return

                                        chatModel.updateNewResponse(index, "")
                                        chatModel.updateThumbsUpState(index, true)
                                        chatModel.updateThumbsDownState(index, false)
                                        Network.sendConversation(LLM.currentChat.id, getConversationJson());
                                    }
                                }

                                Button {
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
                                    background: Image {
                                        anchors.fill: parent
                                        source: "qrc:/gpt4all/icons/thumbs_down.svg"
                                    }
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
                        target: LLM
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
            }
        }

        Button {
            visible: chatModel.count
            Image {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 15
                source: LLM.responseInProgress ? "qrc:/gpt4all/icons/stop_generating.svg" : "qrc:/gpt4all/icons/regenerate.svg"
            }
            leftPadding: 50
            onClicked: {
                var index = Math.max(0, chatModel.count - 1);
                var listElement = chatModel.get(index);

                if (LLM.responseInProgress) {
                    listElement.stopped = true
                    LLM.stopGenerating()
                } else {
                    LLM.regenerateResponse()
                    if (chatModel.count) {
                        if (listElement.name === qsTr("Response: ")) {
                            chatModel.updateCurrentResponse(index, true);
                            chatModel.updateStopped(index, false);
                            chatModel.updateValue(index, LLM.response);
                            chatModel.updateThumbsUpState(index, false);
                            chatModel.updateThumbsDownState(index, false);
                            chatModel.updateNewResponse(index, "");
                            LLM.prompt(listElement.prompt, settingsDialog.promptTemplate,
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
            anchors.bottomMargin: 40
            padding: 15
            contentItem: Text {
                text: LLM.responseInProgress ? qsTr("Stop generating") : qsTr("Regenerate response")
                color: theme.textColor
                Accessible.role: Accessible.Button
                Accessible.name: text
                Accessible.description: qsTr("Controls generation of the response")
            }
            background: Rectangle {
                opacity: .5
                border.color: theme.backgroundLightest
                border.width: 1
                radius: 10
                color: theme.backgroundLight
            }
        }

        ScrollView {
            id: textInputView
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 30
            height: Math.min(contentHeight, 200)

            TextArea {
                id: textInput
                color: theme.textColor
                padding: 20
                rightPadding: 40
                enabled: LLM.isModelLoaded
                wrapMode: Text.WordWrap
                font.pixelSize: theme.fontSizeLarge
                placeholderText: qsTr("Send a message...")
                placeholderTextColor: theme.backgroundLightest
                background: Rectangle {
                    color: theme.backgroundLighter
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

                    LLM.stopGenerating()

                    if (chatModel.count) {
                        var index = Math.max(0, chatModel.count - 1);
                        var listElement = chatModel.get(index);
                        chatModel.updateCurrentResponse(index, false);
                        chatModel.updateValue(index, LLM.response);
                    }
                    var prompt = textInput.text + "\n"
                    chatModel.appendPrompt(qsTr("Prompt: "), textInput.text);
                    chatModel.appendResponse(qsTr("Response: "), prompt);
                    LLM.resetResponse()
                    LLM.prompt(prompt, settingsDialog.promptTemplate,
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

        Button {
            anchors.right: textInputView.right
            anchors.verticalCenter: textInputView.verticalCenter
            anchors.rightMargin: 15
            width: 30
            height: 30

            background: Image {
                anchors.centerIn: parent
                source: "qrc:/gpt4all/icons/send_message.svg"
            }

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Send the message button")
            Accessible.description: qsTr("Sends the message/prompt contained in textfield to the model")

            onClicked: {
                textInput.sendMessage()
            }
        }
    }
}
