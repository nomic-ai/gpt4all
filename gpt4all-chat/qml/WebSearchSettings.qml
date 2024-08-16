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
        MySettings.restoreWebSearchDefaults();
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
                    ListElement { name: qsTr("Force usage for every response where possible") }
                }
                function updateModel() {
                    usageModeBox.currentIndex = MySettings.webSearchUsageMode;
                }
                Accessible.name: usageModeLabel.text
                Accessible.description: usageModeLabel.helpText
                onActivated: {
                    MySettings.webSearchUsageMode = usageModeBox.currentIndex;
                }
                Component.onCompleted: {
                    usageModeBox.updateModel();
                }
                Connections {
                    target: MySettings
                    function onWebSearchUsageModeChanged() {
                        usageModeBox.updateModel();
                    }
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
                enabled: usageModeBox.currentIndex !== 0
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

        RowLayout {
            MySettingsLabel {
                id: contextItemsPerPrompt
                text: qsTr("Max source excerpts per prompt")
                helpText: qsTr("Max best N matches of retrieved source excerpts to add to the context for prompt. Larger numbers increase likelihood of factual responses, but also result in slower generation.")
            }

            MyTextField {
                text: MySettings.webSearchRetrievalSize
                font.pixelSize: theme.fontSizeLarge
                validator: IntValidator {
                    bottom: 1
                }
                onEditingFinished: {
                    var val = parseInt(text)
                    if (!isNaN(val)) {
                        MySettings.webSearchRetrievalSize = val
                        focus = false
                    } else {
                        text = MySettings.webSearchRetrievalSize
                    }
                }
            }
        }

// FIXME:
//        RowLayout {
//            MySettingsLabel {
//                id: askBeforeRunningLabel
//                text: qsTr("Ask before running")
//                helpText: qsTr("The user is queried whether they want the tool to run in every instance.")
//            }
//            MyCheckBox {
//                id: askBeforeRunningBox
//                checked: MySettings.webSearchConfirmationMode
//                onClicked: {
//                    MySettings.webSearchConfirmationMode = !MySettings.webSearchAskBeforeRunning
//                }
//            }
//        }

        Rectangle {
            Layout.topMargin: 15
            Layout.fillWidth: true
            height: 1
            color: theme.settingsDivider
        }
    }
}
