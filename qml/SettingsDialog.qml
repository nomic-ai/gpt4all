import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import download
import network
import llm

Dialog {
    id: settingsDialog
    modal: true
    height: 600
    width: 600
    opacity: 0.9
    background: Rectangle {
        anchors.fill: parent
        anchors.margins: -20
        color: "#202123"
        border.width: 1
        border.color: "white"
        radius: 10
    }

    property real defaultTemperature: 0.28
    property real defaultTopP: 0.95
    property int defaultTopK: 40
    property int defaultMaxLength: 4096
    property int defaultPromptBatchSize: 9
    property string defaultPromptTemplate: "The prompt below is a question to answer, a task to complete, or a conversation to respond to; decide which and write an appropriate response.
### Prompt:
%1
### Response:\n"

    property alias temperature: settings.temperature
    property alias topP: settings.topP
    property alias topK: settings.topK
    property alias maxLength: settings.maxLength
    property alias promptBatchSize: settings.promptBatchSize
    property alias promptTemplate: settings.promptTemplate

    Settings {
        id: settings
        property real temperature: settingsDialog.defaultTemperature
        property real topP: settingsDialog.defaultTopP
        property int topK: settingsDialog.defaultTopK
        property int maxLength: settingsDialog.defaultMaxLength
        property int promptBatchSize: settingsDialog.defaultPromptBatchSize
        property string promptTemplate: settingsDialog.defaultPromptTemplate
    }

    function restoreDefaults() {
        settings.temperature = defaultTemperature;
        settings.topP = defaultTopP;
        settings.topK = defaultTopK;
        settings.maxLength = defaultMaxLength;
        settings.promptBatchSize = defaultPromptBatchSize;
        settings.promptTemplate = defaultPromptTemplate;
        settings.sync()
    }

    Component.onDestruction: {
        settings.sync()
    }

    Item {
        Accessible.role: Accessible.Dialog
        Accessible.name: qsTr("Settings dialog")
        Accessible.description: qsTr("Dialog containing various settings for model text generation")
    }

    GridLayout {
        columns: 2
        rowSpacing: 2
        columnSpacing: 10
        anchors.fill: parent

        Label {
            id: tempLabel
            text: qsTr("Temperature:")
            color: "#d1d5db"
            Layout.row: 0
            Layout.column: 0
        }
        TextField {
            text: settings.temperature.toString()
            color: "#d1d5db"
            background: Rectangle {
                implicitWidth: 150
                color: "#40414f"
                radius: 10
            }
            ToolTip.text: qsTr("Temperature increases the chances of choosing less likely tokens - higher temperature gives more creative but less predictable outputs")
            ToolTip.visible: hovered
            Layout.row: 0
            Layout.column: 1
            validator: DoubleValidator { }
            onAccepted: {
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
             color: "#d1d5db"
            Layout.row: 1
            Layout.column: 0
        }
        TextField {
            text: settings.topP.toString()
            color: "#d1d5db"
            background: Rectangle {
                implicitWidth: 150
                color: "#40414f"
                radius: 10
            }
            ToolTip.text: qsTr("Only the most likely tokens up to a total probability of top_p can be chosen, prevents choosing highly unlikely tokens, aka Nucleus Sampling")
            ToolTip.visible: hovered
            Layout.row: 1
            Layout.column: 1
            validator: DoubleValidator {}
            onAccepted: {
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
             color: "#d1d5db"
             Layout.row: 2
             Layout.column: 0
         }
         TextField {
             text: settings.topK.toString()
             color: "#d1d5db"
             background: Rectangle {
                implicitWidth: 150
                color: "#40414f"
                radius: 10
             }
             ToolTip.text: qsTr("Only the top K most likely tokens will be chosen from")
             ToolTip.visible: hovered
             Layout.row: 2
             Layout.column: 1
             validator: IntValidator { bottom: 1 }
             onAccepted: {
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
             color: "#d1d5db"
             Layout.row: 3
             Layout.column: 0
         }
         TextField {
             text: settings.maxLength.toString()
             color: "#d1d5db"
             background: Rectangle {
                implicitWidth: 150
                color: "#40414f"
                radius: 10
             }
             ToolTip.text: qsTr("Maximum length of response in tokens")
             ToolTip.visible: hovered
             Layout.row: 3
             Layout.column: 1
             validator: IntValidator { bottom: 1 }
             onAccepted: {
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
             color: "#d1d5db"
             Layout.row: 4
             Layout.column: 0
         }
         TextField {
             text: settings.promptBatchSize.toString()
             color: "#d1d5db"
             background: Rectangle {
                implicitWidth: 150
                color: "#40414f"
                radius: 10
             }
             ToolTip.text: qsTr("Amount of prompt tokens to process at once, higher values can speed up reading prompts but will use more RAM")
             ToolTip.visible: hovered
             Layout.row: 4
             Layout.column: 1
             validator: IntValidator { bottom: 1 }
             onAccepted: {
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
             id: nThreadsLabel
             text: qsTr("CPU Threads")
             color: "#d1d5db"
             Layout.row: 5
             Layout.column: 0
         }
         TextField {
             text: LLM.threadCount.toString()
             color: "#d1d5db"
             background: Rectangle {
                implicitWidth: 150
                color: "#40414f"
                radius: 10
             }
             ToolTip.text: qsTr("Amount of processing threads to use")
             ToolTip.visible: hovered
             Layout.row: 5
             Layout.column: 1
             validator: IntValidator { bottom: 1 }
             onAccepted: {
                 var val = parseInt(text)
                 if (!isNaN(val)) {
                     LLM.threadCount = val
                     focus = false
                 } else {
                     text = settingsDialog.nThreads.toString()
                 }
             }
            Accessible.role: Accessible.EditableText
            Accessible.name: nThreadsLabel.text
            Accessible.description: ToolTip.text
         }

         Label {
             id: promptTemplateLabel
             text: qsTr("Prompt Template:")
             color: "#d1d5db"
             Layout.row: 6
             Layout.column: 0
         }
         Rectangle {
             Layout.row: 6
             Layout.column: 1
             Layout.fillWidth: true
             height: 200
             color: "transparent"
             border.width: 1
             border.color: "#ccc"
             radius: 5
             Label {
                id: promptTemplateLabelHelp
                visible: settings.promptTemplate.indexOf("%1") === -1
                font.bold: true
                color: "red"
                text: qsTr("Prompt template must contain %1 to be replaced with the user's input.")
                anchors.bottom: templateScrollView.top
                Accessible.role: Accessible.EditableText
                Accessible.name: text
             }
             ScrollView {
                 id: templateScrollView
                 anchors.fill: parent
                 TextArea {
                     text: settings.promptTemplate
                     color: "#d1d5db"
                     background: Rectangle {
                        implicitWidth: 150
                        color: "#40414f"
                        radius: 10
                     }
                     wrapMode: TextArea.Wrap
                     onTextChanged: {
                         settings.promptTemplate = text
                         settings.sync()
                     }
                     bottomPadding: 10
                     Accessible.role: Accessible.EditableText
                     Accessible.name: promptTemplateLabel.text
                     Accessible.description: promptTemplateLabelHelp.text
                 }
             }
         }
         Button {
             Layout.row: 7
             Layout.column: 1
             Layout.fillWidth: true
             padding: 15
             contentItem: Text {
                 text: qsTr("Restore Defaults")
                 horizontalAlignment: Text.AlignHCenter
                 color: "#d1d5db"
                 Accessible.role: Accessible.Button
                 Accessible.name: text
                 Accessible.description: qsTr("Restores the settings dialog to a default state")
             }

             background: Rectangle {
                 opacity: .5
                 border.color: "#7d7d8e"
                 border.width: 1
                 radius: 10
                 color: "#343541"
             }
             onClicked: {
                 settingsDialog.restoreDefaults()
             }
         }
    }
}
