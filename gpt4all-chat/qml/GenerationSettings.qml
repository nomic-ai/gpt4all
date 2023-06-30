import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import localdocs
import mysettings

MySettingsTab {
    title: qsTr("Presets")
    contentItem: GridLayout {
        id: generationSettingsTabInner
        columns: 2
        rowSpacing: 10
        columnSpacing: 10

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
                MySettings.restoreGenerationDefaults();
                templateTextArea.text = MySettings.promptTemplate
            }
        }
    }
}