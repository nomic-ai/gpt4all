import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import modellist
import mysettings
import chatlistmodel

MySettingsTab {
    onRestoreDefaults: {
        MySettings.restoreModelDefaults(root.currentModelInfo);
    }
    title: qsTr("Model")

    ConfirmationDialog {
        id: resetSystemMessageDialog
        property var index: null
        property bool resetClears: false
        dialogTitle: qsTr("%1 system message?").arg(resetClears ? qsTr("Clear") : qsTr("Reset"))
        description: qsTr("The system message will be %1.").arg(resetClears ? qsTr("removed") : qsTr("reset to the default"))
        onAccepted: MySettings.resetModelSystemMessage(ModelList.modelInfo(index))
        function show(index_, resetClears_) { index = index_; resetClears = resetClears_; open(); }
    }

    ConfirmationDialog {
        id: resetChatTemplateDialog
        property bool resetClears: false
        property var index: null
        dialogTitle: qsTr("%1 chat template?").arg(resetClears ? qsTr("Clear") : qsTr("Reset"))
        description: qsTr("The chat template will be %1.").arg(resetClears ? qsTr("erased") : qsTr("reset to the default"))
        onAccepted: {
            MySettings.resetModelChatTemplate(ModelList.modelInfo(index));
            templateTextArea.resetText();
        }
        function show(index_, resetClears_) { index = index_; resetClears = resetClears_; open(); }
    }

    contentItem: GridLayout {
        id: root
        columns: 3
        rowSpacing: 10
        columnSpacing: 10
        enabled: ModelList.selectableModels.count !== 0

        property var currentModelName: comboBox.currentText
        property var currentModelId: comboBox.currentValue
        property var currentModelInfo: ModelList.modelInfo(root.currentModelId)

        Label {
            Layout.row: 1
            Layout.column: 0
            Layout.bottomMargin: 10
            color: theme.settingsTitleTextColor
            font.pixelSize: theme.fontSizeBannerSmall
            font.bold: true
            text: qsTr("Model Settings")
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.maximumWidth: parent.width
            Layout.row: 2
            Layout.column: 0
            Layout.columnSpan: 2
            spacing: 10

            MyComboBox {
                id: comboBox
                Layout.fillWidth: true
                model: ModelList.selectableModels
                valueRole: "id"
                textRole: "name"
                currentIndex: {
                    var i = comboBox.indexOfValue(ChatListModel.currentChat.modelInfo.id);
                    if (i >= 0)
                        return i;
                    return 0;
                }
                contentItem: Text {
                    leftPadding: 10
                    rightPadding: 20
                    text: comboBox.currentText
                    font: comboBox.font
                    color: theme.textColor
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                delegate: ItemDelegate {
                    width: comboBox.width -20
                    contentItem: Text {
                        text: name
                        color: theme.textColor
                        font: comboBox.font
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 10
                        color: highlighted ? theme.menuHighlightColor : theme.menuBackgroundColor
                    }
                    highlighted: comboBox.highlightedIndex === index
                }
            }

            MySettingsButton {
                id: cloneButton
                text: qsTr("Clone")
                onClicked: {
                    var id = ModelList.clone(root.currentModelInfo);
                    comboBox.currentIndex = comboBox.indexOfValue(id);
                }
            }

            MySettingsDestructiveButton {
                id: removeButton
                enabled: root.currentModelInfo.isClone
                text: qsTr("Remove")
                onClicked: {
                    ModelList.removeClone(root.currentModelInfo);
                    comboBox.currentIndex = 0;
                }
            }
        }

        RowLayout {
            Layout.row: 3
            Layout.column: 0
            Layout.topMargin: 15
            spacing: 10
            MySettingsLabel {
                text: qsTr("Name")
            }
        }

        MyTextField {
            id: uniqueNameField
            text: root.currentModelName
            font.pixelSize: theme.fontSizeLarge
            enabled: root.currentModelInfo.isClone || root.currentModelInfo.description === ""
            Layout.row: 4
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Connections {
                target: MySettings
                function onNameChanged() {
                    uniqueNameField.text = root.currentModelInfo.name;
                }
            }
            Connections {
                target: root
                function onCurrentModelInfoChanged() {
                    uniqueNameField.text = root.currentModelInfo.name;
                }
            }
            onTextChanged: {
                if (text !== "" && ModelList.isUniqueName(text)) {
                    MySettings.setModelName(root.currentModelInfo, text);
                }
            }
        }

        MySettingsLabel {
            text: qsTr("Model File")
            Layout.row: 5
            Layout.column: 0
            Layout.topMargin: 15
        }

        MyTextField {
            text: root.currentModelInfo.filename
            font.pixelSize: theme.fontSizeLarge
            enabled: false
            Layout.row: 6
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.row: 7
            Layout.columnSpan: 2
            Layout.topMargin: 15
            Layout.fillWidth: true
            Layout.maximumWidth: parent.width
            spacing: 10
            MySettingsLabel {
                id: systemMessageLabel
                text: qsTr("System Message")
                helpText: qsTr("A message to set the context or guide the behavior of the model. Leave blank for " +
                               "none. NOTE: Since GPT4All 3.5, this should not contain control tokens.")
                onReset: () => resetSystemMessageDialog.show(root.currentModelId, resetClears)
                function updateResetButton() {
                    const info = root.currentModelInfo;
                    // NOTE: checks if the *override* is set, regardless of whether there is a default
                    canReset = !!info.id && MySettings.isModelSystemMessageSet(info);
                    resetClears = !info.defaultSystemMessage;
                }
                Component.onCompleted: updateResetButton()
                Connections {
                    target: root
                    function onCurrentModelIdChanged() { systemMessageLabel.updateResetButton(); }
                }
                Connections {
                    target: MySettings
                    function onSystemMessageChanged(info)
                    { if (info.id === root.currentModelId) systemMessageLabel.updateResetButton(); }
                }
            }
            Label {
                id: systemMessageLabelHelp
                visible: systemMessageArea.errState !== "ok"
                Layout.alignment: Qt.AlignBottom
                Layout.fillWidth: true
                Layout.rightMargin: 5
                Layout.maximumHeight: systemMessageLabel.height
                text: qsTr("System message is not " +
                           "<a href=\"https://docs.gpt4all.io/gpt4all_desktop/chat_templates.html\">plain text</a>.")
                color: systemMessageArea.errState === "error" ? theme.textErrorColor : theme.textWarningColor
                font.pixelSize: theme.fontSizeLarger
                font.bold: true
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                onLinkActivated: function(link) { Qt.openUrlExternally(link) }
            }
        }

        Rectangle {
            id: systemMessage
            Layout.row: 8
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.fillWidth: true
            color: "transparent"
            Layout.minimumHeight: Math.max(100, systemMessageArea.contentHeight + 20)
            MyTextArea {
                id: systemMessageArea
                anchors.fill: parent
                property bool isBeingReset: false
                function resetText() {
                    const info = root.currentModelInfo;
                    isBeingReset = true;
                    text = (info.id ? info.systemMessage.value : null) ?? "";
                    isBeingReset = false;
                }
                Component.onCompleted: resetText()
                Connections {
                    target: MySettings
                    function onSystemMessageChanged(info)
                    { if (info.id === root.currentModelId) systemMessageArea.resetText(); }
                }
                Connections {
                    target: root
                    function onCurrentModelIdChanged() { systemMessageArea.resetText(); }
                }
                // strict validation, because setModelSystemMessage clears isLegacy
                readonly property var reLegacyCheck: (
                    /(?:^|\s)(?:### *System\b|S(?:ystem|YSTEM):)|<\|(?:im_(?:start|end)|(?:start|end)_header_id|eot_id|SYSTEM_TOKEN)\|>|<<SYS>>/m
                )
                onTextChanged: {
                    const info = root.currentModelInfo;
                    if (!info.id) {
                        errState = "ok";
                    } else if (info.systemMessage.isLegacy && (isBeingReset || reLegacyCheck.test(text))) {
                        errState = "error";
                    } else
                        errState = reLegacyCheck.test(text) ? "warning" : "ok";
                    if (info.id && errState !== "error" && !isBeingReset)
                        MySettings.setModelSystemMessage(info, text);
                    systemMessageLabel.updateResetButton();
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: systemMessageLabel.text
                Accessible.description: systemMessageLabelHelp.text
            }
        }

        RowLayout {
            Layout.row: 9
            Layout.columnSpan: 2
            Layout.topMargin: 15
            Layout.fillWidth: true
            Layout.maximumWidth: parent.width
            spacing: 10
            MySettingsLabel {
                id: chatTemplateLabel
                text: qsTr("Chat Template")
                helpText: qsTr("This Jinja template turns the chat into input for the model.")
                onReset: () => resetChatTemplateDialog.show(root.currentModelId, resetClears)
                function updateResetButton() {
                    const info = root.currentModelInfo;
                    canReset = !!info.id && (
                        MySettings.isModelChatTemplateSet(info)
                        || templateTextArea.text !== (info.chatTemplate.value ?? "")
                    );
                    resetClears = !info.defaultChatTemplate;
                }
                Component.onCompleted: updateResetButton()
                Connections {
                    target: root
                    function onCurrentModelIdChanged() { chatTemplateLabel.updateResetButton(); }
                }
                Connections {
                    target: MySettings
                    function onChatTemplateChanged(info)
                    { if (info.id === root.currentModelId) chatTemplateLabel.updateResetButton(); }
                }
            }
            Label {
                id: chatTemplateLabelHelp
                visible: templateTextArea.errState !== "ok"
                Layout.alignment: Qt.AlignBottom
                Layout.fillWidth: true
                Layout.rightMargin: 5
                Layout.maximumHeight: chatTemplateLabel.height
                text: templateTextArea.errMsg
                color: templateTextArea.errState === "error" ? theme.textErrorColor : theme.textWarningColor
                font.pixelSize: theme.fontSizeLarger
                font.bold: true
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                onLinkActivated: function(link) { Qt.openUrlExternally(link) }
            }
        }

        Rectangle {
            id: chatTemplate
            Layout.row: 10
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.minimumHeight: Math.max(100, templateTextArea.contentHeight + 20)
            color: "transparent"
            clip: true
            MyTextArea {
                id: templateTextArea
                anchors.fill: parent
                font: fixedFont
                property bool isBeingReset: false
                property var errMsg: null
                function resetText() {
                    const info = root.currentModelInfo;
                    isBeingReset = true;
                    text = (info.id ? info.chatTemplate.value : null) ?? "";
                    isBeingReset = false;
                }
                Component.onCompleted: resetText()
                Connections {
                    target: MySettings
                    function onChatTemplateChanged() { templateTextArea.resetText(); }
                }
                Connections {
                    target: root
                    function onCurrentModelIdChanged() { templateTextArea.resetText(); }
                }
                function legacyCheck() {
                    return /%[12]\b/.test(text) || !/\{%.*%\}.*\{\{.*\}\}.*\{%.*%\}/.test(text.replace(/\n/g, ''))
                        || !/\bcontent\b/.test(text);
                }
                onTextChanged: {
                    const info = root.currentModelInfo;
                    let jinjaError;
                    if (!info.id) {
                        errMsg = null;
                        errState = "ok";
                    } else if (info.chatTemplate.isLegacy && (isBeingReset || legacyCheck())) {
                        errMsg = null;
                        errState = "error";
                    } else if (text === "" && !info.chatTemplate.isSet) {
                        errMsg = qsTr("No <a href=\"https://docs.gpt4all.io/gpt4all_desktop/chat_templates.html\">" +
                                      "chat template</a> configured.");
                        errState = "error";
                    } else if (/^\s*$/.test(text)) {
                        errMsg = qsTr("The <a href=\"https://docs.gpt4all.io/gpt4all_desktop/chat_templates.html\">" +
                                      "chat template</a> cannot be blank.");
                        errState = "error";
                    } else if ((jinjaError = MySettings.checkJinjaTemplateError(text)) !== null) {
                        errMsg = qsTr("<a href=\"https://docs.gpt4all.io/gpt4all_desktop/chat_templates.html\">Syntax" +
                                      " error</a>: %1").arg(jinjaError);
                        errState = "error";
                    } else if (legacyCheck()) {
                        errMsg = qsTr("Chat template is not in " +
                                      "<a href=\"https://docs.gpt4all.io/gpt4all_desktop/chat_templates.html\">" +
                                      "Jinja format</a>.")
                        errState = "warning";
                    } else {
                        errState = "ok";
                    }
                    if (info.id && errState !== "error" && !isBeingReset)
                        MySettings.setModelChatTemplate(info, text);
                    chatTemplateLabel.updateResetButton();
                }
                Keys.onPressed: event => {
                    if (event.key === Qt.Key_Tab) {
                        const a = templateTextArea;
                        event.accepted = true;              // suppress tab
                        a.insert(a.cursorPosition, '    '); // four spaces
                    }
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: chatTemplateLabel.text
                Accessible.description: chatTemplateLabelHelp.text
            }
        }

        MySettingsLabel {
            id: chatNamePromptLabel
            text: qsTr("Chat Name Prompt")
            helpText: qsTr("Prompt used to automatically generate chat names.")
            Layout.row: 11
            Layout.column: 0
            Layout.topMargin: 15
        }

        Rectangle {
            id: chatNamePrompt
            Layout.row: 12
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.minimumHeight: Math.max(100, chatNamePromptTextArea.contentHeight + 20)
            color: "transparent"
            clip: true
            MyTextArea {
                id: chatNamePromptTextArea
                anchors.fill: parent
                text: root.currentModelInfo.chatNamePrompt
                Connections {
                    target: MySettings
                    function onChatNamePromptChanged() {
                        chatNamePromptTextArea.text = root.currentModelInfo.chatNamePrompt;
                    }
                }
                Connections {
                    target: root
                    function onCurrentModelInfoChanged() {
                        chatNamePromptTextArea.text = root.currentModelInfo.chatNamePrompt;
                    }
                }
                onTextChanged: {
                    MySettings.setModelChatNamePrompt(root.currentModelInfo, text)
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: chatNamePromptLabel.text
                Accessible.description: chatNamePromptLabel.text
            }
        }

        MySettingsLabel {
            id: suggestedFollowUpPromptLabel
            text: qsTr("Suggested FollowUp Prompt")
            helpText: qsTr("Prompt used to generate suggested follow-up questions.")
            Layout.row: 13
            Layout.column: 0
            Layout.topMargin: 15
        }

        Rectangle {
            id: suggestedFollowUpPrompt
            Layout.row: 14
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.minimumHeight: Math.max(100, suggestedFollowUpPromptTextArea.contentHeight + 20)
            color: "transparent"
            clip: true
            MyTextArea {
                id: suggestedFollowUpPromptTextArea
                anchors.fill: parent
                text: root.currentModelInfo.suggestedFollowUpPrompt
                Connections {
                    target: MySettings
                    function onSuggestedFollowUpPromptChanged() {
                        suggestedFollowUpPromptTextArea.text = root.currentModelInfo.suggestedFollowUpPrompt;
                    }
                }
                Connections {
                    target: root
                    function onCurrentModelInfoChanged() {
                        suggestedFollowUpPromptTextArea.text = root.currentModelInfo.suggestedFollowUpPrompt;
                    }
                }
                onTextChanged: {
                    MySettings.setModelSuggestedFollowUpPrompt(root.currentModelInfo, text)
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: suggestedFollowUpPromptLabel.text
                Accessible.description: suggestedFollowUpPromptLabel.text
            }
        }

        GridLayout {
            Layout.row: 15
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.topMargin: 15
            Layout.fillWidth: true
            columns: 4
            rowSpacing: 30
            columnSpacing: 10

            MySettingsLabel {
                id: contextLengthLabel
                visible: !root.currentModelInfo.isOnline
                text: qsTr("Context Length")
                helpText: qsTr("Number of input and output tokens the model sees.")
                Layout.row: 0
                Layout.column: 0
                Layout.maximumWidth: 300 * theme.fontScale
            }
            Item {
                Layout.row: 0
                Layout.column: 1
                Layout.fillWidth: true
                Layout.maximumWidth: 200
                Layout.margins: 0
                height: contextLengthField.height

                MyTextField {
                    id: contextLengthField
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    visible: !root.currentModelInfo.isOnline
                    text: root.currentModelInfo.contextLength
                    font.pixelSize: theme.fontSizeLarge
                    color: theme.textColor
                    ToolTip.text: qsTr("Maximum combined prompt/response tokens before information is lost.\nUsing more context than the model was trained on will yield poor results.\nNOTE: Does not take effect until you reload the model.")
                    ToolTip.visible: hovered
                    Connections {
                        target: MySettings
                        function onContextLengthChanged() {
                            contextLengthField.text = root.currentModelInfo.contextLength;
                        }
                    }
                    Connections {
                        target: root
                        function onCurrentModelInfoChanged() {
                            contextLengthField.text = root.currentModelInfo.contextLength;
                        }
                    }
                    onEditingFinished: {
                        var val = parseInt(text)
                        if (isNaN(val)) {
                            text = root.currentModelInfo.contextLength
                        } else {
                            if (val < 8) {
                                val = 8
                                contextLengthField.text = val
                            } else if (val > root.currentModelInfo.maxContextLength) {
                                val = root.currentModelInfo.maxContextLength
                                contextLengthField.text = val
                            }
                            MySettings.setModelContextLength(root.currentModelInfo, val)
                            focus = false
                        }
                    }
                    Accessible.role: Accessible.EditableText
                    Accessible.name: contextLengthLabel.text
                    Accessible.description: ToolTip.text
                }
            }

            MySettingsLabel {
                id: tempLabel
                text: qsTr("Temperature")
                helpText: qsTr("Randomness of model output. Higher -> more variation.")
                Layout.row: 1
                Layout.column: 2
                Layout.maximumWidth: 300 * theme.fontScale
            }

            MyTextField {
                id: temperatureField
                text: root.currentModelInfo.temperature
                font.pixelSize: theme.fontSizeLarge
                color: theme.textColor
                ToolTip.text: qsTr("Temperature increases the chances of choosing less likely tokens.\nNOTE: Higher temperature gives more creative but less predictable outputs.")
                ToolTip.visible: hovered
                Layout.row: 1
                Layout.column: 3
                validator: DoubleValidator {
                    locale: "C"
                }
                Connections {
                    target: MySettings
                    function onTemperatureChanged() {
                        temperatureField.text = root.currentModelInfo.temperature;
                    }
                }
                Connections {
                    target: root
                    function onCurrentModelInfoChanged() {
                        temperatureField.text = root.currentModelInfo.temperature;
                    }
                }
                onEditingFinished: {
                    var val = parseFloat(text)
                    if (!isNaN(val)) {
                        MySettings.setModelTemperature(root.currentModelInfo, val)
                        focus = false
                    } else {
                        text = root.currentModelInfo.temperature
                    }
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: tempLabel.text
                Accessible.description: ToolTip.text
            }
            MySettingsLabel {
                id: topPLabel
                text: qsTr("Top-P")
                helpText: qsTr("Nucleus Sampling factor. Lower -> more predictable.")
                Layout.row: 2
                Layout.column: 0
                Layout.maximumWidth: 300 * theme.fontScale
            }
            MyTextField {
                id: topPField
                text: root.currentModelInfo.topP
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                ToolTip.text: qsTr("Only the most likely tokens up to a total probability of top_p can be chosen.\nNOTE: Prevents choosing highly unlikely tokens.")
                ToolTip.visible: hovered
                Layout.row: 2
                Layout.column: 1
                validator: DoubleValidator {
                    locale: "C"
                }
                Connections {
                    target: MySettings
                    function onTopPChanged() {
                        topPField.text = root.currentModelInfo.topP;
                    }
                }
                Connections {
                    target: root
                    function onCurrentModelInfoChanged() {
                        topPField.text = root.currentModelInfo.topP;
                    }
                }
                onEditingFinished: {
                    var val = parseFloat(text)
                    if (!isNaN(val)) {
                        MySettings.setModelTopP(root.currentModelInfo, val)
                        focus = false
                    } else {
                        text = root.currentModelInfo.topP
                    }
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: topPLabel.text
                Accessible.description: ToolTip.text
            }
            MySettingsLabel {
                id: minPLabel
                text: qsTr("Min-P")
                helpText: qsTr("Minimum token probability. Higher -> more predictable.")
                Layout.row: 3
                Layout.column: 0
                Layout.maximumWidth: 300 * theme.fontScale
            }
            MyTextField {
                id: minPField
                text: root.currentModelInfo.minP
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                ToolTip.text: qsTr("Sets the minimum relative probability for a token to be considered.")
                ToolTip.visible: hovered
                Layout.row: 3
                Layout.column: 1
                validator: DoubleValidator {
                    locale: "C"
                }
                Connections {
                    target: MySettings
                    function onMinPChanged() {
                        minPField.text = root.currentModelInfo.minP;
                    }
                }
                Connections {
                    target: root
                    function onCurrentModelInfoChanged() {
                        minPField.text = root.currentModelInfo.minP;
                    }
                }
                onEditingFinished: {
                    var val = parseFloat(text)
                    if (!isNaN(val)) {
                        MySettings.setModelMinP(root.currentModelInfo, val)
                        focus = false
                    } else {
                        text = root.currentModelInfo.minP
                    }
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: minPLabel.text
                Accessible.description: ToolTip.text
            }

            MySettingsLabel {
                id: topKLabel
                visible: !root.currentModelInfo.isOnline
                text: qsTr("Top-K")
                helpText: qsTr("Size of selection pool for tokens.")
                Layout.row: 2
                Layout.column: 2
                Layout.maximumWidth: 300 * theme.fontScale
            }
            MyTextField {
                id: topKField
                visible: !root.currentModelInfo.isOnline
                text: root.currentModelInfo.topK
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                ToolTip.text: qsTr("Only the top K most likely tokens will be chosen from.")
                ToolTip.visible: hovered
                Layout.row: 2
                Layout.column: 3
                validator: IntValidator {
                    bottom: 1
                }
                Connections {
                    target: MySettings
                    function onTopKChanged() {
                        topKField.text = root.currentModelInfo.topK;
                    }
                }
                Connections {
                    target: root
                    function onCurrentModelInfoChanged() {
                        topKField.text = root.currentModelInfo.topK;
                    }
                }
                onEditingFinished: {
                    var val = parseInt(text)
                    if (!isNaN(val)) {
                        MySettings.setModelTopK(root.currentModelInfo, val)
                        focus = false
                    } else {
                        text = root.currentModelInfo.topK
                    }
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: topKLabel.text
                Accessible.description: ToolTip.text
            }
            MySettingsLabel {
                id: maxLengthLabel
                visible: !root.currentModelInfo.isOnline
                text: qsTr("Max Length")
                helpText: qsTr("Maximum response length, in tokens.")
                Layout.row: 0
                Layout.column: 2
                Layout.maximumWidth: 300 * theme.fontScale
            }
            MyTextField {
                id: maxLengthField
                visible: !root.currentModelInfo.isOnline
                text: root.currentModelInfo.maxLength
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                Layout.row: 0
                Layout.column: 3
                validator: IntValidator {
                    bottom: 1
                }
                Connections {
                    target: MySettings
                    function onMaxLengthChanged() {
                        maxLengthField.text = root.currentModelInfo.maxLength;
                    }
                }
                Connections {
                    target: root
                    function onCurrentModelInfoChanged() {
                        maxLengthField.text = root.currentModelInfo.maxLength;
                    }
                }
                onEditingFinished: {
                    var val = parseInt(text)
                    if (!isNaN(val)) {
                        MySettings.setModelMaxLength(root.currentModelInfo, val)
                        focus = false
                    } else {
                        text = root.currentModelInfo.maxLength
                    }
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: maxLengthLabel.text
                Accessible.description: ToolTip.text
            }

            MySettingsLabel {
                id: batchSizeLabel
                visible: !root.currentModelInfo.isOnline
                text: qsTr("Prompt Batch Size")
                helpText: qsTr("The batch size used for prompt processing.")
                Layout.row: 1
                Layout.column: 0
                Layout.maximumWidth: 300 * theme.fontScale
            }
            MyTextField {
                id: batchSizeField
                visible: !root.currentModelInfo.isOnline
                text: root.currentModelInfo.promptBatchSize
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                ToolTip.text: qsTr("Amount of prompt tokens to process at once.\nNOTE: Higher values can speed up reading prompts but will use more RAM.")
                ToolTip.visible: hovered
                Layout.row: 1
                Layout.column: 1
                validator: IntValidator {
                    bottom: 1
                }
                Connections {
                    target: MySettings
                    function onPromptBatchSizeChanged() {
                        batchSizeField.text = root.currentModelInfo.promptBatchSize;
                    }
                }
                Connections {
                    target: root
                    function onCurrentModelInfoChanged() {
                        batchSizeField.text = root.currentModelInfo.promptBatchSize;
                    }
                }
                onEditingFinished: {
                    var val = parseInt(text)
                    if (!isNaN(val)) {
                        MySettings.setModelPromptBatchSize(root.currentModelInfo, val)
                        focus = false
                    } else {
                        text = root.currentModelInfo.promptBatchSize
                    }
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: batchSizeLabel.text
                Accessible.description: ToolTip.text
            }
            MySettingsLabel {
                id: repeatPenaltyLabel
                visible: !root.currentModelInfo.isOnline
                text: qsTr("Repeat Penalty")
                helpText: qsTr("Repetition penalty factor. Set to 1 to disable.")
                Layout.row: 4
                Layout.column: 2
                Layout.maximumWidth: 300 * theme.fontScale
            }
            MyTextField {
                id: repeatPenaltyField
                visible: !root.currentModelInfo.isOnline
                text: root.currentModelInfo.repeatPenalty
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                Layout.row: 4
                Layout.column: 3
                validator: DoubleValidator {
                    locale: "C"
                }
                Connections {
                    target: MySettings
                    function onRepeatPenaltyChanged() {
                        repeatPenaltyField.text = root.currentModelInfo.repeatPenalty;
                    }
                }
                Connections {
                    target: root
                    function onCurrentModelInfoChanged() {
                        repeatPenaltyField.text = root.currentModelInfo.repeatPenalty;
                    }
                }
                onEditingFinished: {
                    var val = parseFloat(text)
                    if (!isNaN(val)) {
                        MySettings.setModelRepeatPenalty(root.currentModelInfo, val)
                        focus = false
                    } else {
                        text = root.currentModelInfo.repeatPenalty
                    }
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: repeatPenaltyLabel.text
                Accessible.description: ToolTip.text
            }
            MySettingsLabel {
                id: repeatPenaltyTokensLabel
                visible: !root.currentModelInfo.isOnline
                text: qsTr("Repeat Penalty Tokens")
                helpText: qsTr("Number of previous tokens used for penalty.")
                Layout.row: 3
                Layout.column: 2
                Layout.maximumWidth: 300 * theme.fontScale
            }
            MyTextField {
                id: repeatPenaltyTokenField
                visible: !root.currentModelInfo.isOnline
                text: root.currentModelInfo.repeatPenaltyTokens
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                Layout.row: 3
                Layout.column: 3
                validator: IntValidator {
                    bottom: 1
                }
                Connections {
                    target: MySettings
                    function onRepeatPenaltyTokensChanged() {
                        repeatPenaltyTokenField.text = root.currentModelInfo.repeatPenaltyTokens;
                    }
                }
                Connections {
                    target: root
                    function onCurrentModelInfoChanged() {
                        repeatPenaltyTokenField.text = root.currentModelInfo.repeatPenaltyTokens;
                    }
                }
                onEditingFinished: {
                    var val = parseInt(text)
                    if (!isNaN(val)) {
                        MySettings.setModelRepeatPenaltyTokens(root.currentModelInfo, val)
                        focus = false
                    } else {
                        text = root.currentModelInfo.repeatPenaltyTokens
                    }
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: repeatPenaltyTokensLabel.text
                Accessible.description: ToolTip.text
            }

            MySettingsLabel {
                id: gpuLayersLabel
                visible: !root.currentModelInfo.isOnline
                text: qsTr("GPU Layers")
                helpText: qsTr("Number of model layers to load into VRAM.")
                Layout.row: 4
                Layout.column: 0
                Layout.maximumWidth: 300 * theme.fontScale
            }
            MyTextField {
                id: gpuLayersField
                visible: !root.currentModelInfo.isOnline
                text: root.currentModelInfo.gpuLayers
                font.pixelSize: theme.fontSizeLarge
                color: theme.textColor
                ToolTip.text: qsTr("How many model layers to load into VRAM. Decrease this if GPT4All runs out of VRAM while loading this model.\nLower values increase CPU load and RAM usage, and make inference slower.\nNOTE: Does not take effect until you reload the model.")
                ToolTip.visible: hovered
                Layout.row: 4
                Layout.column: 1
                Connections {
                    target: MySettings
                    function onGpuLayersChanged() {
                        gpuLayersField.text = root.currentModelInfo.gpuLayers
                    }
                }
                Connections {
                    target: root
                    function onCurrentModelInfoChanged() {
                        if (root.currentModelInfo.gpuLayers === 100) {
                            gpuLayersField.text = root.currentModelInfo.maxGpuLayers
                        } else {
                            gpuLayersField.text = root.currentModelInfo.gpuLayers
                        }
                    }
                }
                onEditingFinished: {
                    var val = parseInt(text)
                    if (isNaN(val)) {
                        gpuLayersField.text = root.currentModelInfo.gpuLayers
                    } else {
                        if (val < 1) {
                            val = 1
                            gpuLayersField.text = val
                        } else if (val > root.currentModelInfo.maxGpuLayers) {
                            val = root.currentModelInfo.maxGpuLayers
                            gpuLayersField.text = val
                        }
                        MySettings.setModelGpuLayers(root.currentModelInfo, val)
                        focus = false
                    }
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: gpuLayersLabel.text
                Accessible.description: ToolTip.text
            }
        }

        Rectangle {
            Layout.row: 16
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.topMargin: 15
            Layout.fillWidth: true
            height: 1
            color: theme.settingsDivider
        }
    }
}
