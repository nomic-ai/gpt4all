import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import chatlistmodel
import download
import llm
import modellist
import network
import mysettings

Rectangle {
    id: modelsView
    color: theme.viewBackground

    signal addModelViewRequested()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 30

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: ModelList.installedModels.count === 0
            ColumnLayout {
                id: noInstalledLabel
                anchors.centerIn: parent
                spacing: 0

                Text {
                    Layout.alignment: Qt.AlignCenter
                    text: qsTr("No Models Installed")
                    color: theme.mutedLightTextColor
                    font.pixelSize: theme.fontSizeBannerSmall
                }

                Text {
                    Layout.topMargin: 15
                    horizontalAlignment: Qt.AlignHCenter
                    color: theme.mutedLighterTextColor
                    text: qsTr("Install a model to get started using GPT4All")
                    font.pixelSize: theme.fontSizeLarge
                }
            }

            MyButton {
                anchors.top: noInstalledLabel.bottom
                anchors.topMargin: 50
                anchors.horizontalCenter: noInstalledLabel.horizontalCenter
                rightPadding: 60
                leftPadding: 60
                text: qsTr("\uFF0B Add Model")
                onClicked: {
                    addModelViewRequested()
                }
                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Shows the add model view")
            }
        }

        RowLayout {
            visible: ModelList.installedModels.count !== 0
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: 50

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft
                Layout.minimumWidth: 200
                spacing: 5

                Text {
                    id: welcome
                    text: qsTr("Installed Models")
                    font.pixelSize: theme.fontSizeBanner
                    color: theme.titleTextColor
                }

                Text {
                    text: qsTr("Locally installed chat models")
                    font.pixelSize: theme.fontSizeLarge
                    color: theme.titleInfoTextColor
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 0
            }

            MyButton {
                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                text: qsTr("\uFF0B Add Model")
                onClicked: {
                    addModelViewRequested()
                }
            }
        }

        ScrollView {
            id: scrollView
            visible: ModelList.installedModels.count !== 0
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: modelListView
                model: ModelList.installedModels
                boundsBehavior: Flickable.StopAtBounds
                spacing: 30

                delegate: Rectangle {
                    id: delegateItem
                    width: modelListView.width
                    height: childrenRect.height + 60
                    color: theme.conversationBackground
                    radius: 10
                    border.width: 1
                    border.color: theme.controlBorder

                    ColumnLayout {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 30

                        Text {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignLeft
                            text: name
                            elide: Text.ElideRight
                            color: theme.titleTextColor
                            font.pixelSize: theme.fontSizeLargest
                            font.bold: true
                            Accessible.role: Accessible.Paragraph
                            Accessible.name: qsTr("Model file")
                            Accessible.description: qsTr("Model file to be downloaded")
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: theme.dividerColor
                        }

                        Text {
                            id: descriptionText
                            text: description
                            font.pixelSize: theme.fontSizeLarge
                            Layout.row: 1
                            Layout.topMargin: 10
                            wrapMode: Text.WordWrap
                            textFormat: Text.StyledText
                            color: theme.textColor
                            linkColor: theme.textColor
                            Accessible.role: Accessible.Paragraph
                            Accessible.name: qsTr("Description")
                            Accessible.description: qsTr("File description")
                            onLinkActivated: function(link) { Qt.openUrlExternally(link); }
                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.NoButton // pass clicks to parent
                                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                            }
                        }

                        Item  {
                            Layout.minimumWidth: childrenRect.width
                            Layout.minimumHeight: childrenRect.height
                            Layout.bottomMargin: 10
                            RowLayout {
                                id: paramRow
                                anchors.centerIn: parent
                                ColumnLayout {
                                    Layout.topMargin: 10
                                    Layout.bottomMargin: 10
                                    Layout.leftMargin: 20
                                    Layout.rightMargin: 20
                                    Text {
                                        text: qsTr("File size")
                                        font.pixelSize: theme.fontSizeSmall
                                        color: theme.mutedDarkTextColor
                                    }
                                    Text {
                                        text: filesize
                                        color: theme.textColor
                                        font.pixelSize: theme.fontSizeSmall
                                        font.bold: true
                                    }
                                }
                                Rectangle {
                                    width: 1
                                    Layout.fillHeight: true
                                    color: theme.dividerColor
                                }
                                ColumnLayout {
                                    Layout.topMargin: 10
                                    Layout.bottomMargin: 10
                                    Layout.leftMargin: 20
                                    Layout.rightMargin: 20
                                    Text {
                                        text: qsTr("RAM required")
                                        font.pixelSize: theme.fontSizeSmall
                                        color: theme.mutedDarkTextColor
                                    }
                                    Text {
                                        text: ramrequired + qsTr(" GB")
                                        color: theme.textColor
                                        font.pixelSize: theme.fontSizeSmall
                                        font.bold: true
                                    }
                                }
                                Rectangle {
                                    width: 1
                                    Layout.fillHeight: true
                                    color: theme.dividerColor
                                }
                                ColumnLayout {
                                    Layout.topMargin: 10
                                    Layout.bottomMargin: 10
                                    Layout.leftMargin: 20
                                    Layout.rightMargin: 20
                                    Text {
                                        text: qsTr("Parameters")
                                        font.pixelSize: theme.fontSizeSmall
                                        color: theme.mutedDarkTextColor
                                    }
                                    Text {
                                        text: parameters
                                        color: theme.textColor
                                        font.pixelSize: theme.fontSizeSmall
                                        font.bold: true
                                    }
                                }
                                Rectangle {
                                    width: 1
                                    Layout.fillHeight: true
                                    color: theme.dividerColor
                                }
                                ColumnLayout {
                                    Layout.topMargin: 10
                                    Layout.bottomMargin: 10
                                    Layout.leftMargin: 20
                                    Layout.rightMargin: 20
                                    Text {
                                        text: qsTr("Quant")
                                        font.pixelSize: theme.fontSizeSmall
                                        color: theme.mutedDarkTextColor
                                    }
                                    Text {
                                        text: quant
                                        color: theme.textColor
                                        font.pixelSize: theme.fontSizeSmall
                                        font.bold: true
                                    }
                                }
                                Rectangle {
                                    width: 1
                                    Layout.fillHeight: true
                                    color: theme.dividerColor
                                }
                                ColumnLayout {
                                    Layout.topMargin: 10
                                    Layout.bottomMargin: 10
                                    Layout.leftMargin: 20
                                    Layout.rightMargin: 20
                                    Text {
                                        text: qsTr("Type")
                                        font.pixelSize: theme.fontSizeSmall
                                        color: theme.mutedDarkTextColor
                                    }
                                    Text {
                                        text: type
                                        color: theme.textColor
                                        font.pixelSize: theme.fontSizeSmall
                                        font.bold: true
                                    }
                                }
                            }

                            Rectangle {
                                color: "transparent"
                                anchors.fill: paramRow
                                border.color: theme.dividerColor
                                border.width: 1
                                radius: 10
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: theme.dividerColor
                        }

                        MySettingsButton {
                            text: qsTr("Remove")
                            textColor: theme.red500
                            onClicked: Download.removeModel(filename)
                            backgroundColor: "transparent"
                            backgroundColorHovered: theme.lighterButtonBackgroundHoveredRed
                        }
                    }
                }
            }
        }
    }
}
