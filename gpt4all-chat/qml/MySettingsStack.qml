import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.folderlistmodel
import mysettings

Item {
    id: settingsStack

    Theme {
        id: theme
    }

    property alias title: titleLabel.text
    property ListModel tabTitlesModel: ListModel { }
    property list<Component> tabs: [ ]

    Label {
        id: titleLabel
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        color: theme.textColor
        padding: 10
        font.bold: true
        font.pixelSize: theme.fontSizeLarger
    }

    Rectangle {
        anchors.top: titleLabel.bottom
        anchors.leftMargin: 15
        anchors.rightMargin: 15
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: theme.tabBorder
    }

    TabBar {
        id: settingsTabBar
        anchors.top: titleLabel.bottom
        anchors.topMargin: 15
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width / 1.75
        z: 200
        visible: tabTitlesModel.count > 1
        background: Rectangle {
            color: "transparent"
        }
        Repeater {
            model: settingsStack.tabTitlesModel
            TabButton {
                id: tabButton
                padding: 10
                contentItem: IconLabel {
                    color: theme.textColor
                    font.pixelSize: theme.fontSizeLarge
                    font.bold: tabButton.checked
                    text: model.title
                }
                background: Rectangle {
                    color: "transparent"
                    border.width: 1
                    border.color: tabButton.checked ? theme.tabBorder : "transparent"
                    radius: 10
                }
                Accessible.role: Accessible.Button
                Accessible.name: model.title
            }
        }
    }

    Rectangle {
        id: dividerTabBar
        visible: tabTitlesModel.count > 1
        anchors.top: settingsTabBar.bottom
        anchors.topMargin: 15
        anchors.bottomMargin: 15
        anchors.leftMargin: 15
        anchors.rightMargin: 15
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: theme.tabBorder
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        radius: 10
        border.width: 1
        border.color: theme.tabBorder
    }

    FolderDialog {
        id: folderDialog
        title: qsTr("Please choose a directory")
    }

    function openFolderDialog(currentFolder, onAccepted) {
        folderDialog.currentFolder = currentFolder;
        folderDialog.accepted.connect(function() { onAccepted(folderDialog.currentFolder); });
        folderDialog.open();
    }

    StackLayout {
        id: stackLayout
        anchors.top: tabTitlesModel.count > 1 ? dividerTabBar.bottom : titleLabel.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        currentIndex: settingsTabBar.currentIndex

        Repeater {
            model: settingsStack.tabs
            delegate: Loader {
                id: loader
                sourceComponent: model.modelData
                onLoaded: {
                    settingsStack.tabTitlesModel.append({ "title": loader.item.title });
                    item.openFolderDialog = settingsStack.openFolderDialog;
                }
            }
        }
    }
}
