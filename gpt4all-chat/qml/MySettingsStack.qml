import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
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
        width: parent.width / 1.25
        z: 200
        Repeater {
            model: settingsStack.tabTitlesModel
            TabButton {
                id: tabButton
                contentItem: IconLabel {
                    color: theme.textColor
                    font.bold: tabButton.checked
                    text: model.title
                }
                background: Rectangle {
                    color: tabButton.checked ? theme.backgroundDarkest : theme.backgroundLight
                    Rectangle {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: tabButton.checked
                        color: theme.tabBorder
                    }
                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: !tabButton.checked
                        color: theme.tabBorder
                    }
                    Rectangle {
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        width: tabButton.checked
                        color: theme.tabBorder
                    }
                    Rectangle {
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        width: tabButton.checked
                        color: theme.tabBorder
                    }
                }
                Accessible.role: Accessible.Button
                Accessible.name: model.title
            }
        }
    }

    StackLayout {
        id: stackLayout
        anchors.top: settingsTabBar.bottom
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
