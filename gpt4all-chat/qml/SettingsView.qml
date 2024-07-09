import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Qt.labs.folderlistmodel
import download
import modellist
import network
import llm
import mysettings

Rectangle {
    id: settingsDialog
    color: theme.viewBackground

    property alias pageToDisplay: listView.currentIndex

    Item {
        Accessible.role: Accessible.Dialog
        Accessible.name: qsTr("Settings")
        Accessible.description: qsTr("Contains various application settings")
    }

    ListModel {
        id: stacksModel
        ListElement {
            title: qsTr("Application")
        }
        ListElement {
            title: qsTr("Model")
        }
        ListElement {
            title: qsTr("LocalDocs")
        }
    }

    ColumnLayout {
        id: mainArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 30
        spacing: 50

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
                    text: qsTr("Settings")
                    font.pixelSize: theme.fontSizeBanner
                    color: theme.titleTextColor
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 0
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                id: stackList
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                width: 220
                color: theme.viewBackground
                radius: 10

                ScrollView {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.topMargin: 10
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    clip: true

                    ListView {
                        id: listView
                        anchors.fill: parent
                        model: stacksModel

                        delegate: Rectangle {
                            id: item
                            width: listView.width
                            height: titleLabel.height + 10
                            color: "transparent"

                            MyButton {
                                id: titleLabel
                                backgroundColor: index === listView.currentIndex ? theme.selectedBackground : theme.viewBackground
                                backgroundColorHovered: backgroundColor
                                borderColor: "transparent"
                                borderWidth: 0
                                textColor: theme.titleTextColor
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: 10
                                font.bold: index === listView.currentIndex
                                text: title
                                textAlignment: Qt.AlignLeft
                                font.pixelSize: theme.fontSizeLarge
                                onClicked: {
                                    listView.currentIndex = index
                                }
                            }
                        }
                    }
                }
            }

            StackLayout {
                id: stackLayout
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: stackList.right
                anchors.right: parent.right
                currentIndex: listView.currentIndex

                MySettingsStack {
                    tabs: [
                        Component { ApplicationSettings { } }
                    ]
                }

                MySettingsStack {
                    tabs: [
                        Component { ModelSettings { } }
                    ]
                }

                MySettingsStack {
                    tabs: [
                        Component { LocalDocsSettings { } }
                    ]
                }
            }
        }
    }
}
