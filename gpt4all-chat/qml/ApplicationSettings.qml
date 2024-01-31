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
        MySettingsLabel {
            id: themeLabel
            text: qsTr("Theme")
            Layout.row: 1
            Layout.column: 0
        }
        MyComboBox {
            id: themeBox
            Layout.row: 1
            Layout.column: 1
            Layout.columnSpan: 1
            Layout.minimumWidth: 200
            Layout.fillWidth: false
            model: ["Dark", "Light", "LegacyDark"]
            Accessible.role: Accessible.ComboBox
            Accessible.name: qsTr("Color theme")
            Accessible.description: qsTr("Color theme for the chat client to use")
            function updateModel() {
                themeBox.currentIndex = themeBox.indexOfValue(MySettings.chatTheme);
            }
            Component.onCompleted: {
                themeBox.updateModel()
            }
            Connections {
                target: MySettings
                function onChatThemeChanged() {
                    themeBox.updateModel()
                }
            }
            onActivated: {
                MySettings.chatTheme = themeBox.currentText
            }
        }
        MySettingsLabel {
            id: fontLabel
            text: qsTr("Font Size")
            Layout.row: 2
            Layout.column: 0
        }
        MyComboBox {
            id: fontBox
            Layout.row: 2
            Layout.column: 1
            Layout.columnSpan: 1
            Layout.minimumWidth: 100
            Layout.fillWidth: false
            model: ["Small", "Medium", "Large"]
            Accessible.role: Accessible.ComboBox
            Accessible.name: qsTr("Font size")
            Accessible.description: qsTr("Font size of the chat client")
            function updateModel() {
                fontBox.currentIndex = fontBox.indexOfValue(MySettings.fontSize);
            }
            Component.onCompleted: {
                fontBox.updateModel()
            }
            Connections {
                target: MySettings
                function onFontSizeChanged() {
                    fontBox.updateModel()
                }
            }
            onActivated: {
                MySettings.fontSize = fontBox.currentText
            }
        }
        MySettingsLabel {
            id: deviceLabel
            text: qsTr("Device")
            Layout.row: 3
            Layout.column: 0
        }
        MyComboBox {
            id: deviceBox
            Layout.row: 3
            Layout.column: 1
            Layout.columnSpan: 1
            Layout.minimumWidth: 350
            Layout.fillWidth: false
            model: MySettings.deviceList
            Accessible.role: Accessible.ComboBox
            Accessible.name: qsTr("Device")
            Accessible.description: qsTr("Device of the chat client")
            function updateModel() {
                deviceBox.currentIndex = deviceBox.indexOfValue(MySettings.device);
            }
            Component.onCompleted: {
                deviceBox.updateModel()
            }
            Connections {
                target: MySettings
                function onDeviceChanged() {
                    deviceBox.updateModel()
                }
                function onDeviceListChanged() {
                    deviceBox.updateModel()
                }
            }
            onActivated: {
                MySettings.device = deviceBox.currentText
            }
        }
        MySettingsLabel {
            id: defaultModelLabel
            text: qsTr("Default model")
            Layout.row: 4
            Layout.column: 0
        }
        MyComboBox {
            id: comboBox
            Layout.row: 4
            Layout.column: 1
            Layout.columnSpan: 2
            Layout.minimumWidth: 350
            Layout.fillWidth: true
            model: ModelList.userDefaultModelList
            Accessible.role: Accessible.ComboBox
            Accessible.name: qsTr("Default model")
            Accessible.description: qsTr("Default model to use; the first item is the current default model")
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
        MySettingsLabel {
            id: modelPathLabel
            text: qsTr("Download path")
            Layout.row: 5
            Layout.column: 0
        }
        MyDirectoryField {
            id: modelPathDisplayField
            text: MySettings.modelPath
            font.pixelSize: theme.fontSizeLarge
            implicitWidth: 300
            Layout.row: 5
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
        MySettingsButton {
            Layout.row: 5
            Layout.column: 2
            text: qsTr("Browse")
            Accessible.description: qsTr("Choose where to save model files")
            onClicked: {
                openFolderDialog("file://" + MySettings.modelPath, function(selectedFolder) {
                    MySettings.modelPath = selectedFolder
                })
            }
        }
        MySettingsLabel {
            id: nThreadsLabel
            text: qsTr("CPU Threads")
            Layout.row: 6
            Layout.column: 0
        }
        MyTextField {
            text: MySettings.threadCount
            color: theme.textColor
            font.pixelSize: theme.fontSizeLarge
            ToolTip.text: qsTr("Amount of processing threads to use bounded by 1 and number of logical processors")
            ToolTip.visible: hovered
            Layout.row: 6
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
        MySettingsLabel {
            id: saveChatsContextLabel
            text: qsTr("Save chats context to disk")
            Layout.row: 7
            Layout.column: 0
        }
        MyCheckBox {
            id: saveChatsContextBox
            Layout.row: 7
            Layout.column: 1
            checked: MySettings.saveChatsContext
            onClicked: {
                MySettings.saveChatsContext = !MySettings.saveChatsContext
            }
            ToolTip.text: qsTr("WARNING: Saving chats to disk can be ~2GB per chat")
            ToolTip.visible: hovered
        }
        MySettingsLabel {
            id: serverChatLabel
            text: qsTr("Enable API server")
            Layout.row: 8
            Layout.column: 0
        }
        MyCheckBox {
            id: serverChatBox
            Layout.row: 8
            Layout.column: 1
            checked: MySettings.serverChat
            onClicked: {
                MySettings.serverChat = !MySettings.serverChat
            }
            ToolTip.text: qsTr("WARNING: This enables the gui to act as a local REST web server(OpenAI API compliant) for API requests and will increase your RAM usage as well")
            ToolTip.visible: hovered
        }
        Rectangle {
            Layout.row: 9
            Layout.column: 0
            Layout.columnSpan: 3
            Layout.fillWidth: true
            height: 3
            color: theme.accentColor
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
            height: 3
            color: theme.accentColor
        }
        MySettingsLabel {
            id: gpuOverrideLabel
            text: qsTr("Force Metal (macOS+arm)")
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
                MySettingsLabel {
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

