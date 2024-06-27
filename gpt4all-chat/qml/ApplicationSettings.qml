import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import modellist
import mysettings
import network
import llm

MySettingsTab {
    onRestoreDefaultsClicked: {
        MySettings.restoreApplicationDefaults();
    }
    title: qsTr("Application")

    NetworkDialog {
        id: networkDialog
        anchors.centerIn: parent
        width: Math.min(1024, window.width - (window.width * .2))
        height: Math.min(600, window.height - (window.height * .2))
        Item {
            Accessible.role: Accessible.Dialog
            Accessible.name: qsTr("Network dialog")
            Accessible.description: qsTr("opt-in to share feedback/conversations")
        }
    }

    Dialog {
        id: checkForUpdatesError
        anchors.centerIn: parent
        modal: false
        padding: 20
        Text {
            horizontalAlignment: Text.AlignJustify
            text: qsTr("ERROR: Update system could not find the MaintenanceTool used<br>
                   to check for updates!<br><br>
                   Did you install this application using the online installer? If so,<br>
                   the MaintenanceTool executable should be located one directory<br>
                   above where this application resides on your filesystem.<br><br>
                   If you can't start it manually, then I'm afraid you'll have to<br>
                   reinstall.")
            color: theme.textErrorColor
            font.pixelSize: theme.fontSizeLarge
            Accessible.role: Accessible.Dialog
            Accessible.name: text
            Accessible.description: qsTr("Error dialog")
        }
        background: Rectangle {
            anchors.fill: parent
            color: theme.containerBackground
            border.width: 1
            border.color: theme.dialogBorder
            radius: 10
        }
    }

    contentItem: GridLayout {
        id: applicationSettingsTabInner
        columns: 3
        rowSpacing: 30
        columnSpacing: 10

        ColumnLayout {
            Layout.row: 0
            Layout.column: 0
            Layout.columnSpan: 3
            Layout.fillWidth: true
            spacing: 10
            Label {
                color: theme.styledTextColor
                font.pixelSize: theme.fontSizeLarge
                font.bold: true
                text: "General"
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: theme.settingsDivider
            }
        }

        MySettingsLabel {
            id: themeLabel
            text: qsTr("Theme")
            helpText: qsTr("Customize the colors of GPT4All")
            Layout.row: 1
            Layout.column: 0
        }
        MyComboBox {
            id: themeBox
            Layout.row: 1
            Layout.column: 2
            Layout.minimumWidth: 200
            Layout.maximumWidth: 200
            Layout.fillWidth: false
            Layout.alignment: Qt.AlignRight
            model: [qsTr("Dark"), qsTr("Light"), qsTr("LegacyDark")]
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
            helpText: qsTr("How big your font is displayed")
            Layout.row: 2
            Layout.column: 0
        }
        MyComboBox {
            id: fontBox
            Layout.row: 2
            Layout.column: 2
            Layout.minimumWidth: 200
            Layout.maximumWidth: 200
            Layout.fillWidth: false
            Layout.alignment: Qt.AlignRight
            model: ["Small", "Medium", "Large"]
            Accessible.name: fontLabel.text
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
            helpText: qsTr('The compute device used for text generation. "Auto" uses Vulkan or Metal.')
            Layout.row: 3
            Layout.column: 0
        }
        MyComboBox {
            id: deviceBox
            Layout.row: 3
            Layout.column: 2
            Layout.minimumWidth: 400
            Layout.maximumWidth: 400
            Layout.fillWidth: false
            Layout.alignment: Qt.AlignRight
            model: MySettings.deviceList
            Accessible.name: deviceLabel.text
            Accessible.description: deviceLabel.helpText
            function updateModel() {
                deviceBox.currentIndex = deviceBox.indexOfValue(MySettings.device);
            }
            Component.onCompleted: {
                deviceBox.updateModel();
            }
            Connections {
                target: MySettings
                function onDeviceChanged() {
                    deviceBox.updateModel();
                }
            }
            onActivated: {
                MySettings.device = deviceBox.currentText;
            }
        }
        MySettingsLabel {
            id: defaultModelLabel
            text: qsTr("Default model")
            helpText: qsTr("The preferred default model")
            Layout.row: 4
            Layout.column: 0
        }
        MyComboBox {
            id: comboBox
            Layout.row: 4
            Layout.column: 2
            Layout.minimumWidth: 400
            Layout.maximumWidth: 400
            Layout.alignment: Qt.AlignRight
            model: ModelList.userDefaultModelList
            Accessible.name: defaultModelLabel.text
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
            helpText: qsTr("The download folder for models")
            Layout.row: 5
            Layout.column: 0
        }

        RowLayout {
            Layout.row: 5
            Layout.column: 2
            Layout.alignment: Qt.AlignRight
            Layout.minimumWidth: 400
            Layout.maximumWidth: 400
            spacing: 10
            MyDirectoryField {
                id: modelPathDisplayField
                text: MySettings.modelPath
                font.pixelSize: theme.fontSizeLarge
                implicitWidth: 300
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
                text: qsTr("Browse")
                Accessible.description: qsTr("Choose where to save model files")
                onClicked: {
                    openFolderDialog("file://" + MySettings.modelPath, function(selectedFolder) {
                        MySettings.modelPath = selectedFolder
                    })
                }
            }
        }

        MySettingsLabel {
            id: dataLakeLabel
            text: qsTr("Opensource Datalake")
            helpText: qsTr("Send your data to the GPT4All Open Source Datalake.")
            Layout.row: 6
            Layout.column: 0
        }
        MyCheckBox {
            id: dataLakeBox
            Layout.row: 6
            Layout.column: 2
            Layout.alignment: Qt.AlignRight
            checked: MySettings.networkIsActive
            onClicked: {
                if (MySettings.networkIsActive) {
                    MySettings.networkIsActive = false
                } else
                    networkDialog.open()
            }
            ToolTip.text: qsTr("Reveals a dialogue where you can opt-in for sharing data over network")
            ToolTip.visible: hovered
        }

        ColumnLayout {
            Layout.row: 7
            Layout.column: 0
            Layout.columnSpan: 3
            Layout.fillWidth: true
            spacing: 10
            Label {
                color: theme.styledTextColor
                font.pixelSize: theme.fontSizeLarge
                font.bold: true
                text: "Advanced"
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: theme.settingsDivider
            }
        }

        MySettingsLabel {
            id: nThreadsLabel
            text: qsTr("CPU Threads")
            helpText: qsTr("Number of CPU threads for inference and embedding")
            Layout.row: 8
            Layout.column: 0
        }
        MyTextField {
            text: MySettings.threadCount
            color: theme.textColor
            font.pixelSize: theme.fontSizeLarge
            ToolTip.text: qsTr("Amount of processing threads to use bounded by 1 and number of logical processors")
            ToolTip.visible: hovered
            Layout.alignment: Qt.AlignRight
            Layout.row: 8
            Layout.column: 2
            Layout.minimumWidth: 200
            Layout.maximumWidth: 200
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
            text: qsTr("Save chat context")
            helpText: qsTr("Save chat context to disk")
            Layout.row: 9
            Layout.column: 0
        }
        MyCheckBox {
            id: saveChatsContextBox
            Layout.row: 9
            Layout.column: 2
            Layout.alignment: Qt.AlignRight
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
            helpText: qsTr("A local http server running on local port")
            Layout.row: 10
            Layout.column: 0
        }
        MyCheckBox {
            id: serverChatBox
            Layout.row: 10
            Layout.column: 2
            Layout.alignment: Qt.AlignRight
            checked: MySettings.serverChat
            onClicked: {
                MySettings.serverChat = !MySettings.serverChat
            }
            ToolTip.text: qsTr("WARNING: This enables the gui to act as a local REST web server (OpenAI API compliant) for API requests and will increase your RAM usage as well")
            ToolTip.visible: hovered
        }
        MySettingsLabel {
            id: serverPortLabel
            text: qsTr("API Server Port:")
            helpText: qsTr("A local port to run the server (Requires restart)")
            Layout.row: 11
            Layout.column: 0
        }
        MyTextField {
            id: serverPortField
            text: MySettings.networkPort
            color: theme.textColor
            font.pixelSize: theme.fontSizeLarge
            ToolTip.text: qsTr("Api server port. WARNING: You need to restart the application for it to take effect")
            ToolTip.visible: hovered
            Layout.row: 11
            Layout.column: 2
            Layout.minimumWidth: 200
            Layout.maximumWidth: 200
            Layout.alignment: Qt.AlignRight
            validator: IntValidator {
                bottom: 1
            }
            onEditingFinished: {
                var val = parseInt(text)
                if (!isNaN(val)) {
                    MySettings.networkPort = val
                    focus = false
                } else {
                    text = MySettings.networkPort
                }
            }
            Accessible.role: Accessible.EditableText
            Accessible.name: serverPortField.text
            Accessible.description: ToolTip.text
        }

        /*MySettingsLabel {
            id: gpuOverrideLabel
            text: qsTr("Force Metal (macOS+arm)")
            Layout.row: 13
            Layout.column: 0
        }
        MyCheckBox {
            id: gpuOverrideBox
            Layout.row: 13
            Layout.column: 2
            Layout.alignment: Qt.AlignRight
            checked: MySettings.forceMetal
            onClicked: {
                MySettings.forceMetal = !MySettings.forceMetal
            }
            ToolTip.text: qsTr("WARNING: On macOS with arm (M1+) this setting forces usage of the GPU. Can cause crashes if the model requires more RAM than the system supports. Because of crash possibility the setting will not persist across restarts of the application. This has no effect on non-macs or intel.")
            ToolTip.visible: hovered
        }*/

        MySettingsLabel {
            id: updatesLabel
            text: qsTr("Check for updates")
            helpText: qsTr("Click to see if an update to the application is available");
            Layout.row: 14
            Layout.column: 0
        }

        MySettingsButton {
            Layout.row: 14
            Layout.column: 2
            Layout.alignment: Qt.AlignRight
            text: qsTr("Updates");
            onClicked: {
                if (!LLM.checkForUpdates())
                    checkForUpdatesError.open()
            }
        }

        Rectangle {
            Layout.row: 15
            Layout.column: 0
            Layout.columnSpan: 3
            Layout.fillWidth: true
            height: 1
            color: theme.settingsDivider
        }
    }
}

