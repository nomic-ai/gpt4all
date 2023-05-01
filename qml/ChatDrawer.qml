import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import llm
import download
import network

Drawer {
    id: chatDrawer
    modal: false
    opacity: 0.9

    Theme {
        id: theme
    }

    signal downloadClicked

    background: Rectangle {
        height: parent.height
        color: theme.backgroundDarkest
    }

    Item {
        anchors.fill: parent
        anchors.margins: 10

        Accessible.role: Accessible.Pane
        Accessible.name: qsTr("Drawer on the left of the application")
        Accessible.description: qsTr("Drawer that is revealed by pressing the hamburger button")

        Button {
            id: newChat
            anchors.left: parent.left
            anchors.right: parent.right
            padding: 15
            font.pixelSize: theme.fontSizeLarger
            background: Rectangle {
                color: theme.backgroundDarkest
                opacity: .5
                border.color: theme.backgroundLightest
                border.width: 1
                radius: 10
            }
            contentItem: Text {
                text: qsTr("New chat")
                horizontalAlignment: Text.AlignHCenter
                color: theme.textColor

                Accessible.role: Accessible.Button
                Accessible.name: text
                Accessible.description: qsTr("Use this to launch an external application that will check for updates to the installer")
            }
            onClicked: {
                LLM.chatListModel.addChat();
            }
        }

        ListView {
            id: conversationList
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 10
            anchors.top: newChat.bottom
            anchors.bottom: checkForUpdatesButton.top
            model: LLM.chatListModel

            delegate: Label {
                id: chatLabel
                anchors.left: parent.left
                anchors.right: parent.right
                color: theme.textColor
                padding: 15
                font.pixelSize: theme.fontSizeLarger
                text: name
                background: Rectangle {
                    color: index % 2 === 0 ? theme.backgroundLight : theme.backgroundLighter
                }
                horizontalAlignment: TextInput.AlignLeft
            }

            Accessible.role: Accessible.List
            Accessible.name: qsTr("List of chats")
            Accessible.description: qsTr("List of chats in the drawer dialog")
        }

        /*Label {
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
        }*/

        Button {
            id: checkForUpdatesButton
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: downloadButton.top
            anchors.bottomMargin: 10
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
                downloadClicked()            }
        }
    }
}