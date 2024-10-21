import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.folderlistmodel
import mysettings

Item {
    id: settingsStack

    Theme {
        id: theme
    }

    property ListModel tabTitlesModel: ListModel { }
    property list<Component> tabs: [ ]

    TabBar {
        id: settingsTabBar
        anchors.top: parent.top
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
        color: theme.settingsDivider
    }

    StackLayout {
        id: stackLayout
        anchors.top: tabTitlesModel.count > 1 ? dividerTabBar.bottom : parent.top
        anchors.topMargin: 5
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
                }
            }
        }
    }
}
