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
    color: theme.containerBackground

    signal addModelViewRequested()

    // FIXME_BLOCKER If we don't have any installed models we should present a button in the middle
    // to add a new model as per design

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 30

        RowLayout {
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
                    text: qsTr("Locally installed large language models")
                    font.pixelSize: theme.fontSizeLarge
                    color: theme.mutedTextColor
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
                            onLinkActivated: Qt.openUrlExternally(link)
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
                                        font.pixelSize: theme.fontSizeSmaller
                                        color: theme.logoColor
                                    }
                                    Text {
                                        text: filesize
                                        font.pixelSize: theme.fontSizeSmaller
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
                                        font.pixelSize: theme.fontSizeSmaller
                                        color: theme.logoColor
                                    }
                                    Text {
                                        text: ramrequired + " GB" // FIXME_BLOCKER This can be done in C++ to add the "GB" I think?
                                        font.pixelSize: theme.fontSizeSmaller
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
                                        text: qsTr("Paremeters")
                                        font.pixelSize: theme.fontSizeSmaller
                                        color: theme.logoColor
                                    }
                                    Text {
                                        text: parameters
                                        font.pixelSize: theme.fontSizeSmaller
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
                                        font.pixelSize: theme.fontSizeSmaller
                                        color: theme.logoColor
                                    }
                                    Text {
                                        text: quant
                                        font.pixelSize: theme.fontSizeSmaller
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
                                        text: qsTr("Tyoe")
                                        font.pixelSize: theme.fontSizeSmaller
                                        color: theme.logoColor
                                    }
                                    Text {
                                        text: type
                                        font.pixelSize: theme.fontSizeSmaller
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

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 30
                            Layout.leftMargin: 15
                            Layout.topMargin: 15
                            Text {
                                text: qsTr("Remove")
                                elide: Text.ElideRight
                                color: theme.red500
                                font.bold: true
                                font.pixelSize: theme.fontSizeSmall
                                TapHandler {
                                    onTapped: {
                                        Download.removeModel(filename);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
