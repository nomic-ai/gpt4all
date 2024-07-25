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

    title: qsTr("Tools")
    contentItem: ColumnLayout {
        id: root
        spacing: 30

        ColumnLayout {
            spacing: 10
            Label {
                color: theme.grayRed900
                font.pixelSize: theme.fontSizeLarge
                font.bold: true
                text: qsTr("Brave Search")
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: theme.grayRed500
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
                Layout.minimumWidth: 200
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
