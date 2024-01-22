import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import download
import network
import llm
import mysettings

MyDialog {
    id: startupDialog
    anchors.centerIn: parent
    modal: true
    padding: 10
    width: 1024
    height: column.height + 20
    closePolicy: !optInStatisticsRadio.choiceMade || !optInNetworkRadio.choiceMade ? Popup.NoAutoClose : (Popup.CloseOnEscape | Popup.CloseOnPressOutside)

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
                text: qsTr("Welcome!")
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
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

        ScrollView {
            clip: true
            height: 150
            width: 1024 - 40
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            MyTextArea {
                id: optInTerms
                width: 1024 - 40
                textFormat: TextEdit.MarkdownText
                text: qsTr(
"### Opt-ins for anonymous usage analytics and datalake
By enabling these features, you will be able to participate in the democratic process of training a
large language model by contributing data for future model improvements.

When a GPT4All model responds to you and you have opted-in, your conversation will be sent to the GPT4All
Open Source Datalake. Additionally, you can like/dislike its response. If you dislike a response, you
can suggest an alternative response. This data will be collected and aggregated in the GPT4All Datalake.

NOTE: By turning on this feature, you will be sending your data to the GPT4All Open Source Datalake.
You should have no expectation of chat privacy when this feature is enabled. You should; however, have
an expectation of an optional attribution if you wish. Your chat data will be openly available for anyone
to download and will be used by Nomic AI to improve future GPT4All models. Nomic AI will retain all
attribution information attached to your data and you will be credited as a contributor to any GPT4All
model release that uses your data!")

                focus: false
                readOnly: true
                Accessible.role: Accessible.Paragraph
                Accessible.name: qsTr("Terms for opt-in")
                Accessible.description: qsTr("Describes what will happen when you opt-in")
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
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                Accessible.role: Accessible.Paragraph
                Accessible.name: qsTr("Opt-in for anonymous usage statistics")
            }

            ButtonGroup {
                buttons: optInStatisticsRadio.children
                onClicked: {
                    MySettings.networkUsageStatsActive = optInStatisticsRadio.checked
                    if (!optInStatisticsRadio.checked)
                        Network.sendOptOut();
                    if (optInNetworkRadio.choiceMade && optInStatisticsRadio.choiceMade)
                        startupDialog.close();
                }
            }

            RowLayout {
                id: optInStatisticsRadio
                Layout.alignment: Qt.AlignVCenter
                Layout.row: 0
                Layout.column: 1
                property alias checked: optInStatisticsRadioYes.checked
                property bool choiceMade: optInStatisticsRadioYes.checked || optInStatisticsRadioNo.checked

                RadioButton {
                    id: optInStatisticsRadioYes
                    checked: false
                    text: qsTr("Yes")
                    font.pixelSize: theme.fontSizeLarge
                    Accessible.role: Accessible.RadioButton
                    Accessible.name: qsTr("Opt-in for anonymous usage statistics")
                    Accessible.description: qsTr("Allow opt-in for anonymous usage statistics")

                    background: Rectangle {
                        color: "transparent"
                    }

                    indicator: Rectangle {
                        implicitWidth: 26
                        implicitHeight: 26
                        x: optInStatisticsRadioYes.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 13
                        border.color: theme.dialogBorder
                        color: "transparent"

                        Rectangle {
                            width: 14
                            height: 14
                            x: 6
                            y: 6
                            radius: 7
                            color: theme.textColor
                            visible: optInStatisticsRadioYes.checked
                        }
                    }

                    contentItem: Text {
                        text: optInStatisticsRadioYes.text
                        font: optInStatisticsRadioYes.font
                        opacity: enabled ? 1.0 : 0.3
                        color: theme.textColor
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: optInStatisticsRadioYes.indicator.width + optInStatisticsRadioYes.spacing
                    }
                }
                RadioButton {
                    id: optInStatisticsRadioNo
                    text: qsTr("No")
                    font.pixelSize: theme.fontSizeLarge
                    Accessible.role: Accessible.RadioButton
                    Accessible.name: qsTr("Opt-out for anonymous usage statistics")
                    Accessible.description: qsTr("Allow opt-out for anonymous usage statistics")

                    background: Rectangle {
                        color: "transparent"
                    }

                    indicator: Rectangle {
                        implicitWidth: 26
                        implicitHeight: 26
                        x: optInStatisticsRadioNo.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 13
                        border.color: theme.dialogBorder
                        color: "transparent"

                        Rectangle {
                            width: 14
                            height: 14
                            x: 6
                            y: 6
                            radius: 7
                            color: theme.textColor
                            visible: optInStatisticsRadioNo.checked
                        }
                    }

                    contentItem: Text {
                        text: optInStatisticsRadioNo.text
                        font: optInStatisticsRadioNo.font
                        opacity: enabled ? 1.0 : 0.3
                        color: theme.textColor
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: optInStatisticsRadioNo.indicator.width + optInStatisticsRadioNo.spacing
                    }
                }
            }

            Label {
                id: optInNetwork
                text: "Opt-in to anonymous sharing of chats to the GPT4All Datalake"
                Layout.row: 1
                Layout.column: 0
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                Accessible.role: Accessible.Paragraph
                Accessible.name: qsTr("Opt-in for network")
                Accessible.description: qsTr("Allow opt-in for network")
            }

            ButtonGroup {
                buttons: optInNetworkRadio.children
                onClicked: {
                    MySettings.networkIsActive = optInNetworkRadio.checked
                    if (optInNetworkRadio.choiceMade && optInStatisticsRadio.choiceMade)
                        startupDialog.close();
                }
            }

            RowLayout {
                id: optInNetworkRadio
                Layout.alignment: Qt.AlignVCenter
                Layout.row: 1
                Layout.column: 1
                property alias checked: optInNetworkRadioYes.checked
                property bool choiceMade: optInNetworkRadioYes.checked || optInNetworkRadioNo.checked

                RadioButton {
                    id: optInNetworkRadioYes
                    checked: false
                    text: qsTr("Yes")
                    font.pixelSize: theme.fontSizeLarge
                    Accessible.role: Accessible.RadioButton
                    Accessible.name: qsTr("Opt-in for network")
                    Accessible.description: qsTr("Allow opt-in anonymous sharing of chats to the GPT4All Datalake")

                    background: Rectangle {
                        color: "transparent"
                    }

                    indicator: Rectangle {
                        implicitWidth: 26
                        implicitHeight: 26
                        x: optInNetworkRadioYes.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 13
                        border.color: theme.dialogBorder
                        color: "transparent"

                        Rectangle {
                            width: 14
                            height: 14
                            x: 6
                            y: 6
                            radius: 7
                            color: theme.textColor
                            visible: optInNetworkRadioYes.checked
                        }
                    }

                    contentItem: Text {
                        text: optInNetworkRadioYes.text
                        font: optInNetworkRadioYes.font
                        opacity: enabled ? 1.0 : 0.3
                        color: theme.textColor
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: optInNetworkRadioYes.indicator.width + optInNetworkRadioYes.spacing
                    }
                }
                RadioButton {
                    id: optInNetworkRadioNo
                    text: qsTr("No")
                    font.pixelSize: theme.fontSizeLarge
                    Accessible.role: Accessible.RadioButton
                    Accessible.name: qsTr("Opt-out for network")
                    Accessible.description: qsTr("Allow opt-out anonymous sharing of chats to the GPT4All Datalake")

                    background: Rectangle {
                        color: "transparent"
                    }

                    indicator: Rectangle {
                        implicitWidth: 26
                        implicitHeight: 26
                        x: optInNetworkRadioNo.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 13
                        border.color: theme.dialogBorder
                        color: "transparent"

                        Rectangle {
                            width: 14
                            height: 14
                            x: 6
                            y: 6
                            radius: 7
                            color: theme.textColor
                            visible: optInNetworkRadioNo.checked
                        }
                    }

                    contentItem: Text {
                        text: optInNetworkRadioNo.text
                        font: optInNetworkRadioNo.font
                        opacity: enabled ? 1.0 : 0.3
                        color: theme.textColor
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: optInNetworkRadioNo.indicator.width + optInNetworkRadioNo.spacing
                    }
                }
            }
        }
    }
}
