import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import download
import network
import llm

Dialog {
    id: newVerionDialog
    anchors.centerIn: parent
    modal: true
    opacity: 0.9
    width: contentItem.width
    height: contentItem.height
    padding: 20

    Theme {
        id: theme
    }

    background: Rectangle {
        anchors.fill: parent
        color: theme.backgroundDarkest
        border.width: 1
        border.color: theme.dialogBorder
        radius: 10
    }

    Item {
        id: contentItem
        width: childrenRect.width + 40
        height: childrenRect.height + 40

        Label {
            id: label
            anchors.top: parent.top
            anchors.left: parent.left
            topPadding: 20
            bottomPadding: 20
            text: qsTr("New version is available:")
            color: theme.textColor
        }

        Button {
            id: button
            anchors.left: label.right
            anchors.leftMargin: 10
            anchors.verticalCenter: label.verticalCenter
            padding: 20
            contentItem: Text {
                text: qsTr("Update")
                horizontalAlignment: Text.AlignHCenter
                color: theme.textColor

                Accessible.role: Accessible.Button
                Accessible.name: text
                Accessible.description: qsTr("Use this to launch an external application that will check for updates to the installer")
            }

            background: Rectangle {
                opacity: .5
                border.color: theme.backgroundLightest
                border.width: 1
                radius: 10
                color: theme.backgroundLight
            }

            onClicked: {
                if (!LLM.checkForUpdates())
                    checkForUpdatesError.open()
            }
        }
    }
}
