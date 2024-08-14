import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import localdocs
import modellist
import mysettings
import network

MySettingsTab {
    onRestoreDefaultsClicked: {
        MySettings.restoreLocalDocsDefaults();
    }

    showRestoreDefaultsButton: true

    title: qsTr("Web Search")
    contentItem: ColumnLayout {
        id: root
        spacing: 30

        ColumnLayout {
            spacing: 10
            Label {
                color: theme.grayRed900
                font.pixelSize: theme.fontSizeLarge
                font.bold: true
                text: qsTr("Web Search")
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: theme.grayRed500
            }
        }

        RowLayout {
            MySettingsLabel {
                id: usageModeLabel
                text: qsTr("Usage Mode")
                helpText: qsTr("When and how the brave search tool is executed.")
            }
            MyComboBox {
                id: usageModeBox
                Layout.minimumWidth: 400
                Layout.maximumWidth: 400
                Layout.alignment: Qt.AlignRight
                // NOTE: indices match values of UsageMode enum, keep them in sync
                model: ListModel {
                    ListElement { name: qsTr("Never") }
                    ListElement { name: qsTr("Model decides") }
                    ListElement { name: qsTr("Ask for confirmation before executing") }
                    ListElement { name: qsTr("Force usage for every response when possible") }
                }
                Accessible.name: usageModeLabel.text
                Accessible.description: usageModeLabel.helpText
                onActivated: {
                }
                Component.onCompleted: {
                }
            }
        }

        RowLayout {
            MySettingsLabel {
                id: apiKeyLabel
                text: qsTr("Brave AI API key")
                helpText: qsTr('The API key to use for Brave Web Search. Get one from the Brave for free <a href="https://brave.com/search/api/">API keys page</a>.')
                onLinkActivated: function(link) { Qt.openUrlExternally(link) }
            }

            MyTextField {
                id: apiKeyField
                text: MySettings.braveSearchAPIKey
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                Layout.alignment: Qt.AlignRight
                Layout.minimumWidth: 400
                Layout.maximumWidth: 400
                onEditingFinished: {
                    MySettings.braveSearchAPIKey = apiKeyField.text;
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: apiKeyLabel.text
                Accessible.description: apiKeyLabel.helpText
            }
        }

        Rectangle {
            Layout.topMargin: 15
            Layout.fillWidth: true
            height: 1
            color: theme.settingsDivider
        }
    }
}
