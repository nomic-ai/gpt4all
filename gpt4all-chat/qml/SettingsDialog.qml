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

MyDialog {
    id: settingsDialog
    modal: true
    padding: 20
    onOpened: {
        Network.sendSettingsDialog();
    }

    signal downloadClicked
    property alias pageToDisplay: listView.currentIndex

    Item {
        Accessible.role: Accessible.Dialog
        Accessible.name: qsTr("Settings")
        Accessible.description: qsTr("Contains various application settings")
    }

    ListModel {
        id: stacksModel
        ListElement {
            title: qsTr("Models")
        }
        ListElement {
            title: qsTr("Application")
        }
        ListElement {
            title: qsTr("LocalDocs")
        }
    }

    ScrollView {
        id: stackList
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 200
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        clip: true

        ListView {
            id: listView
            anchors.fill: parent
            anchors.rightMargin: 10
            model: stacksModel

            delegate: Rectangle {
                id: item
                width: listView.width
                height: titleLabel.height + 25
                color: "transparent"
                border.color: theme.backgroundLighter
                border.width: index == listView.currentIndex ? 1 : 0
                radius: 10

                Text {
                    id: titleLabel
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.margins: 20
                    font.bold: index == listView.currentIndex
                    text: title
                    font.pixelSize: theme.fontSizeLarge
                    elide: Text.ElideRight
                    color: theme.textColor
                    width: 200
                }

                TapHandler {
                    onTapped: {
                        listView.currentIndex = index
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
            title: qsTr("Model/Character Settings")
            tabs: [
                Component { ModelSettings { } }
            ]
        }

        MySettingsStack {
            title: qsTr("Application General Settings")
            tabs: [
                Component { ApplicationSettings { } }
            ]
        }

        MySettingsStack {
            title: qsTr("Local Document Collections")
            tabs: [
                Component {
                    LocalDocsSettings {
                        id: localDocsSettings
                        Component.onCompleted: {
                             localDocsSettings.downloadClicked.connect(settingsDialog.downloadClicked);
                        }
                    }
                }
            ]
        }
    }
}
