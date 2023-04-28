import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import download
import network
import llm

Dialog {
    id: startupDialog
    anchors.centerIn: parent
    modal: false
    opacity: 0.9
    padding: 20
    width: 1024
    height: column.height + 40

    Theme {
        id: theme
    }

    Connections {
        target: startupDialog
        function onClosed() {
            if (!Network.usageStatsActive)
                Network.usageStatsActive = false // opt-out triggered
        }
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
                text: qsTr("Welcome!")
                color: theme.textColor
            }
        }

        ScrollView {
            clip: true
            height: 200
            width: 1024 - 40
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            TextArea {
                id: welcome
                wrapMode: Text.Wrap
                width: 1024 - 40
                padding: 20
                textFormat: TextEdit.MarkdownText
                text: qsTr("### Release notes\n")
                    + Download.releaseInfo.notes
                    + qsTr("### Contributors\n")
                    + Download.releaseInfo.contributors
                color: theme.textColor
                focus: false
                readOnly: true
                Accessible.role: Accessible.Paragraph
                Accessible.name: qsTr("Release notes")
                Accessible.description: qsTr("Release notes for this version")
                background: Rectangle {
                    color: theme.backgroundLight
                    radius: 10
                }
            }
        }

        ScrollView {
            clip: true
            height: 150
            width: 1024 - 40
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            TextArea {
                id: optInTerms
                wrapMode: Text.Wrap
                width: 1024 - 40
                padding: 20
                textFormat: TextEdit.MarkdownText
                text: qsTr(
"### Opt-ins for anonymous usage analytics and datalake
By enabling these features, you will be able to participate in the democratic process of training a
large language model by contributing data for future model improvements.

When a GPT4All model responds to you and you have opted-in, you can like/dislike its response. If you
dislike a response, you can suggest an alternative response. This data will be collected and aggregated
in the GPT4All Datalake.

NOTE: By turning on this feature, you will be sending your data to the GPT4All Open Source Datalake.
You should have no expectation of chat privacy when this feature is enabled. You should; however, have
an expectation of an optional attribution if you wish. Your chat data will be openly available for anyone
to download and will be used by Nomic AI to improve future GPT4All models. Nomic AI will retain all
attribution information attached to your data and you will be credited as a contributor to any GPT4All
model release that uses your data!")

                color: theme.textColor
                focus: false
                readOnly: true
                Accessible.role: Accessible.Paragraph
                Accessible.name: qsTr("Terms for opt-in")
                Accessible.description: qsTr("Describes what will happen when you opt-in")
                background: Rectangle {
                    color: theme.backgroundLight
                    radius: 10
                }
            }
        }

        GridLayout {
            columns: 2
            rowSpacing: 10
            columnSpacing: 10
            anchors.right: parent.right
            Label {
                id: optInStatistics
                text: "Opt-in to anonymous usage analytics used to improve GPT4All"
                Layout.row: 0
                Layout.column: 0
                Accessible.role: Accessible.Paragraph
                Accessible.name: qsTr("Opt-in for anonymous usage statistics")
                Accessible.description: qsTr("Label for opt-in")
            }

            CheckBox {
                id: optInStatisticsBox
                Layout.alignment: Qt.AlignVCenter
                Layout.row: 0
                Layout.column: 1
                property bool defaultChecked: Network.usageStatsActive
                checked: defaultChecked
                Accessible.role: Accessible.CheckBox
                Accessible.name: qsTr("Opt-in for anonymous usage statistics")
                Accessible.description: qsTr("Checkbox to allow opt-in for anonymous usage statistics")
                onClicked: {
                    Network.usageStatsActive = optInStatisticsBox.checked
                    if (optInNetworkBox.checked && optInStatisticsBox.checked)
                        startupDialog.close()
                }
            }
            Label {
                id: optInNetwork
                text: "Opt-in to anonymous sharing of chats to the GPT4All Datalake"
                Layout.row: 1
                Layout.column: 0
                Accessible.role: Accessible.Paragraph
                Accessible.name: qsTr("Opt-in for network")
                Accessible.description: qsTr("Checkbox to allow opt-in for network")
            }

            CheckBox {
                id: optInNetworkBox
                Layout.alignment: Qt.AlignVCenter
                Layout.row: 1
                Layout.column: 1
                property bool defaultChecked: Network.isActive
                checked: defaultChecked
                Accessible.role: Accessible.CheckBox
                Accessible.name: qsTr("Opt-in for network")
                Accessible.description: qsTr("Label for opt-in")
                onClicked: {
                    Network.isActive = optInNetworkBox.checked
                    if (optInNetworkBox.checked && optInStatisticsBox.checked)
                        startupDialog.close()
                }
            }
        }
    }

    background: Rectangle {
        anchors.fill: parent
        color: theme.backgroundDarkest
        border.width: 1
        border.color: theme.dialogBorder
        radius: 10
    }
}
