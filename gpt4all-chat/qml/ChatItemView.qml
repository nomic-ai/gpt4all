import Qt5Compat.GraphicalEffects
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qt.labs.qmlmodels

import gpt4all
import mysettings
import toolenums

ColumnLayout {

property var inputBoxText: null
signal setInputBoxText(text: string)

Item {

Layout.fillWidth: true
Layout.maximumWidth: parent.width
Layout.preferredHeight: gridLayout.height

HoverHandler { id: hoverArea }

GridLayout {
    id: gridLayout
    anchors.left: parent.left
    anchors.right: parent.right
    columns: 2

    Item {
        Layout.row: 0
        Layout.column: 0
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
        Layout.preferredWidth: 32
        Layout.preferredHeight: 32
        Layout.topMargin: model.index > 0 ? 25 : 0

        Image {
            id: logo
            sourceSize: Qt.size(32, 32)
            fillMode: Image.PreserveAspectFit
            mipmap: true
            visible: false
            source: name !== "Response: " ? "qrc:/gpt4all/icons/you.svg" : "qrc:/gpt4all/icons/gpt4all_transparent.svg"
        }

        ColorOverlay {
            id: colorOver
            anchors.fill: logo
            source: logo
            color: theme.conversationHeader
            RotationAnimation {
                id: rotationAnimation
                target: colorOver
                property: "rotation"
                from: 0
                to: 360
                duration: 1000
                loops: Animation.Infinite
                running: isCurrentResponse && currentChat.responseInProgress
            }
        }
    }

    Item {
        Layout.row: 0
        Layout.column: 1
        Layout.fillWidth: true
        Layout.preferredHeight: 38
        Layout.topMargin: model.index > 0 ? 25 : 0

        RowLayout {
            spacing: 5
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            TextArea {
                text: {
                    if (name === "Response: ")
                        return qsTr("GPT4All");
                    return qsTr("You");
                }
                padding: 0
                font.pixelSize: theme.fontSizeLarger
                font.bold: true
                color: theme.conversationHeader
                enabled: false
                focus: false
                readOnly: true
            }
            Text {
                visible: name === "Response: "
                font.pixelSize: theme.fontSizeLarger
                text: currentModelName()
                color: theme.mutedTextColor
            }
            RowLayout {
                visible: isCurrentResponse && (content === "" && currentChat.responseInProgress)
                Text {
                    color: theme.mutedTextColor
                    font.pixelSize: theme.fontSizeLarger
                    text: {
                        switch (currentChat.responseState) {
                        case Chat.ResponseStopped: return qsTr("response stopped ...");
                        case Chat.LocalDocsRetrieval: return qsTr("retrieving localdocs: %1 ...").arg(currentChat.collectionList.join(", "));
                        case Chat.LocalDocsProcessing: return qsTr("searching localdocs: %1 ...").arg(currentChat.collectionList.join(", "));
                        case Chat.PromptProcessing: return qsTr("processing ...")
                        case Chat.ResponseGeneration: return qsTr("generating response ...");
                        case Chat.GeneratingQuestions: return qsTr("generating questions ...");
                        default: return ""; // handle unexpected values
                        }
                    }
                }
            }
        }
    }

    ColumnLayout {
        Layout.row: 1
        Layout.column: 1
        Layout.fillWidth: true
        spacing: 10
        Flow {
            id: attachedUrlsFlow
            Layout.fillWidth: true
            Layout.bottomMargin: 10
            spacing: 10
            visible: promptAttachments.length !== 0
            Repeater {
                model: promptAttachments

                delegate: Rectangle {
                    width: 350
                    height: 50
                    radius: 5
                    color: theme.attachmentBackground
                    border.color: theme.controlBorder

                    Row {
                        spacing: 5
                        anchors.fill: parent
                        anchors.margins: 5

                        MyFileIcon {
                            iconSize: 40
                            fileName: modelData.file
                        }

                        Text {
                            width: 295
                            height: 40
                            text: modelData.file
                            color: theme.textColor
                            horizontalAlignment: Text.AlignHLeft
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: theme.fontSizeMedium
                            font.bold: true
                            wrapMode: Text.WrapAnywhere
                            elide: Qt.ElideRight
                        }
                    }
                }
            }
        }

        Repeater {
            model: childItems

            DelegateChooser {
                id: chooser
                role: "name"
                DelegateChoice {
                    roleValue: "Text: ";
                    ChatTextItem {
                        Layout.fillWidth: true
                        textContent: modelData.content
                    }
                }
                DelegateChoice {
                    roleValue: "ToolCall: ";
                    ChatCollapsibleItem {
                        Layout.fillWidth: true
                        textContent: modelData.content
                        isCurrent: modelData.isCurrentResponse
                        isError: modelData.isToolCallError
                    }
                }
            }

            delegate: chooser
        }

        ChatTextItem {
            Layout.fillWidth: true
            textContent: content
        }

        ThumbsDownDialog {
            id: thumbsDownDialog
            x: Math.round((parent.width - width) / 2)
            y: Math.round((parent.height - height) / 2)
            width: 640
            height: 300
            property string text: content
            response: newResponse === undefined || newResponse === "" ? text : newResponse
            onAccepted: {
                var responseHasChanged = response !== text && response !== newResponse
                if (thumbsDownState && !thumbsUpState && !responseHasChanged)
                    return

                chatModel.updateNewResponse(model.index, response)
                chatModel.updateThumbsUpState(model.index, false)
                chatModel.updateThumbsDownState(model.index, true)
                Network.sendConversation(currentChat.id, getConversationJson());
            }
        }
    }

    Item {
        Layout.row: 2
        Layout.column: 1
        Layout.topMargin: 5
        Layout.alignment: Qt.AlignVCenter
        Layout.preferredWidth: childrenRect.width
        Layout.preferredHeight: childrenRect.height
        visible: {
            if (name !== "Response: ")
                return false
            if (consolidatedSources.length === 0)
                return false
            if (!MySettings.localDocsShowReferences)
                return false
            if (isCurrentResponse && currentChat.responseInProgress
                    && currentChat.responseState !== Chat.GeneratingQuestions )
                return false
            return true
        }

        MyButton {
            backgroundColor: theme.sourcesBackground
            backgroundColorHovered: theme.sourcesBackgroundHovered
            contentItem: RowLayout {
                anchors.centerIn: parent

                Item {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24

                    Image {
                        id: sourcesIcon
                        visible: false
                        anchors.fill: parent
                        sourceSize.width: 24
                        sourceSize.height: 24
                        mipmap: true
                        source: "qrc:/gpt4all/icons/db.svg"
                    }

                    ColorOverlay {
                        anchors.fill: sourcesIcon
                        source: sourcesIcon
                        color: theme.textColor
                    }
                }

                Text {
                    text: qsTr("%n Source(s)", "", consolidatedSources.length)
                    padding: 0
                    font.pixelSize: theme.fontSizeLarge
                    font.bold: true
                    color: theme.styledTextColor
                }

                Item {
                    Layout.preferredWidth: caret.width
                    Layout.preferredHeight: caret.height
                    Image {
                        id: caret
                        anchors.centerIn: parent
                        visible: false
                        sourceSize.width: theme.fontSizeLarge
                        sourceSize.height: theme.fontSizeLarge
                        mipmap: true
                        source: {
                            if (sourcesLayout.state === "collapsed")
                                return "qrc:/gpt4all/icons/caret_right.svg";
                            else
                                return "qrc:/gpt4all/icons/caret_down.svg";
                        }
                    }

                    ColorOverlay {
                        anchors.fill: caret
                        source: caret
                        color: theme.textColor
                    }
                }
            }

            onClicked: {
                if (sourcesLayout.state === "collapsed")
                    sourcesLayout.state = "expanded";
                else
                    sourcesLayout.state = "collapsed";
            }
        }
    }

    ColumnLayout {
        id: sourcesLayout
        Layout.row: 3
        Layout.column: 1
        Layout.topMargin: 5
        visible: {
            if (consolidatedSources.length === 0)
                return false
            if (!MySettings.localDocsShowReferences)
                return false
            if (isCurrentResponse && currentChat.responseInProgress
                    && currentChat.responseState !== Chat.GeneratingQuestions )
                return false
            return true
        }
        clip: true
        Layout.fillWidth: true
        Layout.preferredHeight: 0
        state: "collapsed"
        states: [
            State {
                name: "expanded"
                PropertyChanges { target: sourcesLayout; Layout.preferredHeight: sourcesFlow.height }
            },
            State {
                name: "collapsed"
                PropertyChanges { target: sourcesLayout; Layout.preferredHeight: 0 }
            }
        ]

        transitions: [
            Transition {
                SequentialAnimation {
                    PropertyAnimation {
                        target: sourcesLayout
                        property: "Layout.preferredHeight"
                        duration: 300
                        easing.type: Easing.InOutQuad
                    }
                }
            }
        ]

        Flow {
            id: sourcesFlow
            Layout.fillWidth: true
            spacing: 10
            visible: consolidatedSources.length !== 0
            Repeater {
                model: consolidatedSources

                delegate: Rectangle {
                    radius: 10
                    color: ma.containsMouse ? theme.sourcesBackgroundHovered : theme.sourcesBackground
                    width: 200
                    height: 75

                    MouseArea {
                        id: ma
                        enabled: modelData.path !== ""
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: function() {
                            Qt.openUrlExternally(modelData.fileUri)
                        }
                    }

                    Rectangle {
                        id: debugTooltip
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        width: 24
                        height: 24
                        color: "transparent"
                        ToolTip {
                            parent: debugTooltip
                            visible: debugMouseArea.containsMouse
                            text: modelData.text
                            contentWidth: 900
                            delay: 500
                        }
                        MouseArea {
                            id: debugMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                        }
                    }

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.margins: 10
                        spacing: 0
                        RowLayout {
                            id: title
                            spacing: 5
                            Layout.maximumWidth: 180
                            MyFileIcon {
                                iconSize: 24
                                fileName: modelData.file
                                Layout.preferredWidth: iconSize
                                Layout.preferredHeight: iconSize
                            }
                            Text {
                                Layout.maximumWidth: 156
                                text: modelData.collection !== "" ? modelData.collection : qsTr("LocalDocs")
                                font.pixelSize: theme.fontSizeLarge
                                font.bold: true
                                color: theme.styledTextColor
                                elide: Qt.ElideRight
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                color: "transparent"
                                height: 1
                            }
                        }
                        Text {
                            Layout.fillHeight: true
                            Layout.maximumWidth: 180
                            Layout.maximumHeight: 55 - title.height
                            text: modelData.file
                            color: theme.textColor
                            font.pixelSize: theme.fontSizeSmall
                            elide: Qt.ElideRight
                            wrapMode: Text.WrapAnywhere
                        }
                    }
                }
            }
        }
    }

    ConfirmationDialog {
        id: editPromptDialog
        dialogTitle: qsTr("Edit this message?")
        description: qsTr("All following messages will be permanently erased.")
        onAccepted: {
            const msg = currentChat.popPrompt(index);
            if (msg !== null)
                setInputBoxText(msg);
        }
    }

    ConfirmationDialog {
        id: redoResponseDialog
        dialogTitle: qsTr("Redo this response?")
        description: qsTr("All following messages will be permanently erased.")
        onAccepted: currentChat.regenerateResponse(index)
    }

    RowLayout {
        id: buttonRow
        Layout.row: 4
        Layout.column: 1
        Layout.maximumWidth: parent.width
        Layout.fillWidth: false
        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
        spacing: 3
        visible: !isCurrentResponse || !currentChat.responseInProgress
        enabled: opacity > 0
        opacity: hoverArea.hovered

        Behavior on opacity {
            OpacityAnimator { duration: 30 }
        }

        ChatMessageButton {
            readonly property var editingDisabledReason: {
                if (!currentChat.isModelLoaded)
                    return qsTr("Cannot edit chat without a loaded model.");
                if (currentChat.responseInProgress)
                    return qsTr("Cannot edit chat while the model is generating.");
                return null;
            }
            visible: !currentChat.isServer && model.name === "Prompt: "
            enabled: editingDisabledReason === null
            Layout.maximumWidth: 24
            Layout.maximumHeight: 24
            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: false
            name: editingDisabledReason ?? qsTr("Edit")
            source: "qrc:/gpt4all/icons/edit.svg"
            onClicked: {
                if (inputBoxText === "")
                    editPromptDialog.open();
            }
        }

        ChatMessageButton {
            readonly property var editingDisabledReason: {
                if (!currentChat.isModelLoaded)
                    return qsTr("Cannot redo response without a loaded model.");
                if (currentChat.responseInProgress)
                    return qsTr("Cannot redo response while the model is generating.");
                return null;
            }
            visible: !currentChat.isServer && model.name === "Response: "
            enabled: editingDisabledReason === null
            Layout.maximumWidth: 24
            Layout.maximumHeight: 24
            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: false
            name: editingDisabledReason ?? qsTr("Redo")
            source: "qrc:/gpt4all/icons/regenerate.svg"
            onClicked: {
                if (index == chatModel.count - 1) {
                    // regenerate last message without confirmation
                    currentChat.regenerateResponse(index);
                    return;
                }
                redoResponseDialog.open();
            }
        }

        ChatMessageButton {
            Layout.maximumWidth: 24
            Layout.maximumHeight: 24
            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: false
            name: qsTr("Copy")
            source: "qrc:/gpt4all/icons/copy.svg"
            onClicked: {
                chatModel.copyToClipboard(index);
            }
        }

        Item {
            visible: name === "Response: " && MySettings.networkIsActive
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: childrenRect.width
            Layout.preferredHeight: childrenRect.height
            Layout.fillWidth: false

            ChatMessageButton {
                id: thumbsUp
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                opacity: thumbsUpState || thumbsUpState == thumbsDownState ? 1.0 : 0.2
                source: "qrc:/gpt4all/icons/thumbs_up.svg"
                name: qsTr("Like response")
                onClicked: {
                    if (thumbsUpState && !thumbsDownState)
                        return

                    chatModel.updateNewResponse(index, "")
                    chatModel.updateThumbsUpState(index, true)
                    chatModel.updateThumbsDownState(index, false)
                    Network.sendConversation(currentChat.id, getConversationJson());
                }
            }

            ChatMessageButton {
                id: thumbsDown
                anchors.top: thumbsUp.top
                anchors.topMargin: buttonRow.spacing
                anchors.left: thumbsUp.right
                anchors.leftMargin: buttonRow.spacing
                checked: thumbsDownState
                opacity: thumbsDownState || thumbsUpState == thumbsDownState ? 1.0 : 0.2
                bgTransform: [
                    Matrix4x4 {
                        matrix: Qt.matrix4x4(-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
                    },
                    Translate {
                        x: thumbsDown.width
                    }
                ]
                source: "qrc:/gpt4all/icons/thumbs_down.svg"
                name: qsTr("Dislike response")
                onClicked: {
                    thumbsDownDialog.open()
                }
            }
        }
    }
} // GridLayout

} // Item

GridLayout {
    Layout.fillWidth: true
    Layout.maximumWidth: parent.width

    function shouldShowSuggestions() {
        if (!isCurrentResponse)
            return false;
        if (MySettings.suggestionMode === 2) // Off
            return false;
        if (MySettings.suggestionMode === 0 && consolidatedSources.length === 0) // LocalDocs only
            return false;
        return currentChat.responseState === Chat.GeneratingQuestions || currentChat.generatedQuestions.length !== 0;
    }

    Item {
        visible: parent.shouldShowSuggestions()
        Layout.row: 5
        Layout.column: 0
        Layout.topMargin: 20
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
        Layout.preferredWidth: 28
        Layout.preferredHeight: 28
        Image {
            id: stack
            sourceSize: Qt.size(28, 28)
            fillMode: Image.PreserveAspectFit
            mipmap: true
            visible: false
            source: "qrc:/gpt4all/icons/stack.svg"
        }

        ColorOverlay {
            anchors.fill: stack
            source: stack
            color: theme.conversationHeader
        }
    }

    Item {
        visible: parent.shouldShowSuggestions()
        Layout.row: 5
        Layout.column: 1
        Layout.topMargin: 20
        Layout.fillWidth: true
        Layout.preferredHeight: 38
        RowLayout {
            spacing: 5
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            TextArea {
                text: qsTr("Suggested follow-ups")
                padding: 0
                font.pixelSize: theme.fontSizeLarger
                font.bold: true
                color: theme.conversationHeader
                enabled: false
                focus: false
                readOnly: true
            }
        }
    }

    ColumnLayout {
        visible: parent.shouldShowSuggestions()
        Layout.row: 6
        Layout.column: 1
        Layout.fillWidth: true
        Layout.minimumHeight: 1
        spacing: 10
        Repeater {
            model: currentChat.generatedQuestions
            TextArea {
                id: followUpText
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft
                rightPadding: 40
                topPadding: 10
                leftPadding: 20
                bottomPadding: 10
                text: modelData
                focus: false
                readOnly: true
                wrapMode: Text.WordWrap
                hoverEnabled: !currentChat.responseInProgress
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                background: Rectangle {
                    color: hovered ? theme.sourcesBackgroundHovered : theme.sourcesBackground
                    radius: 10
                }
                MouseArea {
                    id: maFollowUp
                    anchors.fill: parent
                    enabled: !currentChat.responseInProgress
                    onClicked: function() {
                        var chat = window.currentChat
                        var followup = modelData
                        chat.stopGenerating()
                        chat.newPromptResponsePair(followup)
                    }
                }
                Item {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 40
                    height: 40
                    visible: !currentChat.responseInProgress
                    Image {
                        id: plusImage
                        anchors.verticalCenter: parent.verticalCenter
                        sourceSize.width: 20
                        sourceSize.height: 20
                        mipmap: true
                        visible: false
                        source: "qrc:/gpt4all/icons/plus.svg"
                    }

                    ColorOverlay {
                        anchors.fill: plusImage
                        source: plusImage
                        color: theme.styledTextColor
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            color: "transparent"
            radius: 10
            Layout.preferredHeight: currentChat.responseInProgress ? 40 : 0
            clip: true
            ColumnLayout {
                id: followUpLayout
                anchors.fill: parent
                Rectangle {
                    id: myRect1
                    Layout.preferredWidth: 0
                    Layout.minimumWidth: 0
                    Layout.maximumWidth: parent.width
                    height: 12
                    color: theme.sourcesBackgroundHovered
                }

                Rectangle {
                    id: myRect2
                    Layout.preferredWidth: 0
                    Layout.minimumWidth: 0
                    Layout.maximumWidth: parent.width
                    height: 12
                    color: theme.sourcesBackgroundHovered
                }

                SequentialAnimation {
                    id: followUpProgressAnimation
                    ParallelAnimation {
                        PropertyAnimation {
                            target: myRect1
                            property: "Layout.preferredWidth"
                            from: 0
                            to: followUpLayout.width
                            duration: 1000
                        }
                        PropertyAnimation {
                            target: myRect2
                            property: "Layout.preferredWidth"
                            from: 0
                            to: followUpLayout.width / 2
                            duration: 1000
                        }
                    }
                    SequentialAnimation {
                        loops: Animation.Infinite
                        ParallelAnimation {
                            PropertyAnimation {
                                target: myRect1
                                property: "opacity"
                                from: 1
                                to: 0.2
                                duration: 1500
                            }
                            PropertyAnimation {
                                target: myRect2
                                property: "opacity"
                                from: 1
                                to: 0.2
                                duration: 1500
                            }
                        }
                        ParallelAnimation {
                            PropertyAnimation {
                                target: myRect1
                                property: "opacity"
                                from: 0.2
                                to: 1
                                duration: 1500
                            }
                            PropertyAnimation {
                                target: myRect2
                                property: "opacity"
                                from: 0.2
                                to: 1
                                duration: 1500
                            }
                        }
                    }
                }

                onVisibleChanged: {
                    if (visible)
                        followUpProgressAnimation.start();
                }
            }

            Behavior on Layout.preferredHeight {
                NumberAnimation {
                    duration: 300
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }

} // GridLayout

} // ColumnLayout
