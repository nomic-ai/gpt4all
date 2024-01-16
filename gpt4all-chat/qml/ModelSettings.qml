import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import modellist
import mysettings

MySettingsTab {
    onRestoreDefaultsClicked: {
        MySettings.restoreModelDefaults(root.currentModelInfo);
    }
    title: qsTr("Model/Character Settings")
    contentItem: GridLayout {
        id: root
        columns: 3
        rowSpacing: 10
        columnSpacing: 10

        property var currentModelName: comboBox.currentText
        property var currentModelId: comboBox.currentValue
        property var currentModelInfo: ModelList.modelInfo(root.currentModelId)

        Label {
            id: label
            Layout.row: 0
            Layout.column: 0
            text: qsTr("Model/Character:")
            color: theme.textColor
            font.pixelSize: theme.fontSizeLarge
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.row: 1
            Layout.column: 0
            Layout.columnSpan: 2
            height: label.height + 20
            spacing: 10

            MyComboBox {
                id: comboBox
                Layout.fillWidth: true
                model: ModelList.installedModels
                valueRole: "id"
                textRole: "name"
                currentIndex: 0
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
                    width: comboBox.width
                    contentItem: Text {
                        text: name
                        color: theme.textColor
                        font: comboBox.font
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: highlighted ? theme.backgroundLight : theme.backgroundDark
                    }
                    highlighted: comboBox.highlightedIndex === index
                }
            }

            MyButton {
                id: cloneButton
                text: qsTr("Clone")
                onClicked: {
                    var id = ModelList.clone(root.currentModelInfo);
                    comboBox.currentIndex = comboBox.indexOfValue(id);
                }
            }

            MyButton {
                id: removeButton
                enabled: root.currentModelInfo.isClone
                text: qsTr("Remove")
                onClicked: {
                    ModelList.remove(root.currentModelInfo);
                    comboBox.currentIndex = 0;
                }
            }
        }

        RowLayout {
            Layout.row: 2
            Layout.column: 0
            Layout.topMargin: 15
            spacing: 10
            Label {
                id: uniqueNameLabel
                text: qsTr("Unique Name:")
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
            }
            Label {
                id: uniqueNameLabelHelp
                visible: false
                text: qsTr("Must contain a non-empty unique name that does not match any existing model/character.")
                color: theme.textErrorColor
                font.pixelSize: theme.fontSizeLarge
                wrapMode: TextArea.Wrap
            }
        }

        MyTextField {
            id: uniqueNameField
            text: root.currentModelName
            font.pixelSize: theme.fontSizeLarge
            enabled: root.currentModelInfo.isClone || root.currentModelInfo.description === ""
            color: enabled ? theme.textColor : theme.mutedTextColor
            Layout.row: 3
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
                uniqueNameLabelHelp.visible = root.currentModelInfo.name !== "" &&
                    (text === "" || (text !== root.currentModelInfo.name && !ModelList.isUniqueName(text)));
            }
        }

        Label {
            text: qsTr("Model File:")
            color: theme.textColor
            font.pixelSize: theme.fontSizeLarge
            Layout.row: 4
            Layout.column: 0
            Layout.topMargin: 15
        }

        MyTextField {
            text: root.currentModelInfo.filename
            font.pixelSize: theme.fontSizeLarge
            enabled: false
            color: enabled ? theme.textColor : theme.mutedTextColor
            Layout.row: 5
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.fillWidth: true
        }

        Label {
            visible: !root.currentModelInfo.isChatGPT
            text: qsTr("System Prompt:")
            color: theme.textColor
            font.pixelSize: theme.fontSizeLarge
            Layout.row: 6
            Layout.column: 0
            Layout.topMargin: 15
        }

        Rectangle {
            id: systemPrompt
            visible: !root.currentModelInfo.isChatGPT
            Layout.row: 7
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.fillWidth: true
            color: "transparent"
            Layout.minimumHeight: Math.max(100, systemPromptArea.contentHeight + 20)
            TextArea {
                id: systemPromptArea
                anchors.fill: parent
                text: root.currentModelInfo.systemPrompt
                color: theme.textColor
                background: Rectangle {
                    implicitWidth: 150
                    color: theme.backgroundDark
                    radius: 10
                }
                padding: 10
                wrapMode: TextArea.Wrap
                Connections {
                    target: MySettings
                    function onSystemPromptChanged() {
                        systemPromptArea.text = root.currentModelInfo.systemPrompt;
                    }
                }
                Connections {
                    target: root
                    function onCurrentModelInfoChanged() {
                        systemPromptArea.text = root.currentModelInfo.systemPrompt;
                    }
                }
                onTextChanged: {
                    MySettings.setModelSystemPrompt(root.currentModelInfo, text)
                }
                bottomPadding: 10
                Accessible.role: Accessible.EditableText
                ToolTip.text: qsTr("The systemPrompt allows instructions to the model at the beginning of a chat.\nNOTE: A longer, detailed system prompt can lead to higher quality answers, but can also slow down generation.")
                ToolTip.visible: hovered
            }
        }

        RowLayout {
            Layout.row: 8
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.topMargin: 15
            spacing: 10
            Label {
                id: promptTemplateLabel
                text: qsTr("Prompt Template:")
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
            }
            Label {
                id: promptTemplateLabelHelp
                text: qsTr("Must contain the string \"%1\" to be replaced with the user's input.")
                color: theme.textErrorColor
                font.pixelSize: theme.fontSizeLarge
                visible: templateTextArea.text.indexOf("%1") === -1
                wrapMode: TextArea.Wrap
            }
        }

        Rectangle {
            id: promptTemplate
            Layout.row: 9
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.minimumHeight: Math.max(100, templateTextArea.contentHeight + 20)
            color: "transparent"
            clip: true
            TextArea {
                id: templateTextArea
                anchors.fill: parent
                text: root.currentModelInfo.promptTemplate
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                background: Rectangle {
                    implicitWidth: 150
                    color: theme.backgroundDark
                    radius: 10
                }
                padding: 10
                wrapMode: TextArea.Wrap
                Connections {
                    target: MySettings
                    function onPromptTemplateChanged() {
                        templateTextArea.text = root.currentModelInfo.promptTemplate;
                    }
                }
                Connections {
                    target: root
                    function onCurrentModelInfoChanged() {
                        templateTextArea.text = root.currentModelInfo.promptTemplate;
                    }
                }
                onTextChanged: {
                    if (templateTextArea.text.indexOf("%1") !== -1) {
                        MySettings.setModelPromptTemplate(root.currentModelInfo, text)
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

        Rectangle {
            id: optionalImageRect
            visible: false // FIXME: for later
            Layout.row: 2
            Layout.column: 1
            Layout.rowSpan: 5
            Layout.alignment: Qt.AlignHCenter
            Layout.fillHeight: true
            Layout.maximumWidth: height
            Layout.topMargin: 35
            Layout.bottomMargin: 35
            Layout.leftMargin: 35
            width: 3000
            border.width: 1
            border.color: theme.tabBorder
            radius: 10
            color: "transparent"
            Item {
                anchors.centerIn: parent
                height: childrenRect.height
                Image {
                    id: img
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 100
                    height: 100
                    source: "qrc:/gpt4all/icons/image.svg"
                }
                Text {
                    text: qsTr("Add\noptional image")
                    font.pixelSize: theme.fontSizeLarge
                    anchors.top: img.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    wrapMode: TextArea.Wrap
                    horizontalAlignment: Qt.AlignHCenter
                    color: theme.mutedTextColor
                }
            }
        }

        Label {
            text: qsTr("Generation Settings")
            color: theme.textColor
            Layout.row: 10
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.topMargin: 15
            Layout.alignment: Qt.AlignHCenter
            Layout.minimumWidth: promptTemplate.width
            horizontalAlignment: Qt.AlignHCenter
            font.pixelSize: theme.fontSizeLarger
            font.bold: true
        }

        GridLayout {
            Layout.row: 11
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.topMargin: 15
            Layout.fillWidth: true
            Layout.minimumWidth: promptTemplate.width
            columns: 4
            rowSpacing: 10
            columnSpacing: 10

            Label {
                id: contextLengthLabel
                visible: !root.currentModelInfo.isChatGPT
                text: qsTr("Context Length:")
                font.pixelSize: theme.fontSizeLarge
                color: theme.textColor
                Layout.row: 0
                Layout.column: 0
            }
            MyTextField {
                id: contextLengthField
                visible: !root.currentModelInfo.isChatGPT
                text: root.currentModelInfo.contextLength
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                ToolTip.text: qsTr("Maximum combined prompt/response tokens before information is lost.\nUsing more context than the model was trained on will yield poor results.\nNOTE: Does not take effect until you RESTART GPT4All or SWITCH MODELS.")
                ToolTip.visible: hovered
                Layout.row: 0
                Layout.column: 1
                validator: IntValidator {
                    bottom: 1
                }
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
                    if (!isNaN(val)) {
                        MySettings.setModelContextLength(root.currentModelInfo, val)
                        focus = false
                    } else {
                        text = root.currentModelInfo.contextLength
                    }
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: contextLengthLabel.text
                Accessible.description: ToolTip.text
            }

            Label {
                id: tempLabel
                text: qsTr("Temperature:")
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                Layout.row: 1
                Layout.column: 2
            }

            MyTextField {
                id: temperatureField
                text: root.currentModelInfo.temperature
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
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
            Label {
                id: topPLabel
                text: qsTr("Top P:")
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                Layout.row: 2
                Layout.column: 0
            }
            MyTextField {
                id: topPField
                text: root.currentModelInfo.topP
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                ToolTip.text: qsTr("Only the most likely tokens up to a total probability of top_p can be chosen.\nNOTE: Prevents choosing highly unlikely tokens, aka Nucleus Sampling")
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
            Label {
                id: topKLabel
                visible: !root.currentModelInfo.isChatGPT
                text: qsTr("Top K:")
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                Layout.row: 2
                Layout.column: 2
            }
            MyTextField {
                id: topKField
                visible: !root.currentModelInfo.isChatGPT
                text: root.currentModelInfo.topK
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                ToolTip.text: qsTr("Only the top K most likely tokens will be chosen from")
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
            Label {
                id: maxLengthLabel
                visible: !root.currentModelInfo.isChatGPT
                text: qsTr("Max Length:")
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                Layout.row: 0
                Layout.column: 2
            }
            MyTextField {
                id: maxLengthField
                visible: !root.currentModelInfo.isChatGPT
                text: root.currentModelInfo.maxLength
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                ToolTip.text: qsTr("Maximum length of response in tokens")
                ToolTip.visible: hovered
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

            Label {
                id: batchSizeLabel
                visible: !root.currentModelInfo.isChatGPT
                text: qsTr("Prompt Batch Size:")
                font.pixelSize: theme.fontSizeLarge
                color: theme.textColor
                Layout.row: 1
                Layout.column: 0
            }
            MyTextField {
                id: batchSizeField
                visible: !root.currentModelInfo.isChatGPT
                text: root.currentModelInfo.promptBatchSize
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                ToolTip.text: qsTr("Amount of prompt tokens to process at once.\nNOTE: Higher values can speed up reading prompts but will use more RAM")
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
            Label {
                id: repeatPenaltyLabel
                visible: !root.currentModelInfo.isChatGPT
                text: qsTr("Repeat Penalty:")
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                Layout.row: 3
                Layout.column: 0
            }
            MyTextField {
                id: repeatPenaltyField
                visible: !root.currentModelInfo.isChatGPT
                text: root.currentModelInfo.repeatPenalty
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                ToolTip.text: qsTr("Amount to penalize repetitiveness of the output")
                ToolTip.visible: hovered
                Layout.row: 3
                Layout.column: 1
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
            Label {
                id: repeatPenaltyTokensLabel
                visible: !root.currentModelInfo.isChatGPT
                text: qsTr("Repeat Penalty Tokens:")
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                Layout.row: 3
                Layout.column: 2
            }
            MyTextField {
                id: repeatPenaltyTokenField
                visible: !root.currentModelInfo.isChatGPT
                text: root.currentModelInfo.repeatPenaltyTokens
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                ToolTip.text: qsTr("How far back in output to apply repeat penalty")
                ToolTip.visible: hovered
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
        }

        Rectangle {
            Layout.row: 12
            Layout.column: 0
            Layout.columnSpan: 2
            Layout.topMargin: 15
            Layout.fillWidth: true
            Layout.minimumWidth: promptTemplate.width
            height: 1
            color: theme.tabBorder
        }
    }
}
