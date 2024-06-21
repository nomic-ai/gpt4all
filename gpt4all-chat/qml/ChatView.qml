import Qt5Compat.GraphicalEffects
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import chatlistmodel
import download
import gpt4all
import llm
import localdocs
import modellist
import mysettings
import network

Rectangle {
    id: window

    Theme {
        id: theme
    }

    property var currentChat: ChatListModel.currentChat
    property var chatModel: currentChat.chatModel
    signal addCollectionViewRequested()

    color: theme.viewBackground

    Connections {
        target: currentChat
        function onResponseInProgressChanged() {
            if (MySettings.networkIsActive && !currentChat.responseInProgress)
                Network.sendConversation(currentChat.id, getConversationJson());
        }
        function onModelLoadingErrorChanged() {
            if (currentChat.modelLoadingError !== "")
                modelLoadingErrorPopup.open()
        }
        function onModelLoadingWarning(warning) {
            modelLoadingWarningPopup.open_(warning)
        }
    }

    function currentModelName() {
        return ModelList.modelInfo(currentChat.modelInfo.id).name;
    }

    PopupDialog {
        id: modelLoadingErrorPopup
        anchors.centerIn: parent
        shouldTimeOut: false
        text: qsTr("<h3>Encountered an error loading model:</h3><br>")
              + "<i>\"" + currentChat.modelLoadingError + "\"</i>"
              + qsTr("<br><br>Model loading failures can happen for a variety of reasons, but the most common "
                     + "causes include a bad file format, an incomplete or corrupted download, the wrong file "
                     + "type, not enough system RAM or an incompatible model type. Here are some suggestions for resolving the problem:"
                     + "<br><ul>"
                     + "<li>Ensure the model file has a compatible format and type"
                     + "<li>Check the model file is complete in the download folder"
                     + "<li>You can find the download folder in the settings dialog"
                     + "<li>If you've sideloaded the model ensure the file is not corrupt by checking md5sum"
                     + "<li>Read more about what models are supported in our <a href=\"https://docs.gpt4all.io/gpt4all_chat.html\">documentation</a> for the gui"
                     + "<li>Check out our <a href=\"https://discord.gg/4M2QFmTt2k\">discord channel</a> for help")
    }

    PopupDialog {
        id: modelLoadingWarningPopup
        property string message
        anchors.centerIn: parent
        shouldTimeOut: false
        text: qsTr("<h3>Warning</h3><p>%1</p>").arg(message)
        function open_(msg) { message = msg; open(); }
    }

    SwitchModelDialog {
        id: switchModelDialog
        anchors.centerIn: parent
        Item {
            Accessible.role: Accessible.Dialog
            Accessible.name: qsTr("Switch model dialog")
            Accessible.description: qsTr("Warn the user if they switch models, then context will be erased")
        }
    }

    PopupDialog {
        id: copyMessage
        anchors.centerIn: parent
        text: qsTr("Conversation copied to clipboard.")
        font.pixelSize: theme.fontSizeLarge
    }

    PopupDialog {
        id: copyCodeMessage
        anchors.centerIn: parent
        text: qsTr("Code copied to clipboard.")
        font.pixelSize: theme.fontSizeLarge
    }

    function getConversation() {
        var conversation = "";
        for (var i = 0; i < chatModel.count; i++) {
            var item = chatModel.get(i)
            var string = item.name;
            var isResponse = item.name === qsTr("Response: ")
            string += chatModel.get(i).value
            if (isResponse && item.stopped)
                string += " <stopped>"
            string += "\n"
            conversation += string
        }
        return conversation
    }

    function getConversationJson() {
        var str = "{\"conversation\": [";
        for (var i = 0; i < chatModel.count; i++) {
            var item = chatModel.get(i)
            var isResponse = item.name === qsTr("Response: ")
            str += "{\"content\": ";
            str += JSON.stringify(item.value)
            str += ", \"role\": \"" + (isResponse ? "assistant" : "user") + "\"";
            if (isResponse && item.thumbsUpState !== item.thumbsDownState)
                str += ", \"rating\": \"" + (item.thumbsUpState ? "positive" : "negative") + "\"";
            if (isResponse && item.newResponse !== "")
                str += ", \"edited_content\": " + JSON.stringify(item.newResponse);
            if (isResponse && item.stopped)
                str += ", \"stopped\": \"true\""
            if (!isResponse)
                str += "},"
            else
                str += ((i < chatModel.count - 1) ? "}," : "}")
        }
        return str + "]}"
    }

    ChatDrawer {
        id: chatDrawer
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: Math.max(180, Math.min(600, 0.23 * window.width))
    }

    PopupDialog {
        id: referenceContextDialog
        anchors.centerIn: parent
        shouldTimeOut: false
        shouldShowBusy: false
        modal: true
    }

    Item {
        id: mainArea
        anchors.left: chatDrawer.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        state: "expanded"

        states: [
            State {
                name: "expanded"
                AnchorChanges {
                    target: mainArea
                    anchors.left: chatDrawer.right
                }
            },
            State {
                name: "collapsed"
                AnchorChanges {
                    target: mainArea
                    anchors.left: parent.left
                }
            }
        ]

        function toggleLeftPanel() {
            if (mainArea.state === "expanded") {
                mainArea.state = "collapsed";
            } else {
                mainArea.state = "expanded";
            }
        }

        transitions: Transition {
            AnchorAnimation {
                easing.type: Easing.InOutQuad
                duration: 200
            }
        }

        Rectangle {
            id: header
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 100
            color: theme.conversationBackground

            RowLayout {
                id: comboLayout
                height: 80
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 20

                Rectangle {
                    Layout.alignment: Qt.AlignLeft
                    Layout.leftMargin: 30
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    color: "transparent"
                    Layout.preferredHeight: childrenRect.height
                    MyToolButton {
                        id: drawerButton
                        anchors.left: parent.left
                        backgroundColor: theme.iconBackgroundLight
                        width: 40
                        height: 40
                        imageWidth: 40
                        imageHeight: 40
                        padding: 15
                        source: mainArea.state === "expanded" ? "qrc:/gpt4all/icons/left_panel_open.svg" : "qrc:/gpt4all/icons/left_panel_closed.svg"
                        Accessible.role: Accessible.ButtonMenu
                        Accessible.name: qsTr("Chat panel")
                        Accessible.description: qsTr("Chat panel with options")
                        onClicked: {
                            mainArea.toggleLeftPanel()
                        }
                    }
                }

                MyComboBox {
                    id: comboBox
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillHeight: true
                    Layout.preferredWidth: 350
                    Layout.maximumWidth: 675
                    enabled: !currentChat.isServer
                        && !currentChat.trySwitchContextInProgress
                        && !currentChat.isCurrentlyLoading
                    model: ModelList.installedModels
                    valueRole: "id"
                    textRole: "name"

                    function changeModel(index) {
                        currentChat.stopGenerating()
                        currentChat.reset();
                        currentChat.modelInfo = ModelList.modelInfo(comboBox.valueAt(index))
                    }

                    Connections {
                        target: switchModelDialog
                        function onAccepted() {
                            comboBox.changeModel(switchModelDialog.index)
                        }
                    }

                    background: Rectangle {
                        color: theme.mainComboBackground
                        radius: 10
                        ProgressBar {
                            id: modelProgress
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.right: parent.right
                            visible: currentChat.isCurrentlyLoading
                            height: 10
                            value: currentChat.modelLoadingPercentage
                            background: Rectangle {
                                color: theme.green100
                                radius: 10
                            }
                            contentItem: Item {
                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    width: modelProgress.visualPosition * parent.width
                                    height: 10
                                    radius: 2
                                    color: theme.green600
                                }
                            }
                        }
                    }
                    contentItem: Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        leftPadding: 10
                        rightPadding: {
                            if (ejectButton.visible && reloadButton)
                                return 105;
                            if (reloadButton.visible)
                                return 65
                            return 25
                        }
                        text: {
                            if (currentChat.modelLoadingError !== "")
                                return qsTr("Model loading error...")
                            if (currentChat.trySwitchContextInProgress === 1)
                                return qsTr("Waiting for model...")
                            if (currentChat.trySwitchContextInProgress === 2)
                                return qsTr("Switching context...")
                            if (currentModelName() === "")
                                return qsTr("Choose a model...")
                            if (currentChat.modelLoadingPercentage === 0.0)
                                return qsTr("Reload \u00B7 ") + currentModelName()
                            if (currentChat.isCurrentlyLoading)
                                return qsTr("Loading \u00B7 ") + currentModelName()
                            return currentModelName()
                        }
                        font.pixelSize: theme.fontSizeLarger
                        color: theme.iconBackgroundLight
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                    }
                    delegate: ItemDelegate {
                        id: comboItemDelegate
                        width: comboBox.width
                        contentItem: Text {
                            text: name
                            color: theme.textColor
                            font: comboBox.font
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: (index % 2 === 0 ? theme.darkContrast : theme.lightContrast)
                            border.width: highlighted
                            border.color: theme.accentColor
                        }
                        highlighted: comboBox.highlightedIndex === index
                    }
                    Accessible.role: Accessible.ComboBox
                    Accessible.name: currentModelName()
                    Accessible.description: qsTr("The top item is the current model")
                    onActivated: function (index) {
                        var newInfo = ModelList.modelInfo(comboBox.valueAt(index));
                        if (newInfo === currentChat.modelInfo) {
                            currentChat.reloadModel();
                        } else if (currentModelName() !== "" && chatModel.count !== 0) {
                            switchModelDialog.index = index;
                            switchModelDialog.open();
                        } else {
                            comboBox.changeModel(index);
                        }
                    }

                    MyMiniButton {
                        id: ejectButton
                        visible: currentChat.isModelLoaded && !currentChat.isCurrentlyLoading
                        z: 500
                        anchors.right: parent.right
                        anchors.rightMargin: 50
                        anchors.verticalCenter: parent.verticalCenter
                        source: "qrc:/gpt4all/icons/eject.svg"
                        backgroundColor: theme.gray300
                        backgroundColorHovered: theme.iconBackgroundLight
                        onClicked: {
                            currentChat.forceUnloadModel();
                        }
                        ToolTip.text: qsTr("Eject the currently loaded model")
                        ToolTip.visible: hovered
                    }

                    MyMiniButton {
                        id: reloadButton
                        visible: currentChat.modelLoadingError === ""
                            && !currentChat.trySwitchContextInProgress
                            && !currentChat.isCurrentlyLoading
                            && (currentChat.isModelLoaded || currentModelName() !== "")
                        z: 500
                        anchors.right: ejectButton.visible ? ejectButton.left : parent.right
                        anchors.rightMargin: ejectButton.visible ? 10 : 50
                        anchors.verticalCenter: parent.verticalCenter
                        source: "qrc:/gpt4all/icons/regenerate.svg"
                        backgroundColor: theme.gray300
                        backgroundColorHovered: theme.iconBackgroundLight
                        onClicked: {
                            if (currentChat.isModelLoaded)
                                currentChat.forceReloadModel();
                            else
                                currentChat.reloadModel();
                        }
                        ToolTip.text: qsTr("Reload the currently loaded model")
                        ToolTip.visible: hovered
                    }
                }

                Rectangle {
                    color: "transparent"
                    Layout.alignment: Qt.AlignRight
                    Layout.rightMargin: 30
                    Layout.fillWidth: true
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: childrenRect.height

                    RowLayout {
                        spacing: 20
                        anchors.right: parent.right
                        MyButton {
                            id: collectionsButton
                            borderWidth: 0
                            backgroundColor: theme.collectionButtonBackground
                            backgroundColorHovered: theme.collectionButtonBackgroundHovered
                            backgroundRadius: 5
                            padding: 15
                            topPadding: 8
                            bottomPadding: 8

                            contentItem: RowLayout {
                                spacing: 10
                                Item {
                                    visible: currentChat.collectionModel.count === 0
                                    Layout.minimumWidth: collectionsImage.width
                                    Layout.minimumHeight: collectionsImage.height
                                    Image {
                                        id: collectionsImage
                                        anchors.verticalCenter: parent.verticalCenter
                                        sourceSize.width: 24
                                        sourceSize.height: 24
                                        mipmap: true
                                        visible: false
                                        source: "qrc:/gpt4all/icons/db.svg"
                                    }

                                    ColorOverlay {
                                        anchors.fill: collectionsImage
                                        source: collectionsImage
                                        color: theme.green600
                                    }
                                }

                                MyBusyIndicator {
                                    visible: currentChat.collectionModel.updatingCount !== 0
                                    color: theme.green400
                                    size: 24
                                    Layout.minimumWidth: 24
                                    Layout.minimumHeight: 24
                                    Text {
                                        anchors.centerIn: parent
                                        text: currentChat.collectionModel.updatingCount
                                        color: theme.green600
                                        font.pixelSize: 14 // fixed regardless of theme
                                    }
                                }

                                Rectangle {
                                    visible: currentChat.collectionModel.count !== 0
                                    radius: 6
                                    color: theme.green600
                                    Layout.minimumWidth: collectionsImage.width
                                    Layout.minimumHeight: collectionsImage.height
                                    Text {
                                        anchors.centerIn: parent
                                        text: currentChat.collectionModel.count
                                        color: theme.white
                                        font.pixelSize: 14 // fixed regardless of theme
                                    }
                                }

                                Text {
                                    text: qsTr("LocalDocs")
                                    color: theme.green600
                                    font.pixelSize: theme.fontSizeLarge
                                }
                            }

                            fontPixelSize: theme.fontSizeLarge

                            background: Rectangle {
                                radius: collectionsButton.backgroundRadius
                                color: collectionsButton.toggled ? collectionsButton.backgroundColorHovered : collectionsButton.backgroundColor
                            }

                            Accessible.name: qsTr("Add documents")
                            Accessible.description: qsTr("add collections of documents to the chat")

                            onClicked: {
                                conversation.toggleRightPanel()
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            id: conversationDivider
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            color: theme.conversationDivider
            height: 2
        }

        CollectionsDrawer {
            id: collectionsDrawer
            anchors.right: parent.right
            anchors.top: conversationDivider.bottom
            anchors.bottom: parent.bottom
            width: Math.max(180, Math.min(600, 0.23 * window.width))
            color: theme.conversationBackground
            onAddDocsClicked: {
                addCollectionViewRequested()
            }
        }

        Rectangle {
            id: conversation
            color: theme.conversationBackground
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.top: conversationDivider.bottom
            state: "collapsed"

            states: [
                State {
                    name: "expanded"
                    AnchorChanges {
                        target: conversation
                        anchors.right: collectionsDrawer.left
                    }
                },
                State {
                    name: "collapsed"
                    AnchorChanges {
                        target: conversation
                        anchors.right: parent.right
                    }
                }
            ]

            function toggleRightPanel() {
                if (conversation.state === "expanded") {
                    conversation.state = "collapsed";
                } else {
                    conversation.state = "expanded";
                }
            }

            transitions: Transition {
                AnchorAnimation {
                    easing.type: Easing.InOutQuad
                    duration: 300
                }
            }

            ScrollView {
                id: scrollView
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: !currentChat.isServer ? textInputView.top : parent.bottom
                anchors.bottomMargin: !currentChat.isServer ? 30 : 0
                ScrollBar.vertical.policy: ScrollBar.AlwaysOff

                Rectangle {
                    anchors.fill: parent
                    color: currentChat.isServer ? theme.black : theme.conversationBackground

                    Rectangle {
                        id: homePage
                        color: "transparent"
                        anchors.fill: parent
                        visible: !currentChat.isModelLoaded && (ModelList.installedModels.count === 0 || currentModelName() === "") && !currentChat.isServer

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 0

                            Rectangle {
                                Layout.alignment: Qt.AlignCenter
                                Layout.preferredWidth: image.width
                                Layout.preferredHeight: image.height
                                color: "transparent"

                                Image {
                                    id: image
                                    anchors.centerIn: parent
                                    sourceSize.width: 160
                                    sourceSize.height: 110
                                    fillMode: Image.PreserveAspectFit
                                    mipmap: true
                                    visible: false
                                    source: "qrc:/gpt4all/icons/nomic_logo.svg"
                                }

                                ColorOverlay {
                                    anchors.fill: image
                                    source: image
                                    color: theme.containerBackground
                                }
                            }

                            // FIXME_BLOCKER Need answer for displaying if datalake is on/off
                        }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        visible: ModelList.installedModels.count !== 0 && chatModel.count !== 0
                        ListView {
                            id: listView
                            Layout.maximumWidth: 1280
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            Layout.margins: 20
                            Layout.leftMargin: 50
                            Layout.rightMargin: 50
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 50
                            model: chatModel

                            ScrollBar.vertical: ScrollBar {
                                policy: ScrollBar.AsNeeded
                            }

                            Accessible.role: Accessible.List
                            Accessible.name: qsTr("Conversation with the model")
                            Accessible.description: qsTr("prompt / response pairs from the conversation")

                            delegate: RowLayout {
                                width: listView.contentItem.width
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    RowLayout {
                                        spacing: 10
                                        Layout.fillWidth: true
                                        TextArea {
                                            // FIXME_BLOCKER Need icons for this
                                            text: name === qsTr("Response: ") ? qsTr("GPT4All") : qsTr("You")
                                            padding: 0
                                            font.pixelSize: theme.fontSizeLargest
                                            font.bold: true
                                            color: theme.titleTextColor
                                            readOnly: true
                                        }
                                        Text {
                                            visible: name === qsTr("Response: ")
                                            font.pixelSize: theme.fontSizeLarger
                                            text: currentModelName()
                                            color: theme.gray500
                                        }
                                        RowLayout {
                                            visible: (currentResponse ? true : false) && ((value === "" && currentChat.responseInProgress) || currentChat.isRecalc)
                                            MyBusyIndicator {
                                                size: 24
                                                color: theme.green400
                                                Accessible.role: Accessible.Animation
                                                Accessible.name: qsTr("Busy indicator")
                                                Accessible.description: qsTr("The model is thinking")
                                            }
                                            Text {
                                                color: theme.gray500
                                                font.pixelSize: theme.fontSizeLarger
                                                text: {
                                                    if (currentChat.isRecalc)
                                                        return qsTr("recalculating context ...");
                                                    switch (currentChat.responseState) {
                                                    case Chat.ResponseStopped: return qsTr("response stopped ...");
                                                    case Chat.LocalDocsRetrieval: return qsTr("retrieving localdocs: ") + currentChat.collectionList.join(", ") + " ...";
                                                    case Chat.LocalDocsProcessing: return qsTr("searching localdocs: ") + currentChat.collectionList.join(", ") + " ...";
                                                    case Chat.PromptProcessing: return qsTr("processing ...")
                                                    case Chat.ResponseGeneration: return qsTr("generating response ...");
                                                    default: return ""; // handle unexpected values
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    ColumnLayout {
                                        visible: sources.length !== 0 && MySettings.localDocsShowReferences
                                        RowLayout {
                                            Layout.topMargin: 15
                                            Image {
                                                sourceSize.width: 24
                                                sourceSize.height: 24
                                                mipmap: true
                                                source: "qrc:/gpt4all/icons/db.svg"
                                            }

                                            Text {
                                                text: qsTr("\u00B7 Sources")
                                                font.pixelSize: theme.fontSizeLarger
                                            }
                                        }
                                        Flow {
                                            Layout.fillWidth: true
                                            Layout.topMargin: 5
                                            spacing: 10
                                            visible: sources.length !== 0
                                            Repeater {
                                                model: sources

                                                delegate: Rectangle {
                                                    radius: 10
                                                    color: ma.containsMouse ? theme.gray200 : theme.gray100
                                                    width: 200
                                                    height: 75

                                                    MouseArea {
                                                        id: ma
                                                        enabled: modelData.path !== ""
                                                        anchors.fill: parent
                                                        hoverEnabled: true
                                                        onClicked: function() {
                                                            Qt.openUrlExternally("file://" + modelData.path)
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

                                                    RowLayout {
                                                        anchors.fill: parent
                                                        ColumnLayout {
                                                            spacing: 10
                                                            Layout.alignment: Qt.AlignTop
                                                            Layout.margins: 10
                                                            RowLayout {
                                                                Image {
                                                                    sourceSize.width: 24
                                                                    sourceSize.height: 24
                                                                    mipmap: true
                                                                    source: {
                                                                        if (modelData.file.endsWith(".txt"))
                                                                            return "qrc:/gpt4all/icons/file-txt.svg"
                                                                        else if (modelData.file.endsWith(".pdf"))
                                                                            return "qrc:/gpt4all/icons/file-pdf.svg"
                                                                        else if (modelData.file.endsWith(".md"))
                                                                            return "qrc:/gpt4all/icons/file-md.svg"
                                                                        else
                                                                            return "qrc:/gpt4all/icons/file.svg"
                                                                    }
                                                                }
                                                                Text {
                                                                    Layout.maximumWidth: 180
                                                                    text: modelData.collection !== "" ? modelData.collection : qsTr("LocalDocs")
                                                                    font.pixelSize: theme.fontSizeLarge
                                                                    font.bold: true
                                                                    color: theme.grayRed900
                                                                    elide: Qt.ElideRight
                                                                }
                                                            }
                                                            Text {
                                                                Layout.fillHeight: true
                                                                Layout.maximumWidth: 180
                                                                height: 75
                                                                text: modelData.file
                                                                font.pixelSize: theme.fontSizeSmall
                                                                elide: Qt.ElideRight
                                                                wrapMode: Text.WrapAnywhere
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        RowLayout {
                                            Layout.topMargin: 15
                                            Image {
                                                sourceSize.width: 24
                                                sourceSize.height: 24
                                                mipmap: true
                                                source: "qrc:/gpt4all/icons/info.svg"
                                            }
                                            Text {
                                                text: qsTr("\u00B7 Answer")
                                                font.pixelSize: theme.fontSizeLarger
                                            }
                                        }
                                    }

                                    TextArea {
                                        id: myTextArea
                                        text: value
                                        Layout.fillWidth: true
                                        padding: 0
                                        color: {
                                            if (!currentChat.isServer)
                                                return theme.textColor
                                            if (name === qsTr("Response: "))
                                                return theme.white
                                            return theme.black
                                        }
                                        wrapMode: Text.WordWrap
                                        textFormat: TextEdit.PlainText
                                        focus: false
                                        readOnly: true
                                        font.pixelSize: theme.fontSizeLarge
                                        cursorVisible: currentResponse ? currentChat.responseInProgress : false
                                        cursorPosition: text.length
                                        TapHandler {
                                            id: tapHandler
                                            onTapped: function(eventPoint, button) {
                                                var clickedPos = myTextArea.positionAt(eventPoint.position.x, eventPoint.position.y);
                                                var link = responseText.getLinkAtPosition(clickedPos);
                                                if (link.startsWith("context://")) {
                                                    var integer = parseInt(link.split("://")[1]);
                                                    referenceContextDialog.text = referencesContext[integer - 1];
                                                    referenceContextDialog.open();
                                                } else {
                                                    var success = responseText.tryCopyAtPosition(clickedPos);
                                                    if (success)
                                                        copyCodeMessage.open();
                                                }
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

                                        Menu {
                                            id: conversationContextMenu
                                            MenuItem {
                                                text: qsTr("Copy")
                                                enabled: myTextArea.selectedText !== ""
                                                height: enabled ? implicitHeight : 0
                                                onTriggered: myTextArea.copy()
                                            }
                                            MenuItem {
                                                text: qsTr("Copy Message")
                                                enabled: myTextArea.selectedText === ""
                                                height: enabled ? implicitHeight : 0
                                                onTriggered: {
                                                    myTextArea.selectAll()
                                                    myTextArea.copy()
                                                    myTextArea.deselect()
                                                }
                                            }
                                        }

                                        ResponseText {
                                            id: responseText
                                        }

                                        Component.onCompleted: {
                                            responseText.setLinkColor(theme.linkColor);
                                            responseText.setHeaderColor(name === qsTr("Response: ") ? theme.darkContrast : theme.lightContrast);
                                            responseText.textDocument = textDocument
                                        }

                                        Accessible.role: Accessible.Paragraph
                                        Accessible.name: text
                                        Accessible.description: name === qsTr("Response: ") ? "The response by the model" : "The prompt by the user"
                                    }

                                    ThumbsDownDialog {
                                        id: thumbsDownDialog
                                        property point globalPoint: mapFromItem(window,
                                                                                window.width / 2 - width / 2,
                                                                                window.height / 2 - height / 2)
                                        x: globalPoint.x
                                        y: globalPoint.y
                                        property string text: value
                                        response: newResponse === undefined || newResponse === "" ? text : newResponse
                                        onAccepted: {
                                            var responseHasChanged = response !== text && response !== newResponse
                                            if (thumbsDownState && !thumbsUpState && !responseHasChanged)
                                                return

                                            chatModel.updateNewResponse(index, response)
                                            chatModel.updateThumbsUpState(index, false)
                                            chatModel.updateThumbsDownState(index, true)
                                            Network.sendConversation(currentChat.id, getConversationJson());
                                        }
                                    }

                                    Column {
                                        Layout.alignment: Qt.AlignRight
                                        Layout.rightMargin: 15
                                        visible: name === qsTr("Response: ") &&
                                                 (!currentResponse || !currentChat.responseInProgress) && MySettings.networkIsActive
                                        spacing: 10

                                        Item {
                                            width: childrenRect.width
                                            height: childrenRect.height
                                            MyToolButton {
                                                id: thumbsUp
                                                width: 24
                                                height: 24
                                                imageWidth: width
                                                imageHeight: height
                                                opacity: thumbsUpState || thumbsUpState == thumbsDownState ? 1.0 : 0.2
                                                source: "qrc:/gpt4all/icons/thumbs_up.svg"
                                                Accessible.name: qsTr("Thumbs up")
                                                Accessible.description: qsTr("Gives a thumbs up to the response")
                                                onClicked: {
                                                    if (thumbsUpState && !thumbsDownState)
                                                        return

                                                    chatModel.updateNewResponse(index, "")
                                                    chatModel.updateThumbsUpState(index, true)
                                                    chatModel.updateThumbsDownState(index, false)
                                                    Network.sendConversation(currentChat.id, getConversationJson());
                                                }
                                            }

                                            MyToolButton {
                                                id: thumbsDown
                                                anchors.top: thumbsUp.top
                                                anchors.topMargin: 10
                                                anchors.left: thumbsUp.right
                                                anchors.leftMargin: 2
                                                width: 24
                                                height: 24
                                                imageWidth: width
                                                imageHeight: height
                                                checked: thumbsDownState
                                                opacity: thumbsDownState || thumbsUpState == thumbsDownState ? 1.0 : 0.2
                                                transform: [
                                                    Matrix4x4 {
                                                        matrix: Qt.matrix4x4(-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
                                                    },
                                                    Translate {
                                                        x: thumbsDown.width
                                                    }
                                                ]
                                                source: "qrc:/gpt4all/icons/thumbs_down.svg"
                                                Accessible.name: qsTr("Thumbs down")
                                                Accessible.description: qsTr("Opens thumbs down dialog")
                                                onClicked: {
                                                    thumbsDownDialog.open()
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            property bool shouldAutoScroll: true
                            property bool isAutoScrolling: false

                            Connections {
                                target: currentChat
                                function onResponseChanged() {
                                    if (listView.shouldAutoScroll) {
                                        listView.isAutoScrolling = true
                                        listView.positionViewAtEnd()
                                        listView.isAutoScrolling = false
                                    }
                                }
                            }

                            onContentYChanged: {
                                if (!isAutoScrolling)
                                    shouldAutoScroll = atYEnd
                            }

                            Component.onCompleted: {
                                shouldAutoScroll = true
                                positionViewAtEnd()
                            }

                            footer: Item {
                                id: bottomPadding
                                width: parent.width
                                height: 60
                            }
                        }
                    }
                }
            }

            RowLayout {
                id: conversationButtons
                anchors.bottom: textInputView.top
                anchors.horizontalCenter: textInputView.horizontalCenter
                anchors.bottomMargin: 20
                spacing: 10
                MyButton {
                    textColor: theme.textColor
                    visible: chatModel.count && !currentChat.isServer && currentChat.isModelLoaded
                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 15
                        source: currentChat.responseInProgress ? "qrc:/gpt4all/icons/stop_generating.svg" : "qrc:/gpt4all/icons/regenerate.svg"
                    }

                    leftPadding: 50
                    onClicked: {
                        var index = Math.max(0, chatModel.count - 1);
                        var listElement = chatModel.get(index);

                        if (currentChat.responseInProgress) {
                            listElement.stopped = true
                            currentChat.stopGenerating()
                        } else {
                            currentChat.regenerateResponse()
                            if (chatModel.count) {
                                if (listElement.name === qsTr("Response: ")) {
                                    chatModel.updateCurrentResponse(index, true);
                                    chatModel.updateStopped(index, false);
                                    chatModel.updateThumbsUpState(index, false);
                                    chatModel.updateThumbsDownState(index, false);
                                    chatModel.updateNewResponse(index, "");
                                    currentChat.prompt(listElement.prompt)
                                }
                            }
                        }
                    }

                    borderWidth: 1
                    backgroundColor: theme.conversationButtonBackground
                    backgroundColorHovered: theme.conversationButtonBackgroundHovered
                    backgroundRadius: 5
                    padding: 15
                    topPadding: 8
                    bottomPadding: 8
                    text: currentChat.responseInProgress ? qsTr("Stop generating") : qsTr("Regenerate response")
                    fontPixelSize: theme.fontSizeSmall
                    Accessible.description: qsTr("Controls generation of the response")
                }

                MyButton {
                    textColor: theme.textColor
                    visible: !currentChat.isServer
                        && !currentChat.isModelLoaded
                        && currentChat.modelLoadingError === ""
                        && !currentChat.trySwitchContextInProgress
                        && !currentChat.isCurrentlyLoading
                        && currentModelName() !== ""

                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 15
                        source: "qrc:/gpt4all/icons/regenerate.svg"
                    }
                    leftPadding: 50
                    onClicked: {
                        currentChat.reloadModel();
                    }

                    borderWidth: 1
                    backgroundColor: theme.conversationButtonBackground
                    backgroundColorHovered: theme.conversationButtonBackgroundHovered
                    backgroundRadius: 5
                    padding: 15
                    topPadding: 8
                    bottomPadding: 8
                    text: qsTr("Reload \u00B7 ") + currentChat.modelInfo.name
                    fontPixelSize: theme.fontSizeSmall
                    Accessible.description: qsTr("Reloads the model")
                }
            }

            Text {
                id: device
                anchors.top: textInputView.bottom
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.rightMargin: 30
                anchors.left: parent.left
                anchors.leftMargin: 30
                horizontalAlignment: Qt.AlignRight
                verticalAlignment: Qt.AlignVCenter
                color: theme.mutedTextColor
                visible: currentChat.tokenSpeed !== ""
                elide: Text.ElideRight
                wrapMode: Text.WordWrap
                text: currentChat.tokenSpeed + " \u00B7 " + currentChat.device + currentChat.fallbackReason
                font.pixelSize: theme.fontSizeSmaller
                font.bold: true
            }

            RectangularGlow {
                id: effect
                visible: !currentChat.isServer
                anchors.fill: textInputView
                glowRadius: 50
                spread: 0
                color: theme.sendGlow
                cornerRadius: 10
                opacity: 0.1
            }

            ScrollView {
                id: textInputView
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 30
                anchors.leftMargin: Math.max((parent.width - 1310) / 2, 30)
                anchors.rightMargin: Math.max((parent.width - 1310) / 2, 30)
                height: Math.min(contentHeight, 200)
                visible: !currentChat.isServer
                MyTextArea {
                    id: textInput
                    color: theme.textColor
                    topPadding: 30
                    bottomPadding: 30
                    leftPadding: 20
                    rightPadding: 40
                    enabled: currentChat.isModelLoaded && !currentChat.isServer
                    font.pixelSize: theme.fontSizeLarger
                    placeholderText: currentChat.isModelLoaded ? qsTr("Send a message...") : qsTr("Load a model to continue...")
                    Accessible.role: Accessible.EditableText
                    Accessible.name: placeholderText
                    Accessible.description: qsTr("Send messages/prompts to the model")
                    Keys.onReturnPressed: (event)=> {
                                              if (event.modifiers & Qt.ControlModifier || event.modifiers & Qt.ShiftModifier)
                                              event.accepted = false;
                                              else {
                                                  editingFinished();
                                                  sendMessage()
                                              }
                                          }
                    function sendMessage() {
                        if (textInput.text === "")
                            return

                        currentChat.stopGenerating()
                        currentChat.newPromptResponsePair(textInput.text);
                        currentChat.prompt(textInput.text,
                                           MySettings.promptTemplate,
                                           MySettings.maxLength,
                                           MySettings.topK,
                                           MySettings.topP,
                                           MySettings.minP,
                                           MySettings.temperature,
                                           MySettings.promptBatchSize,
                                           MySettings.repeatPenalty,
                                           MySettings.repeatPenaltyTokens)
                        textInput.text = ""
                    }

                    MouseArea {
                        id: textInputMouseArea
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton

                        onClicked: (mouse) => {
                            if (mouse.button === Qt.RightButton) {
                                textInputContextMenu.x = textInputMouseArea.mouseX
                                textInputContextMenu.y = textInputMouseArea.mouseY
                                textInputContextMenu.open()
                            }
                        }
                    }

                    Menu {
                        id: textInputContextMenu
                        MenuItem {
                            text: qsTr("Cut")
                            enabled: textInput.selectedText !== ""
                            height: enabled ? implicitHeight : 0
                            onTriggered: textInput.cut()
                        }
                        MenuItem {
                            text: qsTr("Copy")
                            enabled: textInput.selectedText !== ""
                            height: enabled ? implicitHeight : 0
                            onTriggered: textInput.copy()
                        }
                        MenuItem {
                            text: qsTr("Paste")
                            onTriggered: textInput.paste()
                        }
                        MenuItem {
                            text: qsTr("Select All")
                            onTriggered: textInput.selectAll()
                        }
                    }
                }
            }

            Image {
                id: antennaImage
                anchors.right: sendButton.left
                anchors.verticalCenter: textInputView.verticalCenter
                anchors.rightMargin: 15
                sourceSize.width: 32
                sourceSize.height: 32
                visible: false
                fillMode: Image.PreserveAspectFit
                source: "qrc:/gpt4all/icons/antenna_3.svg"
            }

            ColorOverlay {
                visible: currentChat.isServer || currentChat.modelInfo.isOnline || MySettings.networkIsActive
                anchors.fill: antennaImage
                source: antennaImage
                color: theme.grayRed900
            }

            SequentialAnimation {
                running: true
                loops: Animation.Infinite

                PropertyAnimation {
                    target: antennaImage
                    property: "source"
                    duration: 500
                    from: "qrc:/gpt4all/icons/antenna_1.svg"
                    to: "qrc:/gpt4all/icons/antenna_2.svg"
                }

                PauseAnimation {
                    duration: 1500
                }

                PropertyAnimation {
                    target: antennaImage
                    property: "source"
                    duration: 500
                    from: "qrc:/gpt4all/icons/antenna_2.svg"
                    to: "qrc:/gpt4all/icons/antenna_3.svg"
                }

                PauseAnimation {
                    duration: 1500
                }

                PropertyAnimation {
                    target: antennaImage
                    property: "source"
                    duration: 500
                    from: "qrc:/gpt4all/icons/antenna_3.svg"
                    to: "qrc:/gpt4all/icons/antenna_2.svg"
                }

                PauseAnimation {
                    duration: 1500
                }

                PropertyAnimation {
                    target: antennaImage
                    property: "source"
                    duration: 1500
                    from: "qrc:/gpt4all/icons/antenna_2.svg"
                    to: "qrc:/gpt4all/icons/antenna_1.svg"
                }

                PauseAnimation {
                    duration: 500
                }
            }

            MyToolButton {
                id: sendButton
                backgroundColor: theme.sendButtonBackground
                backgroundColorHovered: theme.sendButtonBackgroundHovered
                anchors.right: textInputView.right
                anchors.verticalCenter: textInputView.verticalCenter
                anchors.rightMargin: 15
                width: 30
                height: 30
                visible: !currentChat.isServer
                enabled: !currentChat.responseInProgress
                source: "qrc:/gpt4all/icons/send_message.svg"
                Accessible.name: qsTr("Send message")
                Accessible.description: qsTr("Sends the message/prompt contained in textfield to the model")

                onClicked: {
                    textInput.sendMessage()
                }
            }
        }
    }
}
