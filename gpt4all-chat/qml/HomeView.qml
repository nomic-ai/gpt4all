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

Rectangle {
    id: homeView

    Theme {
        id: theme
    }

    color: theme.viewBackground
    signal chatViewRequested()
    signal localDocsViewRequested()
    signal settingsViewRequested(int page)
    signal addModelViewRequested()
    property bool shouldShowFirstStart: false

    ColumnLayout {
        id: mainArea
        anchors.fill: parent
        anchors.margins: 30
        spacing: 30

        ColumnLayout {
            Layout.fillWidth: true
            Layout.maximumWidth: 1530
            Layout.alignment: Qt.AlignCenter
            spacing: 30

            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 5

                Text {
                    id: welcome
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Welcome to GPT4All")
                    font.pixelSize: theme.fontSizeBanner
                    color: theme.titleTextColor
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("the privacy-first LLM chat application")
                    font.pixelSize: theme.fontSizeLarge
                    color: theme.titleInfoTextColor
                }
            }

            MyButton {
                id: startChat
                visible: shouldShowFirstStart
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Start chatting")
                onClicked: {
                    chatViewRequested()
                }
            }

            // FIXME! We need a spot for dynamic content to be loaded here

            RowLayout {
                spacing: 15
                visible: !startChat.visible
                Layout.alignment: Qt.AlignHCenter

                MyWelcomeButton {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 500
                    Layout.preferredHeight: 150
                    text: qsTr("Start Chatting")
                    description: qsTr("Chat with any LLM")
                    imageSource: "qrc:/gpt4all/icons/chat.svg"
                    onClicked: {
                        chatViewRequested()
                    }
                }
                MyWelcomeButton {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 500
                    Layout.preferredHeight: 150
                    text: qsTr("LocalDocs")
                    description: qsTr("Chat with your local files")
                    imageSource: "qrc:/gpt4all/icons/db.svg"
                    onClicked: {
                        localDocsViewRequested()
                    }
                }
                MyWelcomeButton {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 500
                    Layout.preferredHeight: 150
                    text: qsTr("Find Models")
                    description: qsTr("Explore and download models")
                    imageSource: "qrc:/gpt4all/icons/models.svg"
                    onClicked: {
                        addModelViewRequested()
                    }
                }
            }

            Item {
                visible: !startChat.visible
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 150
                Layout.maximumHeight: textArea.height

                Rectangle {
                    id: roundedFrame
                    anchors.fill: parent
                    z: 299
                    radius: 10
                    border.width: 1
                    border.color: theme.controlBorder
                    color: "transparent"
                    clip: true
                }

                Item {
                    anchors.fill: parent
                    layer.enabled: true
                    layer.effect: OpacityMask {
                        maskSource: Rectangle {
                            width: roundedFrame.width
                            height: roundedFrame.height
                            radius: 10
                        }
                    }

                    RowLayout {
                        spacing: 0
                        anchors.fill: parent

                        Rectangle {
                            color: "transparent"
                            width: 82
                            height: 100
                            Image {
                                id: changelogImg
                                anchors.centerIn: parent
                                sourceSize: Qt.size(40, 40)
                                mipmap: true
                                visible: false
                                source: "qrc:/gpt4all/icons/changelog.svg"
                            }

                            ColorOverlay {
                                anchors.fill: changelogImg
                                source: changelogImg
                                color: theme.styledTextColor
                            }
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            ScrollBar.vertical.policy: ScrollBar.AsNeeded
                            ScrollBar.horizontal.policy: ScrollBar.AsNeeded

                            TextArea {
                                id: textArea
                                padding: 10
                                color: theme.styledTextColor
                                font.pixelSize: theme.fontSizeLarge
                                textFormat: TextEdit.MarkdownText
                                text: "### "
                                      + qsTr("Release notes for version ")
                                      + Download.releaseInfo.version
                                      + Download.releaseInfo.notes
                                      + "<br />\n### "
                                      + qsTr("Contributors")
                                      + Download.releaseInfo.contributors
                                background: Rectangle {
                                    implicitWidth: 150
                                    color: theme.conversationBackground
                                }
                                focus: false
                                readOnly: true
                                selectByKeyboard: false
                                selectByMouse: false
                                Accessible.role: Accessible.Paragraph
                                Accessible.name: qsTr("Release notes")
                                Accessible.description: qsTr("Release notes for this version")
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            id: linkBar
            Layout.alignment: Qt.AlignBottom
            Layout.fillWidth: true
            border.width: 1
            border.color: theme.dividerColor
            radius: 6
            z: 200
            height: 30
            color: theme.conversationBackground

            RowLayout {
                anchors.fill: parent
                spacing: 0
                RowLayout {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                    spacing: 4

                    MyFancyLink {
                        text: qsTr("Changelog")
                        imageSource: "qrc:/gpt4all/icons/changelog.svg"
                        onClicked: { Qt.openUrlExternally("!fixme!") }
                    }

                    MyFancyLink {
                        text: qsTr("Discord")
                        imageSource: "qrc:/gpt4all/icons/discord.svg"
                        onClicked: { Qt.openUrlExternally("https://discord.gg/4M2QFmTt2k") }
                    }

                    MyFancyLink {
                        text: qsTr("X (Twitter)")
                        imageSource: "qrc:/gpt4all/icons/twitter.svg"
                        onClicked: { Qt.openUrlExternally("https://twitter.com/nomic_ai") }
                    }

                    MyFancyLink {
                        text: qsTr("Github")
                        imageSource: "qrc:/gpt4all/icons/github.svg"
                        onClicked: { Qt.openUrlExternally("https://github.com/nomic-ai/gpt4all") }
                    }
                }

                RowLayout {
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    spacing: 40

                    MyFancyLink {
                        text: qsTr("GPT4All.io")
                        imageSource: "qrc:/gpt4all/icons/globe.svg"
                        onClicked: { Qt.openUrlExternally("https://gpt4all.io") }
                        rightPadding: 15
                    }
                }
            }
        }
    }
}

// FIXME_BLOCKER We have no 'about' button
