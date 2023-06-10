import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Qt.labs.folderlistmodel
import download
import network
import llm

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

    property var currentChat: LLM.chatListModel.currentChat

    Theme {
        id: theme
    }

    property real defaultTemperature: 0.7
    property real defaultTopP: 0.1
    property int defaultTopK: 40
    property int defaultMaxLength: 4096
    property int defaultPromptBatchSize: 128
    property real defaultRepeatPenalty: 1.18
    property int defaultRepeatPenaltyTokens: 64
    property int defaultThreadCount: 0
    property bool defaultSaveChats: false
    property bool defaultSaveChatGPTChats: true
    property bool defaultServerChat: false
    property string defaultPromptTemplate: "### Human:
%1
### Assistant:\n"
    property string defaultModelPath: Download.defaultLocalModelsPath()
    property string defaultUserDefaultModel: "Application default"

    property alias temperature: settings.temperature
    property alias topP: settings.topP
    property alias topK: settings.topK
    property alias maxLength: settings.maxLength
    property alias promptBatchSize: settings.promptBatchSize
    property alias promptTemplate: settings.promptTemplate
    property alias repeatPenalty: settings.repeatPenalty
    property alias repeatPenaltyTokens: settings.repeatPenaltyTokens
    property alias threadCount: settings.threadCount
    property alias saveChats: settings.saveChats
    property alias saveChatGPTChats: settings.saveChatGPTChats
    property alias serverChat: settings.serverChat
    property alias modelPath: settings.modelPath
    property alias userDefaultModel: settings.userDefaultModel

    Settings {
        id: settings
        property real temperature: settingsDialog.defaultTemperature
        property real topP: settingsDialog.defaultTopP
        property int topK: settingsDialog.defaultTopK
        property int maxLength: settingsDialog.defaultMaxLength
        property int promptBatchSize: settingsDialog.defaultPromptBatchSize
        property int threadCount: settingsDialog.defaultThreadCount
        property bool saveChats: settingsDialog.defaultSaveChats
        property bool saveChatGPTChats: settingsDialog.defaultSaveChatGPTChats
        property bool serverChat: settingsDialog.defaultServerChat
        property real repeatPenalty: settingsDialog.defaultRepeatPenalty
        property int repeatPenaltyTokens: settingsDialog.defaultRepeatPenaltyTokens
        property string promptTemplate: settingsDialog.defaultPromptTemplate
        property string modelPath: settingsDialog.defaultModelPath
        property string userDefaultModel: settingsDialog.defaultUserDefaultModel
    }

    function restoreGenerationDefaults() {
        settings.temperature = defaultTemperature
        settings.topP = defaultTopP
        settings.topK = defaultTopK
        settings.maxLength = defaultMaxLength
        settings.promptBatchSize = defaultPromptBatchSize
        settings.promptTemplate = defaultPromptTemplate
        settings.repeatPenalty = defaultRepeatPenalty
        settings.repeatPenaltyTokens = defaultRepeatPenaltyTokens
        settings.sync()
    }

    function restoreApplicationDefaults() {
        settings.modelPath = settingsDialog.defaultModelPath
        settings.threadCount = defaultThreadCount
        settings.saveChats = defaultSaveChats
        settings.saveChatGPTChats = defaultSaveChatGPTChats
        settings.serverChat = defaultServerChat
        settings.userDefaultModel = defaultUserDefaultModel
        Download.downloadLocalModelsPath = settings.modelPath
        LLM.threadCount = settings.threadCount
        LLM.serverEnabled = settings.serverChat
        LLM.chatListModel.shouldSaveChats = settings.saveChats
        LLM.chatListModel.shouldSaveChatGPTChats = settings.saveChatGPTChats
        settings.sync()
    }

    Component.onCompleted: {
        LLM.threadCount = settings.threadCount
        LLM.serverEnabled = settings.serverChat
        LLM.chatListModel.shouldSaveChats = settings.saveChats
        LLM.chatListModel.shouldSaveChatGPTChats = settings.saveChatGPTChats
        Download.downloadLocalModelsPath = settings.modelPath
    }

    Connections {
        target: settingsDialog
        function onClosed() {
            settings.sync()
        }
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
                        text: settings.temperature.toString()
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
                                settings.temperature = val
                                settings.sync()
                                focus = false
                            } else {
                                text = settings.temperature.toString()
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
                        text: settings.topP.toString()
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
                                settings.topP = val
                                settings.sync()
                                focus = false
                            } else {
                                text = settings.topP.toString()
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
                        text: settings.topK.toString()
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
                                settings.topK = val
                                settings.sync()
                                focus = false
                            } else {
                                text = settings.topK.toString()
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
                        text: settings.maxLength.toString()
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
                                settings.maxLength = val
                                settings.sync()
                                focus = false
                            } else {
                                text = settings.maxLength.toString()
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
                        text: settings.promptBatchSize.toString()
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
                                settings.promptBatchSize = val
                                settings.sync()
                                focus = false
                            } else {
                                text = settings.promptBatchSize.toString()
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
                        text: settings.repeatPenalty.toString()
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
                                settings.repeatPenalty = val
                                settings.sync()
                                focus = false
                            } else {
                                text = settings.repeatPenalty.toString()
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
                        text: settings.repeatPenaltyTokens.toString()
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
                                settings.repeatPenaltyTokens = val
                                settings.sync()
                                focus = false
                            } else {
                                text = settings.repeatPenaltyTokens.toString()
                            }
                        }
                        Accessible.role: Accessible.EditableText
                        Accessible.name: repeatPenaltyTokensLabel.text
                        Accessible.description: ToolTip.text
                    }

                    Label {
                        id: promptTemplateLabel
                        text: qsTr("Prompt Template:")
                        color: theme.textColor
                        Layout.row: 7
                        Layout.column: 0
                    }
                    Rectangle {
                        Layout.row: 7
                        Layout.column: 1
                        Layout.fillWidth: true
                        height: 200
                        color: "transparent"
                        clip: true
                        Label {
                            id: promptTemplateLabelHelp
                            visible: settings.promptTemplate.indexOf(
                                         "%1") === -1
                            font.bold: true
                            color: theme.textErrorColor
                            text: qsTr("Prompt template must contain %1 to be replaced with the user's input.")
                            anchors.fill: templateScrollView
                            z: 200
                            padding: 10
                            wrapMode: TextArea.Wrap
                            Accessible.role: Accessible.EditableText
                            Accessible.name: text
                        }
                        ScrollView {
                            id: templateScrollView
                            anchors.fill: parent
                            TextArea {
                                text: settings.promptTemplate
                                color: theme.textColor
                                background: Rectangle {
                                    implicitWidth: 150
                                    color: theme.backgroundLighter
                                    radius: 10
                                }
                                padding: 10
                                wrapMode: TextArea.Wrap
                                onTextChanged: {
                                    settings.promptTemplate = text
                                    settings.sync()
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
                        model: modelList
                        Accessible.role: Accessible.ComboBox
                        Accessible.name: qsTr("ComboBox for displaying/picking the default model")
                        Accessible.description: qsTr("Use this for picking the default model to use; the first item is the current default model")
                        function updateModel(newModelList) {
                            var newArray = Array.from(newModelList);
                            newArray.unshift('Application default');
                            comboBox.model = newArray;
                            settings.sync();
                            comboBox.currentIndex = comboBox.indexOfValue(settingsDialog.userDefaultModel);

                        }
                        Component.onCompleted: {
                            comboBox.updateModel(currentChat.modelList)
                        }
                        Connections {
                            target: settings
                            function onUserDefaultModelChanged() {
                                comboBox.updateModel(currentChat.modelList)
                            }
                        }
                        Connections {
                            target: currentChat
                            function onModelListChanged() {
                                comboBox.updateModel(currentChat.modelList)
                            }
                        }
                        onActivated: {
                            settingsDialog.userDefaultModel = comboBox.currentText
                            settings.sync()
                        }
                    }
                    FolderDialog {
                        id: modelPathDialog
                        title: "Please choose a directory"
                        currentFolder: "file://" + Download.downloadLocalModelsPath
                        onAccepted: {
                            modelPathDisplayField.text = selectedFolder
                            Download.downloadLocalModelsPath = modelPathDisplayField.text
                            settings.modelPath = Download.downloadLocalModelsPath
                            settings.sync()
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
                        text: Download.downloadLocalModelsPath
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
                                Download.downloadLocalModelsPath = modelPathDisplayField.text
                                settings.modelPath = Download.downloadLocalModelsPath
                                settings.sync()
                            } else {
                                text = Download.downloadLocalModelsPath
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
                        text: settingsDialog.threadCount.toString()
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
                                settingsDialog.threadCount = val
                                LLM.threadCount = val
                                settings.sync()
                                focus = false
                            } else {
                                text = settingsDialog.threadCount.toString()
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
                        checked: settingsDialog.saveChats
                        onClicked: {
                            Network.sendSaveChatsToggled(saveChatsBox.checked);
                            settingsDialog.saveChats = saveChatsBox.checked
                            LLM.chatListModel.shouldSaveChats = saveChatsBox.checked
                            settings.sync()
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
                        checked: settingsDialog.saveChatGPTChats
                        onClicked: {
                            settingsDialog.saveChatGPTChats = saveChatGPTChatsBox.checked
                            LLM.chatListModel.shouldSaveChatGPTChats = saveChatGPTChatsBox.checked
                            settings.sync()
                        }
                    }
                    Label {
                        id: serverChatLabel
                        text: qsTr("Enable web server:")
                        color: theme.textColor
                        Layout.row: 6
                        Layout.column: 0
                    }
                    MyCheckBox {
                        id: serverChatBox
                        Layout.row: 6
                        Layout.column: 1
                        checked: settings.serverChat
                        onClicked: {
                            settingsDialog.serverChat = serverChatBox.checked
                            LLM.serverEnabled = serverChatBox.checked
                            settings.sync()
                        }
                        ToolTip.text: qsTr("WARNING: This enables the gui to act as a local web server for AI API requests and will increase your RAM usage as well")
                        ToolTip.visible: hovered
                    }
                    MyButton {
                        Layout.row: 7
                        Layout.column: 1
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
