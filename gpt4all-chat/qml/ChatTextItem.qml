import Qt5Compat.GraphicalEffects
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import gpt4all
import mysettings
import toolenums

TextArea {
    id: myTextArea
    property string textContent: ""
    visible: textContent != ""
    Layout.fillWidth: true
    padding: 0
    color: {
        if (!currentChat.isServer)
            return theme.textColor
        return theme.white
    }
    wrapMode: Text.WordWrap
    textFormat: TextEdit.PlainText
    focus: false
    readOnly: true
    font.pixelSize: theme.fontSizeLarge
    cursorVisible: isCurrentResponse ? currentChat.responseInProgress : false
    cursorPosition: text.length
    TapHandler {
        id: tapHandler
        onTapped: function(eventPoint, button) {
            var clickedPos = myTextArea.positionAt(eventPoint.position.x, eventPoint.position.y);
            var success = textProcessor.tryCopyAtPosition(clickedPos);
            if (success)
                copyCodeMessage.open();
        }
    }

    MouseArea {
        id: conversationMouseArea
        anchors.fill: parent
        acceptedButtons: Qt.RightButton

        onClicked: (mouse) => {
                       if (mouse.button === Qt.RightButton) {
                           conversationContextMenu.x = conversationMouseArea.mouseX
                           conversationContextMenu.y = conversationMouseArea.mouseY
                           conversationContextMenu.open()
                       }
                   }
    }

    onLinkActivated: function(link) {
        if (!isCurrentResponse || !currentChat.responseInProgress)
            Qt.openUrlExternally(link)
    }

    onLinkHovered: function (link) {
        if (!isCurrentResponse || !currentChat.responseInProgress)
            statusBar.externalHoveredLink = link
    }

    MyMenu {
        id: conversationContextMenu
        MyMenuItem {
            text: qsTr("Copy")
            enabled: myTextArea.selectedText !== ""
            height: enabled ? implicitHeight : 0
            onTriggered: myTextArea.copy()
        }
        MyMenuItem {
            text: qsTr("Copy Message")
            enabled: myTextArea.selectedText === ""
            height: enabled ? implicitHeight : 0
            onTriggered: {
                myTextArea.selectAll()
                myTextArea.copy()
                myTextArea.deselect()
            }
        }
        MyMenuItem {
            text: textProcessor.shouldProcessText ? qsTr("Disable markdown") : qsTr("Enable markdown")
            height: enabled ? implicitHeight : 0
            onTriggered: {
                textProcessor.shouldProcessText = !textProcessor.shouldProcessText;
                textProcessor.setValue(textContent);
            }
        }
    }

    ChatViewTextProcessor {
        id: textProcessor
    }

    function resetChatViewTextProcessor() {
        textProcessor.fontPixelSize                = myTextArea.font.pixelSize
        textProcessor.codeColors.defaultColor      = theme.codeDefaultColor
        textProcessor.codeColors.keywordColor      = theme.codeKeywordColor
        textProcessor.codeColors.functionColor     = theme.codeFunctionColor
        textProcessor.codeColors.functionCallColor = theme.codeFunctionCallColor
        textProcessor.codeColors.commentColor      = theme.codeCommentColor
        textProcessor.codeColors.stringColor       = theme.codeStringColor
        textProcessor.codeColors.numberColor       = theme.codeNumberColor
        textProcessor.codeColors.headerColor       = theme.codeHeaderColor
        textProcessor.codeColors.backgroundColor   = theme.codeBackgroundColor
        textProcessor.textDocument                 = textDocument
        textProcessor.setValue(textContent);
    }

    property bool textProcessorReady: false

    Component.onCompleted: {
        resetChatViewTextProcessor();
        textProcessorReady = true;
    }

    Connections {
        target: myTextArea
        function onTextContentChanged() {
            if (myTextArea.textProcessorReady)
                textProcessor.setValue(textContent);
        }
    }

    Connections {
        target: MySettings
        function onFontSizeChanged() {
            myTextArea.resetChatViewTextProcessor();
        }
        function onChatThemeChanged() {
            myTextArea.resetChatViewTextProcessor();
        }
    }

    Accessible.role: Accessible.Paragraph
    Accessible.name: text
    Accessible.description: name === "Response: " ? "The response by the model" : "The prompt by the user"
}
