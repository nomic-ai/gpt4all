import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
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

            TextField {
                id: modelNameField
                color: "#d1d5db"
                padding: 20
                font.pixelSize: 24
                text: "GPT4ALL Model: " + LLM.modelName
                background: Rectangle {
                    color: "#202123"
                }
                focus: false
                horizontalAlignment: TextInput.AlignHCenter
            }
        }

        BusyIndicator {
            anchors.centerIn: parent
            visible: !LLM.isModelLoaded
            running: !LLM.isModelLoaded
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
        id: copyButton
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
                            LLM.prompt(listElement.prompt)
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
                LLM.prompt(prompt)
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
