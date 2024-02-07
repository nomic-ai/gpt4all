import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import chatlistmodel
import llm
import download
import network
import mysettings

Drawer {
    id: chatDrawer
    modal: false

    Theme {
        id: theme
    }

    signal downloadClicked
    signal aboutClicked

    background: Rectangle {
        height: parent.height
        color: theme.containerBackground
    }

    Item {
        anchors.fill: parent
        anchors.margins: 10

        Accessible.role: Accessible.Pane
        Accessible.name: qsTr("Drawer")
        Accessible.description: qsTr("Main navigation drawer")

        MyButton {
            id: newChat
            anchors.left: parent.left
            anchors.right: parent.right
            font.pixelSize: theme.fontSizeLarger
            topPadding: 20
            bottomPadding: 20
            text: qsTr("\uFF0B New chat")
            Accessible.description: qsTr("Create a new chat")
            onClicked: {
                ChatListModel.addChat();
                Network.sendNewChat(ChatListModel.count)
            }
        }

        ScrollView {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: -10
            anchors.topMargin: 10
            anchors.top: newChat.bottom
            anchors.bottom: checkForUpdatesButton.top
            anchors.bottomMargin: 10
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff
            clip: true

            ListView {
                id: conversationList
                anchors.fill: parent
                anchors.rightMargin: 10
                model: ChatListModel
                ScrollBar.vertical: ScrollBar {
                    parent: conversationList.parent
                    anchors.top: conversationList.top
                    anchors.left: conversationList.right
                    anchors.bottom: conversationList.bottom
                }

                delegate: Rectangle {
                    id: chatRectangle
                    width: conversationList.width
                    height: chatName.height
                    property bool isCurrent: ChatListModel.currentChat === ChatListModel.get(index)
                    property bool isServer: ChatListModel.get(index) && ChatListModel.get(index).isServer
                    property bool trashQuestionDisplayed: false
                    visible: !isServer || MySettings.serverChat
                    z: isCurrent ? 199 : 1
                    color: index % 2 === 0 ? theme.darkContrast : theme.lightContrast
                    border.width: isCurrent
                    border.color: chatName.readOnly ? theme.assistantColor : theme.userColor
                    TextField {
                        id: chatName
                        anchors.left: parent.left
                        anchors.right: buttons.left
                        color: theme.textColor
                        padding: 15
                        focus: false
                        readOnly: true
                        wrapMode: Text.NoWrap
                        hoverEnabled: false // Disable hover events on the TextArea
                        selectByMouse: false // Disable text selection in the TextArea
                        font.pixelSize: theme.fontSizeLarger
                        text: readOnly ? metrics.elidedText : name
                        horizontalAlignment: TextInput.AlignLeft
                        opacity: trashQuestionDisplayed ? 0.5 : 1.0
                        TextMetrics {
                            id: metrics
                            font: chatName.font
                            text: name
                            elide: Text.ElideRight
                            elideWidth: chatName.width - 40
                        }
                        background: Rectangle {
                            color: "transparent"
                        }
                        onEditingFinished: {
                            // Work around a bug in qml where we're losing focus when the whole window
                            // goes out of focus even though this textfield should be marked as not
                            // having focus
                            if (chatName.readOnly)
                                return;
                            changeName();
                            Network.sendRenameChat()
                        }
                        function changeName() {
                            ChatListModel.get(index).name = chatName.text
                            chatName.focus = false
                            chatName.readOnly = true
                            chatName.selectByMouse = false
                        }
                        TapHandler {
                            onTapped: {
                                if (isCurrent)
                                    return;
                                ChatListModel.currentChat = ChatListModel.get(index);
                            }
                        }
                        Accessible.role: Accessible.Button
                        Accessible.name: text
                        Accessible.description: qsTr("Select the current chat or edit the chat when in edit mode")
                    }
                    Row {
                        id: buttons
                        anchors.verticalCenter: chatName.verticalCenter
                        anchors.right: chatRectangle.right
                        anchors.rightMargin: 10
                        spacing: 10
                        MyToolButton {
                            id: editButton
                            width: 30
                            height: 30
                            visible: isCurrent && !isServer
                            opacity: trashQuestionDisplayed ? 0.5 : 1.0
                            source: "qrc:/gpt4all/icons/edit.svg"
                            onClicked: {
                                chatName.focus = true
                                chatName.readOnly = false
                                chatName.selectByMouse = true
                            }
                            Accessible.name: qsTr("Edit chat name")
                        }
                        MyToolButton {
                            id: trashButton
                            width: 30
                            height: 30
                            visible: isCurrent && !isServer
                            source: "qrc:/gpt4all/icons/trash.svg"
                            onClicked: {
                                trashQuestionDisplayed = true
                                timer.start()
                            }
                            Accessible.name: qsTr("Delete chat")
                        }
                    }
                    Rectangle {
                        id: trashSureQuestion
                        anchors.top: buttons.bottom
                        anchors.topMargin: 10
                        anchors.right: buttons.right
                        width: childrenRect.width
                        height: childrenRect.height
                        color: chatRectangle.color
                        visible: isCurrent && trashQuestionDisplayed
                        opacity: 1.0
                        radius: 10
                        z: 200
                        Row {
                            spacing: 10
                            Button {
                                id: checkMark
                                width: 30
                                height: 30
                                contentItem: Text {
                                    color: theme.textErrorColor
                                    text: "\u2713"
                                    font.pixelSize: theme.fontSizeLarger
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    width: 30
                                    height: 30
                                    color: "transparent"
                                }
                                onClicked: {
                                    ChatListModel.removeChat(ChatListModel.get(index))
                                    Network.sendRemoveChat()
                                }
                                Accessible.role: Accessible.Button
                                Accessible.name: qsTr("Confirm chat deletion")
                            }
                            Button {
                                id: cancel
                                width: 30
                                height: 30
                                contentItem: Text {
                                    color: theme.textColor
                                    text: "\u2715"
                                    font.pixelSize: theme.fontSizeLarger
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    width: 30
                                    height: 30
                                    color: "transparent"
                                }
                                onClicked: {
                                    trashQuestionDisplayed = false
                                }
                                Accessible.role: Accessible.Button
                                Accessible.name: qsTr("Cancel chat deletion")
                            }
                        }
                    }
                    Timer {
                        id: timer
                        interval: 3000; running: false; repeat: false
                        onTriggered: trashQuestionDisplayed = false
                    }
                }

                Accessible.role: Accessible.List
                Accessible.name: qsTr("List of chats")
                Accessible.description: qsTr("List of chats in the drawer dialog")
            }
        }

        MyButton {
            id: checkForUpdatesButton
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: downloadButton.top
            anchors.bottomMargin: 10
            text: qsTr("Updates")
            font.pixelSize: theme.fontSizeLarge
            Accessible.description: qsTr("Launch an external application that will check for updates to the installer")
            onClicked: {
                if (!LLM.checkForUpdates())
                    checkForUpdatesError.open()
            }
        }

        MyButton {
            id: downloadButton
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: aboutButton.top
            anchors.bottomMargin: 10
            text: qsTr("Downloads")
            Accessible.description: qsTr("Launch a dialog to download new models")
            onClicked: {
                downloadClicked()
            }
        }

        MyButton {
            id: aboutButton
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            text: qsTr("About")
            Accessible.description: qsTr("Launch a dialog to show the about page")
            onClicked: {
                aboutClicked()
            }
        }
    }
}
