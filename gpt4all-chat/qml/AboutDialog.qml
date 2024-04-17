import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import download
import network
import llm

MyDialog {
    id: abpoutDialog
    anchors.centerIn: parent
    modal: false
    padding: 20
    width: 1024
    height: column.height + 40

    Theme {
        id: theme
    }

    Column {
        id: column
        spacing: 20
        Item {
            width: childrenRect.width
            height: childrenRect.height
            Image {
                id: img
                anchors.top: parent.top
                anchors.left: parent.left
                width: 60
                height: 60
                source: "qrc:/gpt4all/icons/logo.svg"
            }
            Text {
                anchors.left: img.right
                anchors.leftMargin: 30
                anchors.verticalCenter: img.verticalCenter
                text: qsTr("About GPT4All")
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                font.bold: true
            }
        }

        ScrollView {
            clip: true
            height: 200
            width: 1024 - 40
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            MyTextArea {
                id: welcome
                width: 1024 - 40
                textFormat: TextEdit.MarkdownText
                text: qsTr("### Release notes\n")
                    + Download.releaseInfo.notes
                    + qsTr("### Contributors\n")
                    + Download.releaseInfo.contributors
                focus: false
                readOnly: true
                Accessible.role: Accessible.Paragraph
                Accessible.name: qsTr("Release notes")
                Accessible.description: qsTr("Release notes for this version")
            }
        }

        MySettingsLabel {
            id: discordLink
            width: parent.width
            textFormat: Text.StyledText
            wrapMode: Text.WordWrap
            text: qsTr("Check out our discord channel <a href=\"https://discord.gg/4M2QFmTt2k\">https://discord.gg/4M2QFmTt2k</a>")
            font.pixelSize: theme.fontSizeLarge
            onLinkActivated: { Qt.openUrlExternally("https://discord.gg/4M2QFmTt2k") }
            color: theme.textColor
            linkColor: theme.linkColor

            Accessible.role: Accessible.Link
            Accessible.name: qsTr("Discord link")
        }

        MySettingsLabel {
            id: nomicProps
            width: parent.width
            textFormat: Text.StyledText
            wrapMode: Text.WordWrap
            text: qsTr("Thank you to <a href=\"https://home.nomic.ai\">Nomic AI</a> and the community for contributing so much great data, code, ideas, and energy to the growing open source AI ecosystem!")
            font.pixelSize: theme.fontSizeLarge
            onLinkActivated: { Qt.openUrlExternally("https://home.nomic.ai") }
            color: theme.textColor
            linkColor: theme.linkColor

            Accessible.role: Accessible.Paragraph
            Accessible.name: qsTr("Thank you blurb")
            Accessible.description: qsTr("Contains embedded link to https://home.nomic.ai")
        }
    }
}
