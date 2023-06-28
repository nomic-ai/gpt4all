import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Qt.labs.folderlistmodel
import chatlistmodel
import download
import modellist
import network
import llm
import mysettings

Dialog {
    id: settingsDialog
    modal: true
    opacity: 0.9
    padding: 20
    bottomPadding: 30
    background: Rectangle {
        anchors.fill: parent
        color: theme.backgroundDarkest
        border.width: 1
        border.color: theme.dialogBorder
        radius: 10
    }

    onOpened: {
        Network.sendSettingsDialog();
    }

    property var currentChat: ChatListModel.currentChat

    Theme {
        id: theme
    }

    function restoreGenerationDefaults() {
        MySettings.restoreGenerationDefaults();
        templateTextArea.text = MySettings.promptTemplate
    }

    function restoreApplicationDefaults() {
        MySettings.restoreApplicationDefaults();
        ModelList.localModelsPath = MySettings.modelPath
        LLM.threadCount = MySettings.threadCount
        LLM.serverEnabled = MySettings.serverChat
        ChatListModel.shouldSaveChats = MySettings.saveChats
        ChatListModel.shouldSaveChatGPTChats = MySettings.saveChatGPTChats
        MySettings.forceMetal = false
    }

    Component.onCompleted: {
        LLM.threadCount = MySettings.threadCount
        LLM.serverEnabled = MySettings.serverChat
        ChatListModel.shouldSaveChats = MySettings.saveChats
        ChatListModel.shouldSaveChatGPTChats = MySettings.saveChatGPTChats
        ModelList.localModelsPath = MySettings.modelPath
    }

    Item {
        Accessible.role: Accessible.Dialog
        Accessible.name: qsTr("Settings dialog")
        Accessible.description: qsTr("Dialog containing various application settings")
    }
    TabBar {
        id: settingsTabBar
        width: parent.width / 1.25
        z: 200

        TabButton {
            id: genSettingsButton
            contentItem: IconLabel {
                color: theme.textColor
                font.bold: genSettingsButton.checked
                text: qsTr("Generation")
            }
            background: Rectangle {
                color: genSettingsButton.checked ? theme.backgroundDarkest : theme.backgroundLight
                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: genSettingsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: !genSettingsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    width: genSettingsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    width: genSettingsButton.checked
                    color: theme.tabBorder
                }
            }
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Generation settings")
            Accessible.description: qsTr("Settings related to how the model generates text")
        }

        TabButton {
            id: appSettingsButton
            contentItem: IconLabel {
                color: theme.textColor
                font.bold: appSettingsButton.checked
                text: qsTr("Application")
            }
            background: Rectangle {
                color: appSettingsButton.checked ? theme.backgroundDarkest : theme.backgroundLight
                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: appSettingsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: !appSettingsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    width: appSettingsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    width: appSettingsButton.checked
                    color: theme.tabBorder
                }
            }
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Application settings")
            Accessible.description: qsTr("Settings related to general behavior of the application")
        }

        TabButton {
            id: localDocsButton
            contentItem: IconLabel {
                color: theme.textColor
                font.bold: localDocsButton.checked
                text: qsTr("LocalDocs Plugin (BETA)")
            }
            background: Rectangle {
                color: localDocsButton.checked ? theme.backgroundDarkest : theme.backgroundLight
                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: localDocsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: !localDocsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    width: localDocsButton.checked
                    color: theme.tabBorder
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    width: localDocsButton.checked
                    color: theme.tabBorder
                }
            }
            Accessible.role: Accessible.Button
            Accessible.name: qsTr("LocalDocs settings")
            Accessible.description: qsTr("Settings related to localdocs plugin")
        }
    }

    StackLayout {
        anchors.top: settingsTabBar.bottom
        anchors.topMargin: -1
        width: parent.width
        height: availableHeight
        currentIndex: settingsTabBar.currentIndex

        Item {
            id: generationSettingsTab
            ScrollView {
                background: Rectangle {
                    color: 'transparent'
                    border.color: theme.tabBorder
                    border.width: 1
                    radius: 2
                }
                padding: 10
                width: parent.width
                height: parent.height - 30
                contentWidth: availableWidth - 20
                contentHeight: generationSettingsTabInner.implicitHeight + 40
                ScrollBar.vertical.policy: ScrollBar.AlwaysOn

                GridLayout {
                    id: generationSettingsTabInner
                    anchors.margins: 10
                    columns: 2
                    rowSpacing: 10
                    columnSpacing: 10
                    anchors.fill: parent

                    Label {
                        id: tempLabel
                        text: qsTr("Temperature:")
                        color: theme.textColor
                        Layout.row: 0
                        Layout.column: 0
                    }
                    MyTextField {
                        text: MySettings.temperature
                        color: theme.textColor
                        ToolTip.text: qsTr("Temperature increases the chances of choosing less likely tokens.\nNOTE: Higher temperature gives more creative but less predictable outputs.")
                        ToolTip.visible: hovered
                        Layout.row: 0
                        Layout.column: 1
                        validator: DoubleValidator {
                            locale: "C"
                        }
                        onEditingFinished: {
                            var val = parseFloat(text)
                            if (!isNaN(val)) {
                                MySettings.temperature = val
                                focus = false
                            } else {
                                text = MySettings.temperature
                            }
                        }
                        Accessible.role: Accessible.EditableText
                        Accessible.name: tempLabel.text
                        Accessible.description: ToolTip.text
                    }
                    Label {
                        id: topPLabel
                        text: qsTr("Top P:")
                        color: theme.textColor
                        Layout.row: 1
                        Layout.column: 0
                    }
                    MyTextField {
                        text: MySettings.topP
                        color: theme.textColor
                        ToolTip.text: qsTr("Only the most likely tokens up to a total probability of top_p can be chosen.\nNOTE: Prevents choosing highly unlikely tokens, aka Nucleus Sampling")
                        ToolTip.visible: hovered
                        Layout.row: 1
                        Layout.column: 1
                        validator: DoubleValidator {
                            locale: "C"
                        }
                        onEditingFinished: {
                            var val = parseFloat(text)
                            if (!isNaN(val)) {
                                MySettings.topP = val
                                focus = false
                            } else {
                                text = MySettings.topP
                            }
                        }
                        Accessible.role: Accessible.EditableText
                        Accessible.name: topPLabel.text
                        Accessible.description: ToolTip.text
                    }
                    Label {
                        id: topKLabel
                        text: qsTr("Top K:")
                        color: theme.textColor
                        Layout.row: 2
                        Layout.column: 0
                    }
                    MyTextField {
                        text: MySettings.topK
                        color: theme.textColor
                        ToolTip.text: qsTr("Only the top K most likely tokens will be chosen from")
                        ToolTip.visible: hovered
                        Layout.row: 2
                        Layout.column: 1
                        validator: IntValidator {
                            bottom: 1
                        }
                        onEditingFinished: {
                            var val = parseInt(text)
                            if (!isNaN(val)) {
                                MySettings.topK = val
                                focus = false
                            } else {
                                text = MySettings.topK
                            }
                        }
                        Accessible.role: Accessible.EditableText
                        Accessible.name: topKLabel.text
                        Accessible.description: ToolTip.text
                    }
                    Label {
                        id: maxLengthLabel
                        text: qsTr("Max Length:")
                        color: theme.textColor
                        Layout.row: 3
                        Layout.column: 0
                    }
                    MyTextField {
                        text: MySettings.maxLength
                        color: theme.textColor
                        ToolTip.text: qsTr("Maximum length of response in tokens")
                        ToolTip.visible: hovered
                        Layout.row: 3
                        Layout.column: 1
                        validator: IntValidator {
                            bottom: 1
                        }
                        onEditingFinished: {
                            var val = parseInt(text)
                            if (!isNaN(val)) {
                                MySettings.maxLength = val
                                focus = false
                            } else {
                                text = MySettings.maxLength
                            }
                        }
                        Accessible.role: Accessible.EditableText
                        Accessible.name: maxLengthLabel.text
                        Accessible.description: ToolTip.text
                    }

                    Label {
                        id: batchSizeLabel
                        text: qsTr("Prompt Batch Size:")
                        color: theme.textColor
                        Layout.row: 4
                        Layout.column: 0
                    }
                    MyTextField {
                        text: MySettings.promptBatchSize
                        color: theme.textColor
                        ToolTip.text: qsTr("Amount of prompt tokens to process at once.\nNOTE: Higher values can speed up reading prompts but will use more RAM")
                        ToolTip.visible: hovered
                        Layout.row: 4
                        Layout.column: 1
                        validator: IntValidator {
                            bottom: 1
                        }
                        onEditingFinished: {
                            var val = parseInt(text)
                            if (!isNaN(val)) {
                                MySettings.promptBatchSize = val
                                focus = false
                            } else {
                                text = MySettings.promptBatchSize
                            }
                        }
                        Accessible.role: Accessible.EditableText
                        Accessible.name: batchSizeLabel.text
                        Accessible.description: ToolTip.text
                    }
                    Label {
                        id: repeatPenaltyLabel
                        text: qsTr("Repeat Penalty:")
                        color: theme.textColor
                        Layout.row: 5
                        Layout.column: 0
                    }
                    MyTextField {
                        text: MySettings.repeatPenalty
                        color: theme.textColor
                        ToolTip.text: qsTr("Amount to penalize repetitiveness of the output")
                        ToolTip.visible: hovered
                        Layout.row: 5
                        Layout.column: 1
                        validator: DoubleValidator {
                            locale: "C"
                        }
                        onEditingFinished: {
                            var val = parseFloat(text)
                            if (!isNaN(val)) {
                                MySettings.repeatPenalty = val
                                focus = false
                            } else {
                                text = MySettings.repeatPenalty
                            }
                        }
                        Accessible.role: Accessible.EditableText
                        Accessible.name: repeatPenaltyLabel.text
                        Accessible.description: ToolTip.text
                    }
                    Label {
                        id: repeatPenaltyTokensLabel
                        text: qsTr("Repeat Penalty Tokens:")
                        color: theme.textColor
                        Layout.row: 6
                        Layout.column: 0
                    }
                    MyTextField {
                        text: MySettings.repeatPenaltyTokens
                        color: theme.textColor
                        ToolTip.text: qsTr("How far back in output to apply repeat penalty")
                        ToolTip.visible: hovered
                        Layout.row: 6
                        Layout.column: 1
                        validator: IntValidator {
                            bottom: 1
                        }
                        onEditingFinished: {
                            var val = parseInt(text)
                            if (!isNaN(val)) {
                                MySettings.repeatPenaltyTokens = val
                                focus = false
                            } else {
                                text = MySettings.repeatPenaltyTokens
                            }
                        }
                        Accessible.role: Accessible.EditableText
                        Accessible.name: repeatPenaltyTokensLabel.text
                        Accessible.description: ToolTip.text
                    }

                    ColumnLayout {
                        Layout.row: 7
                        Layout.column: 0
                        Layout.topMargin: 10
                        Layout.alignment: Qt.AlignTop
                        spacing: 20

                        Label {
                            id: promptTemplateLabel
                            text: qsTr("Prompt Template:")
                            color: theme.textColor
                        }

                        Label {
                            id: promptTemplateLabelHelp
                            Layout.maximumWidth: promptTemplateLabel.width
                            visible: templateTextArea.text.indexOf(
                                         "%1") === -1
                            color: theme.textErrorColor
                            text: qsTr("Must contain the string \"%1\" to be replaced with the user's input.")
                            wrapMode: TextArea.Wrap
                            Accessible.role: Accessible.EditableText
                            Accessible.name: text
                        }
                    }

                    Rectangle {
                        Layout.row: 7
                        Layout.column: 1
                        Layout.fillWidth: true
                        height: 200
                        color: "transparent"
                        clip: true
                        ScrollView {
                            id: templateScrollView
                            anchors.fill: parent
                            TextArea {
                                id: templateTextArea
                                text: MySettings.promptTemplate
                                color: theme.textColor
                                background: Rectangle {
                                    implicitWidth: 150
                                    color: theme.backgroundLighter
                                    radius: 10
                                }
                                padding: 10
                                wrapMode: TextArea.Wrap
                                onTextChanged: {
                                    if (templateTextArea.text.indexOf("%1") !== -1) {
                                        MySettings.promptTemplate = text
                                    }
                                }
                                bottomPadding: 10
                                Accessible.role: Accessible.EditableText
                                Accessible.name: promptTemplateLabel.text
                                Accessible.description: promptTemplateLabelHelp.text
                                ToolTip.text: qsTr("The prompt template partially determines how models will respond to prompts.\nNOTE: A longer, detailed template can lead to higher quality answers, but can also slow down generation.")
                                ToolTip.visible: hovered
                            }
                        }
                    }
                    MyButton {
                        Layout.row: 8
                        Layout.column: 1
                        Layout.fillWidth: true
                        text: qsTr("Restore Defaults")
                        Accessible.description: qsTr("Restores the settings dialog to a default state")
                        onClicked: {
                            settingsDialog.restoreGenerationDefaults()
                        }
                    }
                }
            }
        }
        Item {
            id: applicationSettingsTab
            ScrollView {
                background: Rectangle {
                    color: 'transparent'
                    border.color: theme.tabBorder
                    border.width: 1
                    radius: 2
                }
                padding: 10
                width: parent.width
                height: parent.height - 30
                contentWidth: availableWidth - 20
                ScrollBar.vertical.policy: ScrollBar.AlwaysOn

                GridLayout {
                    anchors.margins: 10
                    columns: 3
                    rowSpacing: 10
                    columnSpacing: 10
                    anchors.fill: parent
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
                        Layout.minimumWidth: 350
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
                        currentFolder: "file://" + ModelList.localModelsPath
                        onAccepted: {
                            modelPathDisplayField.text = selectedFolder
                            ModelList.localModelsPath = modelPathDisplayField.text
                            MySettings.modelPath = ModelList.localModelsPath
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
                        text: ModelList.localModelsPath
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
                                ModelList.localModelsPath = modelPathDisplayField.text
                                MySettings.modelPath = ModelList.localModelsPath
                            } else {
                                text = ModelList.localModelsPath
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
                                LLM.threadCount = val
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
                            ChatListModel.shouldSaveChats = saveChatsBox.checked
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
                            ChatListModel.shouldSaveChatGPTChats = saveChatGPTChatsBox.checked
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
                            LLM.serverEnabled = serverChatBox.checked
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
                        color: theme.dialogBorder
                    }
                    Rectangle {
                        Layout.row: 9
                        Layout.column: 0
                        Layout.fillWidth: true
                        Layout.columnSpan: 3
                        height: 1
                        color: theme.dialogBorder
                    }
                    Label {
                        id: gpuOverrideLabel
                        text: qsTr("Force Metal (macOS+arm):")
                        color: theme.textColor
                        Layout.row: 8
                        Layout.column: 0
                    }
                    RowLayout {
                        Layout.row: 8
                        Layout.column: 1
                        Layout.columnSpan: 2
                        MyCheckBox {
                            id: gpuOverrideBox
                            checked: MySettings.forceMetal
                            onClicked: {
                                MySettings.forceMetal = !MySettings.forceMetal
                            }
                        }
                        Label {
                            id: warningLabel
                            Layout.maximumWidth: 730
                            Layout.alignment: Qt.AlignTop
                            color: theme.textErrorColor
                            wrapMode: Text.WordWrap
                            text: qsTr("WARNING: On macOS with arm (M1+) this setting forces usage of the GPU. Can cause crashes if the model requires more RAM than the system supports. Because of crash possibility the setting will not persist across restarts of the application. This has no effect on non-macs or intel.")
                        }
                    }
                    MyButton {
                        Layout.row: 10
                        Layout.column: 1
                        Layout.columnSpan: 2
                        Layout.fillWidth: true
                        text: qsTr("Restore Defaults")
                        Accessible.description: qsTr("Restores the settings dialog to a default state")
                        onClicked: {
                            settingsDialog.restoreApplicationDefaults()
                        }
                    }
                }
            }
        }
        Item {
            id: localDocsTab
            ScrollView {
                background: Rectangle {
                    color: 'transparent'
                    border.color: theme.tabBorder
                    border.width: 1
                    radius: 2
                }
                padding: 10
                width: parent.width
                height: parent.height - 30
                contentWidth: availableWidth - 20
                ScrollBar.vertical.policy: ScrollBar.AlwaysOn

                LocalDocs {
                    anchors.margins: 10
                    anchors.fill: parent
                }
            }
        }
    }
}
