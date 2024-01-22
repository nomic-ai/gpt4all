import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    id: root
    property string title: ""
    property Item contentItem: null
    property Item advancedSettings: null
    property bool showAdvancedSettingsButton: true
    property bool showRestoreDefaultsButton: true
    property var openFolderDialog
    signal restoreDefaultsClicked
    signal downloadClicked

    onContentItemChanged: function() {
        if (contentItem) {
            contentItem.parent = contentInner;
            contentItem.anchors.left = contentInner.left;
            contentItem.anchors.right = contentInner.right;
        }
    }

    onAdvancedSettingsChanged: function() {
        if (advancedSettings) {
            advancedSettings.parent = advancedInner;
            advancedSettings.anchors.left = advancedInner.left;
            advancedSettings.anchors.right = advancedInner.right;
        }
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
            }

            Column {
                id: advancedInner
                visible: false
                Layout.fillWidth: true
            }

            Item {
                Layout.fillWidth: true
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
                    onClicked: {
                        root.restoreDefaultsClicked();
                    }
                }
                MySettingsButton {
                    id: advancedSettingsButton
                    anchors.right: parent.right
                    visible: root.advancedSettings && showAdvancedSettingsButton
                    width: implicitWidth
                    text: !advancedInner.visible ? qsTr("Advanced Settings") : qsTr("Hide Advanced Settings")
                    font.pixelSize: theme.fontSizeLarge
                    Accessible.role: Accessible.Button
                    Accessible.name: text
                    Accessible.description: qsTr("Shows/hides the advanced settings")
                    onClicked: {
                        advancedInner.visible = !advancedInner.visible;
                    }
                }
            }
        }
    }
}
