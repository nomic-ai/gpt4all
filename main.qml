import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import llm

Window {
    id: window
    width: 1280
    height: 720
    visible: true
    title: qsTr("GPT4All Chat")
    color: "#d1d5db"

    Rectangle {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 100
        color: "#202123"

        Item {
            anchors.centerIn: parent
            width: childrenRect.width
            height: childrenRect.height
            visible: LLM.isModelLoaded

            Label {
                id: modelNameField
                color: "#d1d5db"
                padding: 20
                font.pixelSize: 24
                text: "GPT4ALL Model: " + LLM.modelName
                background: Rectangle {
                    color: "#202123"
                }
                horizontalAlignment: TextInput.AlignHCenter
            }
        }

        BusyIndicator {
            anchors.centerIn: parent
            visible: !LLM.isModelLoaded
            running: !LLM.isModelLoaded
        }
    }


    Dialog {
        id: settingsDialog
        modal: true
        anchors.centerIn: parent
        title: qsTr("Settings")
        height: 600
        width: 600
        property real defaultTemperature: 0.7
        property real defaultTopP: 0.95
        property int defaultTopK: 40
        property int defaultMaxLength: 4096
        property int defaultPromptBatchSize: 9

        property string defaultPromptTemplate: "The prompt below is a question to answer, a task to complete, or a conversation to respond to; decide which and write an appropriate response.
### Prompt:
%1
### Response:\n"

        property string promptTemplate: ""
        property real temperature: 0.0
        property real topP: 0.0
        property int topK: 0
        property int maxLength: 0
        property int promptBatchSize: 0

        function restoreDefaults() {
            temperature = defaultTemperature;
            topP = defaultTopP;
            topK = defaultTopK;
            maxLength = defaultMaxLength;
            promptBatchSize = defaultPromptBatchSize;
            promptTemplate = defaultPromptTemplate;
        }

        Component.onCompleted: {
            restoreDefaults();
        }

        GridLayout {
            columns: 2
            rowSpacing: 10
            columnSpacing: 10
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            Label {
                text: qsTr("Temperature:")
                Layout.row: 0
                Layout.column: 0
            }
            TextField {
                text: settingsDialog.temperature.toString()
                ToolTip.text: qsTr("Temperature increases the chances of choosing less likely tokens - higher temperature gives more creative but less predictable outputs")
                ToolTip.visible: hovered
                Layout.row: 0
                Layout.column: 1
                validator: DoubleValidator { }
                onAccepted: {
                    var val = parseFloat(text)
                    if (!isNaN(val)) {
                        settingsDialog.temperature = val
                        focus = false
                    } else {
                        text = settingsDialog.temperature.toString()
                    }
                }
            }
            Label {
                text: qsTr("Top P:")
                Layout.row: 1
                Layout.column: 0
            }
            TextField {
                text: settingsDialog.topP.toString()
                ToolTip.text: qsTr("Only the most likely tokens up to a total probability of top_p can be chosen, prevents choosing highly unlikely tokens, aka Nucleus Sampling")
                ToolTip.visible: hovered
                Layout.row: 1
                Layout.column: 1
                validator: DoubleValidator {}
                onAccepted: {
                    var val = parseFloat(text)
                    if (!isNaN(val)) {
                        settingsDialog.topP = val
                        focus = false
                    } else {
                        text = settingsDialog.topP.toString()
                    }
                }
            }
            Label {
                 text: qsTr("Top K:")
                 Layout.row: 2
                 Layout.column: 0
             }
             TextField {
                 text: settingsDialog.topK.toString()
                 ToolTip.text: qsTr("Only the top K most likely tokens will be chosen from")
                 ToolTip.visible: hovered
                 Layout.row: 2
                 Layout.column: 1
                 validator: IntValidator { bottom: 1 }
                 onAccepted: {
                     var val = parseInt(text)
                     if (!isNaN(val)) {
                         settingsDialog.topK = val
                         focus = false
                     } else {
                         text = settingsDialog.topK.toString()
                     }
                 }
             }
             Label {
                 text: qsTr("Max Length:")
                 Layout.row: 3
                 Layout.column: 0
             }
             TextField {
                 text: settingsDialog.maxLength.toString()
                 ToolTip.text: qsTr("Maximum length of response in tokens")
                 ToolTip.visible: hovered
                 Layout.row: 3
                 Layout.column: 1
                 validator: IntValidator { bottom: 1 }
                 onAccepted: {
                     var val = parseInt(text)
                     if (!isNaN(val)) {
                         settingsDialog.maxLength = val
                         focus = false
                     } else {
                         text = settingsDialog.maxLength.toString()
                     }
                 }
             }

             Label {
                 text: qsTr("Prompt Batch Size:")
                 Layout.row: 4
                 Layout.column: 0
             }
             TextField {
                 text: settingsDialog.promptBatchSize.toString()
                 ToolTip.text: qsTr("Amount of prompt tokens to process at once, higher values can speed up reading prompts but will use more RAM")
                 ToolTip.visible: hovered
                 Layout.row: 4
                 Layout.column: 1
                 validator: IntValidator { bottom: 1 }
                 onAccepted: {
                     var val = parseInt(text)
                     if (!isNaN(val)) {
                         settingsDialog.promptBatchSize = val
                         focus = false
                     } else {
                         text = settingsDialog.promptBatchSize.toString()
                     }
                 }
             }

             Label {
                 text: qsTr("Prompt Template:")
                 Layout.row: 5
                 Layout.column: 0
             }
             Rectangle {
                 Layout.row: 5
                 Layout.column: 1
                 Layout.fillWidth: true
                 height: 200
                 color: "#222"
                 border.width: 1
                 border.color: "#ccc"
                 radius: 5
                 Label {
                    visible: settingsDialog.promptTemplate.indexOf("%1") == -1
                    font.bold: true
                    color: "red"
                    text: qsTr("Prompt template must contain %1 to be replaced with the user's input.")
                    anchors.bottom: templateScrollView.top
                 }
                 ScrollView {
                     id: templateScrollView
                     anchors.fill: parent
                     TextArea {
                         text: settingsDialog.promptTemplate
                         wrapMode: TextArea.Wrap
                         onTextChanged: {
                             settingsDialog.promptTemplate = text
                         }
                         bottomPadding: 10
                     }
                 }
             }
             Button {
                 Layout.row: 6
                 Layout.column: 1
                 Layout.fillWidth: true
                 padding: 15
                 contentItem: Text {
                     text: qsTr("Restore Defaults")
                     horizontalAlignment: Text.AlignHCenter
                     color: "#d1d5db"
                 }

                 background: Rectangle {
                     opacity: .5
                     border.color: "#7d7d8e"
                     border.width: 1
                     radius: 10
                     color: "#343541"
                 }
                onClicked: {
                    settingsDialog.restoreDefaults()
                }
             }
        }
    }

    Button {
        id: drawerButton
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.leftMargin: 30
        width: 60
        height: 40
        z: 200
        padding: 15

        background: Item {
            anchors.fill: parent

            Rectangle {
                id: bar1
                color: "#7d7d8e"
                width: parent.width
                height: 8
                radius: 2
                antialiasing: true
            }

            Rectangle {
                id: bar2
                anchors.centerIn: parent
                color: "#7d7d8e"
                width: parent.width
                height: 8
                radius: 2
                antialiasing: true
            }

            Rectangle {
                id: bar3
                anchors.bottom: parent.bottom
                color: "#7d7d8e"
                width: parent.width
                height: 8
                radius: 2
                antialiasing: true
            }


        }
        onClicked: {
            drawer.visible = !drawer.visible
        }
    }

    Button {
        id: settingsButton
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.rightMargin: 30
        width: 60
        height: 40
        z: 200
        padding: 15

        background: Item {
            anchors.fill: parent
            Image {
                anchors.centerIn: parent
                width: 40
                height: 40
                source: "qrc:/gpt4all-chat/icons/settings.svg"
            }
        }

        onClicked: {
            settingsDialog.open()
        }
    }

    Dialog {
        id: copyMessage
        anchors.centerIn: parent
        modal: false
        opacity: 0.9
        Text {
            horizontalAlignment: Text.AlignJustify
            text: qsTr("Conversation copied to clipboard.")
            color: "#d1d5db"
        }
        background: Rectangle {
            anchors.fill: parent
            color: "#202123"
            border.width: 1
            border.color: "white"
            radius: 10
        }

        exit: Transition {
            NumberAnimation { duration: 500; property: "opacity"; from: 1.0; to: 0.0 }
        }
    }

    Button {
        id: copyButton
        anchors.right: settingsButton.left
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.rightMargin: 30
        width: 60
        height: 40
        z: 200
        padding: 15

        background: Item {
            anchors.fill: parent
            Image {
                anchors.centerIn: parent
                width: 40
                height: 40
                source: "qrc:/gpt4all-chat/icons/copy.svg"
            }
        }

        TextEdit{
            id: copyEdit
            visible: false
        }

        onClicked: {
            var conversation = "";
            for (var i = 0; i < chatModel.count; i++) {
                var item = chatModel.get(i)
                var string = item.name;
                if (item.currentResponse)
                    string += LLM.response
                else
                    string += chatModel.get(i).value
                string += "\n"
                conversation += string
            }
            copyEdit.text = conversation
            copyEdit.selectAll()
            copyEdit.copy()
            copyMessage.open()
            timer.start()
        }
        Timer {
            id: timer
            interval: 500; running: false; repeat: false
            onTriggered: copyMessage.close()
        }
    }

    Button {
        id: resetContextButton
        anchors.right: copyButton.left
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.rightMargin: 30
        width: 60
        height: 40
        z: 200
        padding: 15

        background: Item {
            anchors.fill: parent
            Image {
                anchors.centerIn: parent
                width: 40
                height: 40
                source: "qrc:/gpt4all-chat/icons/regenerate.svg"
            }
        }

        onClicked: {
            LLM.stopGenerating()
            LLM.resetContext()
            chatModel.clear()
        }
    }

    Dialog {
        id: checkForUpdatesError
        anchors.centerIn: parent
        modal: false
        opacity: 0.9
        Text {
            horizontalAlignment: Text.AlignJustify
            text: qsTr("ERROR: Update system could not find the MaintenanceTool used<br>
                   to check for updates!<br><br>
                   Did you install this application using the online installer? If so,<br>
                   the MaintenanceTool executable should be located one directory<br>
                   above where this application resides on your filesystem.<br><br>
                   If you can't start it manually, then I'm afraid you'll have to<br>
                   reinstall.")
            color: "#d1d5db"
        }
        background: Rectangle {
            anchors.fill: parent
            color: "#202123"
            border.width: 1
            border.color: "white"
            radius: 10
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
            color: "#202123"
        }

        Item {
            anchors.fill: parent
            anchors.margins: 30

            Label {
                id: conversationList
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                wrapMode: Text.WordWrap
                text: qsTr("Chat lists of specific conversations coming soon! Check back often for new features :)")
                color: "#d1d5db"
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
                color: "#d1d5db"
            }

            Label {
                id: nomicProps
                textFormat: Text.RichText
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: discordLink.bottom
                anchors.topMargin: 20
                wrapMode: Text.WordWrap
                text: qsTr("Thanks to <a href=\"https://home.nomic.ai\">nomic.ai</a> and the community for contributing so much great data and energy!")
                onLinkActivated: { Qt.openUrlExternally("https://home.nomic.ai") }
                color: "#d1d5db"
            }

            Button {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                padding: 15
                contentItem: Text {
                    text: qsTr("Check for updates...")
                    horizontalAlignment: Text.AlignHCenter
                    color: "#d1d5db"
                }

                background: Rectangle {
                    opacity: .5
                    border.color: "#7d7d8e"
                    border.width: 1
                    radius: 10
                    color: "#343541"
                }

                onClicked: {
                    if (!LLM.checkForUpdates())
                        checkForUpdatesError.open()
                }
            }
        }
    }

    Rectangle {
        id: conversation
        color: "#343541"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.top: header.bottom

        ScrollView {
            id: scrollView
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: textInput.top
            anchors.bottomMargin: 30
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn

            ListModel {
                id: chatModel
            }

            Rectangle {
                anchors.fill: parent
                color: "#444654"

                ListView {
                    id: listView
                    anchors.fill: parent
                    model: chatModel
                    delegate: TextArea {
                        text: currentResponse ? LLM.response : value
                        width: listView.width
                        color: "#d1d5db"
                        wrapMode: Text.WordWrap
                        focus: false
                        readOnly: true
                        padding: 20
                        font.pixelSize: 24
                        cursorVisible: currentResponse ? (LLM.response !== "" ? LLM.responseInProgress : false) : false
                        cursorPosition: text.length
                        background: Rectangle {
                            color: name === qsTr("Response: ") ? "#444654" : "#343541"
                        }

                        leftPadding: 100

                        BusyIndicator {
                            anchors.left: parent.left
                            anchors.leftMargin: 90
                            anchors.top: parent.top
                            anchors.topMargin: 5
                            visible: currentResponse && LLM.response === "" && LLM.responseInProgress
                            running: currentResponse && LLM.response === "" && LLM.responseInProgress
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.leftMargin: 20
                            anchors.topMargin: 20
                            width: 30
                            height: 30
                            radius: 5
                            color: name === qsTr("Response: ") ? "#10a37f" : "#ec86bf"

                            Text {
                                anchors.centerIn: parent
                                text: name === qsTr("Response: ") ? "R" : "P"
                                color: "white"
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
            Image {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 15
                source: LLM.responseInProgress ? "qrc:/gpt4all-chat/icons/stop_generating.svg" : "qrc:/gpt4all-chat/icons/regenerate.svg"
            }
            leftPadding: 50
            onClicked: {
                if (LLM.responseInProgress)
                    LLM.stopGenerating()
                else {
                    LLM.resetResponse()
                    if (chatModel.count) {
                        var listElement = chatModel.get(chatModel.count - 1)
                        if (listElement.name === qsTr("Response: ")) {
                            listElement.currentResponse = true
                            listElement.value = LLM.response
                            LLM.prompt(listElement.prompt, settingsDialog.promptTemplate, settingsDialog.maxLength,
                                       settingsDialog.topK, settingsDialog.topP, settingsDialog.temperature,
                                       settingsDialog.promptBatchSize)
                        }
                    }
                }
            }
            anchors.bottom: textInput.top
            anchors.horizontalCenter: textInput.horizontalCenter
            anchors.bottomMargin: 40
            padding: 15
            contentItem: Text {
                text: LLM.responseInProgress ? qsTr("Stop generating") : qsTr("Regenerate response")
                color: "#d1d5db"
            }
            background: Rectangle {
                opacity: .5
                border.color: "#7d7d8e"
                border.width: 1
                radius: 10
                color: "#343541"
            }
        }

        TextField {
            id: textInput
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 30
            color: "#dadadc"
            padding: 20
            enabled: LLM.isModelLoaded
            font.pixelSize: 24
            placeholderText: qsTr("Send a message...")
            placeholderTextColor: "#7d7d8e"
            background: Rectangle {
                color: "#40414f"
                radius: 10
            }
            onAccepted: {
                LLM.stopGenerating()

                if (chatModel.count) {
                    var listElement = chatModel.get(chatModel.count - 1)
                    listElement.currentResponse = false
                    listElement.value = LLM.response
                }

                var prompt = textInput.text + "\n"
                chatModel.append({"name": qsTr("Prompt: "), "currentResponse": false, "value": textInput.text})
                chatModel.append({"name": qsTr("Response: "), "currentResponse": true, "value": "", "prompt": prompt})
                LLM.resetResponse()
                LLM.prompt(prompt, settingsDialog.promptTemplate, settingsDialog.maxLength, settingsDialog.topK,
                           settingsDialog.topP, settingsDialog.temperature, settingsDialog.promptBatchSize)
                textInput.text = ""
            }

            Button {
                anchors.right: textInput.right
                anchors.verticalCenter: textInput.verticalCenter
                anchors.rightMargin: 15
                width: 30
                height: 30

                background: Image {
                    anchors.centerIn: parent
                    source: "qrc:/gpt4all-chat/icons/send_message.svg"
                }

                onClicked: {
                    textInput.accepted()
                }
            }
        }
    }
}
