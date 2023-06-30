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

Dialog {
    id: settingsDialog
    modal: true
    opacity: 0.9
    padding: 20
    bottomPadding: 30
    background: Rectangle {
        anchors.fill: parent
        color: theme.backgroundDarkest
        border.width: 1
        border.color: theme.dialogBorder
        radius: 10
    }

    onOpened: {
        Network.sendSettingsDialog();
    }

    Theme {
        id: theme
    }

    Item {
        Accessible.role: Accessible.Dialog
        Accessible.name: qsTr("Settings dialog")
        Accessible.description: qsTr("Dialog containing various application settings")
    }
    TabBar {
        id: settingsTabBar
        width: parent.width / 1.25
        z: 200

        TabButton {
            id: genSettingsButton
            contentItem: IconLabel {
                color: theme.textColor
                font.bold: genSettingsButton.checked
                text: qsTr("Generation")
            }
            background: Rectangle {
                color: genSettingsButton.checked ? theme.backgroundDarkest : theme.backgroundLight
                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: genSettingsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: !genSettingsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    width: genSettingsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    width: genSettingsButton.checked
                    color: theme.tabBorder
                }
            }
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Generation settings")
            Accessible.description: qsTr("Settings related to how the model generates text")
        }

        TabButton {
            id: appSettingsButton
            contentItem: IconLabel {
                color: theme.textColor
                font.bold: appSettingsButton.checked
                text: qsTr("Application")
            }
            background: Rectangle {
                color: appSettingsButton.checked ? theme.backgroundDarkest : theme.backgroundLight
                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: appSettingsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: !appSettingsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    width: appSettingsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    width: appSettingsButton.checked
                    color: theme.tabBorder
                }
            }
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Application settings")
            Accessible.description: qsTr("Settings related to general behavior of the application")
        }

        TabButton {
            id: localDocsButton
            contentItem: IconLabel {
                color: theme.textColor
                font.bold: localDocsButton.checked
                text: qsTr("LocalDocs Plugin (BETA)")
            }
            background: Rectangle {
                color: localDocsButton.checked ? theme.backgroundDarkest : theme.backgroundLight
                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: localDocsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: !localDocsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    width: localDocsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    width: localDocsButton.checked
                    color: theme.tabBorder
                }
            }
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("LocalDocs settings")
            Accessible.description: qsTr("Settings related to localdocs plugin")
        }
    }

    StackLayout {
        anchors.top: settingsTabBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        currentIndex: settingsTabBar.currentIndex

        GenerationSettings {
        }

        ApplicationSettings {
        }

        LocalDocsSettings {
        }
    }
}
