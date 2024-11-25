import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: root
    property string title: ""
    property Item contentItem: null
    property bool showRestoreDefaultsButton: true
    signal restoreDefaults

    onContentItemChanged: function() {
        if (contentItem) {
            contentItem.parent = contentInner;
            contentItem.anchors.left = contentInner.left;
            contentItem.anchors.right = contentInner.right;
        }
    }

    ConfirmationDialog {
        id: restoreDefaultsDialog
        dialogTitle: qsTr("Restore defaults?")
        description: qsTr("This page of settings will be reset to the defaults.")
        onAccepted: root.restoreDefaults()
    }

    ScrollView {
        id: scrollView
        width: parent.width
        height: parent.height
        topPadding: 15
        leftPadding: 5
        contentWidth: availableWidth
        contentHeight: innerColumn.height
        ScrollBar.vertical: ScrollBar {
            parent: scrollView.parent
            anchors.top: scrollView.top
            anchors.left: scrollView.right
            anchors.bottom: scrollView.bottom
        }

        Theme {
            id: theme
        }

        ColumnLayout {
            id: innerColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 15
            spacing: 10
            Column {
                id: contentInner
                Layout.fillWidth: true
                Layout.maximumWidth: parent.width
            }

            Item {
                Layout.fillWidth: true
                Layout.topMargin: 20
                height: restoreDefaultsButton.height
                MySettingsButton {
                    id: restoreDefaultsButton
                    anchors.left: parent.left
                    visible: showRestoreDefaultsButton
                    width: implicitWidth
                    text: qsTr("Restore Defaults")
                    font.pixelSize: theme.fontSizeLarge
                    Accessible.role: Accessible.Button
                    Accessible.name: text
                    Accessible.description: qsTr("Restores settings dialog to a default state")
                    onClicked: restoreDefaultsDialog.open()
                }
            }
        }
    }
}
