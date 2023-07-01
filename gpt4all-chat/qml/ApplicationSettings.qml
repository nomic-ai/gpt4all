import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import modellist
import mysettings
import network

MySettingsTab {
    onRestoreDefaultsClicked: {
        MySettings.restoreApplicationDefaults();
    }
    title: qsTr("Application")
    contentItem: GridLayout {
        id: applicationSettingsTabInner
        columns: 3
        rowSpacing: 10
        columnSpacing: 10

        Label {
            id: defaultModelLabel
            text: qsTr("Default model:")
            color: theme.textColor
            Layout.row: 1
            Layout.column: 0
        }
        MyComboBox {
            id: comboBox
            Layout.row: 1
            Layout.column: 1
            Layout.columnSpan: 2
            Layout.minimumWidth: 350
            Layout.fillWidth: true
            model: ModelList.userDefaultModelList
            Accessible.role: Accessible.ComboBox
            Accessible.name: qsTr("ComboBox for displaying/picking the default model")
            Accessible.description: qsTr("Use this for picking the default model to use; the first item is the current default model")
            function updateModel() {
                comboBox.currentIndex = comboBox.indexOfValue(MySettings.userDefaultModel);
            }
            Component.onCompleted: {
                comboBox.updateModel()
            }
            Connections {
                target: MySettings
                function onUserDefaultModelChanged() {
                    comboBox.updateModel()
                }
            }
            onActivated: {
                MySettings.userDefaultModel = comboBox.currentText
            }
        }
        FolderDialog {
            id: modelPathDialog
            title: "Please choose a directory"
            currentFolder: "file://" + MySettings.modelPath
            onAccepted: {
                MySettings.modelPath = selectedFolder
            }
        }
        Label {
            id: modelPathLabel
            text: qsTr("Download path:")
            color: theme.textColor
            Layout.row: 2
            Layout.column: 0
        }
        MyDirectoryField {
            id: modelPathDisplayField
            text: MySettings.modelPath
            implicitWidth: 300
            Layout.row: 2
            Layout.column: 1
            Layout.fillWidth: true
            ToolTip.text: qsTr("Path where model files will be downloaded to")
            ToolTip.visible: hovered
            Accessible.role: Accessible.ToolTip
            Accessible.name: modelPathDisplayField.text
            Accessible.description: ToolTip.text
            onEditingFinished: {
                if (isValid) {
                    MySettings.modelPath = modelPathDisplayField.text
                } else {
                    text = MySettings.modelPath
                }
            }
        }
        MyButton {
            Layout.row: 2
            Layout.column: 2
            text: qsTr("Browse")
            Accessible.description: qsTr("Opens a folder picker dialog to choose where to save model files")
            onClicked: modelPathDialog.open()
        }
        Label {
            id: nThreadsLabel
            text: qsTr("CPU Threads:")
            color: theme.textColor
            Layout.row: 3
            Layout.column: 0
        }
        MyTextField {
            text: MySettings.threadCount
            color: theme.textColor
            ToolTip.text: qsTr("Amount of processing threads to use, a setting of 0 will use the lesser of 4 or your number of CPU threads")
            ToolTip.visible: hovered
            Layout.row: 3
            Layout.column: 1
            validator: IntValidator {
                bottom: 1
            }
            onEditingFinished: {
                var val = parseInt(text)
                if (!isNaN(val)) {
                    MySettings.threadCount = val
                    focus = false
                } else {
                    text = MySettings.threadCount
                }
            }
            Accessible.role: Accessible.EditableText
            Accessible.name: nThreadsLabel.text
            Accessible.description: ToolTip.text
        }
        Label {
            id: saveChatsLabel
            text: qsTr("Save chats to disk:")
            color: theme.textColor
            Layout.row: 4
            Layout.column: 0
        }
        MyCheckBox {
            id: saveChatsBox
            Layout.row: 4
            Layout.column: 1
            checked: MySettings.saveChats
            onClicked: {
                Network.sendSaveChatsToggled(saveChatsBox.checked);
                MySettings.saveChats = !MySettings.saveChats
            }
            ToolTip.text: qsTr("WARNING: Saving chats to disk can be ~2GB per chat")
            ToolTip.visible: hovered
        }
        Label {
            id: saveChatGPTChatsLabel
            text: qsTr("Save ChatGPT chats to disk:")
            color: theme.textColor
            Layout.row: 5
            Layout.column: 0
        }
        MyCheckBox {
            id: saveChatGPTChatsBox
            Layout.row: 5
            Layout.column: 1
            checked: MySettings.saveChatGPTChats
            onClicked: {
                MySettings.saveChatGPTChats = !MySettings.saveChatGPTChats
            }
        }
        Label {
            id: serverChatLabel
            text: qsTr("Enable API server:")
            color: theme.textColor
            Layout.row: 6
            Layout.column: 0
        }
        MyCheckBox {
            id: serverChatBox
            Layout.row: 6
            Layout.column: 1
            checked: MySettings.serverChat
            onClicked: {
                MySettings.serverChat = !MySettings.serverChat
            }
            ToolTip.text: qsTr("WARNING: This enables the gui to act as a local REST web server(OpenAI API compliant) for API requests and will increase your RAM usage as well")
            ToolTip.visible: hovered
        }
        Rectangle {
            Layout.row: 7
            Layout.column: 0
            Layout.columnSpan: 3
            Layout.fillWidth: true
            height: 1
            color: theme.tabBorder
        }
    }
    advancedSettings: GridLayout {
        columns: 3
        rowSpacing: 10
        columnSpacing: 10
        Rectangle {
            Layout.row: 2
            Layout.column: 0
            Layout.fillWidth: true
            Layout.columnSpan: 3
            height: 1
            color: theme.tabBorder
        }
        Label {
            id: gpuOverrideLabel
            text: qsTr("Force Metal (macOS+arm):")
            color: theme.textColor
            Layout.row: 1
            Layout.column: 0
        }
        RowLayout {
            Layout.row: 1
            Layout.column: 1
            Layout.columnSpan: 2
            MyCheckBox {
                id: gpuOverrideBox
                checked: MySettings.forceMetal
                onClicked: {
                    MySettings.forceMetal = !MySettings.forceMetal
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                Layout.minimumHeight: warningLabel.height
                Label {
                    id: warningLabel
                    width: parent.width
                    color: theme.textErrorColor
                    wrapMode: Text.WordWrap
                    text: qsTr("WARNING: On macOS with arm (M1+) this setting forces usage of the GPU. Can cause crashes if the model requires more RAM than the system supports. Because of crash possibility the setting will not persist across restarts of the application. This has no effect on non-macs or intel.")
                }
            }
        }
    }
}

