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
    signal downloadViewRequested(bool showEmbeddingModels)
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
                    color: theme.mutedTextColor
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
                        downloadViewRequested(false /*show embedding models*/)
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
                            width: 100
                            height: 100
                            Image {
                                id: changelogImg
                                anchors.centerIn: parent
                                sourceSize.width: 64
                                sourceSize.height: 64
                                mipmap: true
                                visible: false
                                source: "qrc:/gpt4all/icons/changelog.svg"
                            }

                            ColorOverlay {
                                anchors.fill: changelogImg
                                source: changelogImg
                                color: theme.changeLogColor
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
                                padding: 30
                                color: theme.changeLogColor
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
            height: 50
            color: theme.conversationBackground

            RowLayout {
                anchors.fill: parent
                spacing: 0
                RowLayout {
                    Layout.alignment: Qt.AlignLeft
                    spacing: 40

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
                    Layout.alignment: Qt.AlignRight
                    spacing: 40

                    MyFancyLink {
                        text: qsTr("GPT4All.io")
                        imageSource: "qrc:/gpt4all/icons/globe.svg"
                        onClicked: { Qt.openUrlExternally("https://gpt4all.io") }
                    }
                }
            }
        }
    }
}


//            MyToolButton {
//                id: networkButton
//                backgroundColor: theme.iconBackgroundViewBar
//                backgroundColorHovered: toggled ? backgroundColor : theme.iconBackgroundViewBarHovered
//                toggledColor: theme.iconBackgroundViewBar
//                Layout.preferredWidth: 40
//                Layout.preferredHeight: 40
//                scale: 1.2
//                toggled: MySettings.networkIsActive
//                source: "qrc:/gpt4all/icons/network.svg"
//                Accessible.name: qsTr("Network")
//                Accessible.description: qsTr("Reveals a dialogue where you can opt-in for sharing data over network")

//                onClicked: {
//                    if (MySettings.networkIsActive) {
//                        MySettings.networkIsActive = false
//                        Network.sendNetworkToggled(false);
//                    } else
//                        networkDialog.open()
//                }
//            }

//            MyToolButton {
//                id: infoButton
//                backgroundColor: theme.iconBackgroundViewBar
//                backgroundColorHovered: theme.iconBackgroundViewBarHovered
//                Layout.preferredWidth: 40
//                Layout.preferredHeight: 40
//                scale: 1.2
//                source: "qrc:/gpt4all/icons/info.svg"
//                Accessible.name: qsTr("About")
//                Accessible.description: qsTr("Reveals an about dialog")
//                onClicked: {
//                    aboutDialog.open()
//                }
//            }