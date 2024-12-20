import Qt5Compat.GraphicalEffects
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs
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
    property var currentModelInfo: currentChat && currentChat.modelInfo
    property var currentModelId: null
    onCurrentModelInfoChanged: {
        const newId = currentModelInfo && currentModelInfo.id;
        if (currentModelId !== newId) { currentModelId = newId; }
    }
    signal addCollectionViewRequested()
    signal addModelViewRequested()

    color: theme.viewBackground

    Connections {
        target: currentChat
        // FIXME: https://github.com/nomic-ai/gpt4all/issues/3334
        // function onResponseInProgressChanged() {
        //    if (MySettings.networkIsActive && !currentChat.responseInProgress)
        //        Network.sendConversation(currentChat.id, getConversationJson());
        // }
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

    function currentModelInstalled() {
        return currentModelName() !== "" && ModelList.modelInfo(currentChat.modelInfo.id).installed;
    }

    PopupDialog {
        id: modelLoadingErrorPopup
        anchors.centerIn: parent
        shouldTimeOut: false
        text: qsTr("<h3>Encountered an error loading model:</h3><br>"
              + "<i>\"%1\"</i>"
              + "<br><br>Model loading failures can happen for a variety of reasons, but the most common "
              + "causes include a bad file format, an incomplete or corrupted download, the wrong file "
              + "type, not enough system RAM or an incompatible model type. Here are some suggestions for resolving the problem:"
              + "<br><ul>"
              + "<li>Ensure the model file has a compatible format and type"
              + "<li>Check the model file is complete in the download folder"
              + "<li>You can find the download folder in the settings dialog"
              + "<li>If you've sideloaded the model ensure the file is not corrupt by checking md5sum"
              + "<li>Read more about what models are supported in our <a href=\"https://docs.gpt4all.io/\">documentation</a> for the gui"
              + "<li>Check out our <a href=\"https://discord.gg/4M2QFmTt2k\">discord channel</a> for help").arg(currentChat.modelLoadingError);
    }

    PopupDialog {
        id: modelLoadingWarningPopup
        property string message
        anchors.centerIn: parent
        shouldTimeOut: false
        text: qsTr("<h3>Warning</h3><p>%1</p>").arg(message)
        function open_(msg) { message = msg; open(); }
    }

    ConfirmationDialog {
        id: switchModelDialog
        property int index: -1
        dialogTitle: qsTr("Erase conversation?")
        description: qsTr("Changing the model will erase the current conversation.")
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

    ConfirmationDialog {
        id: resetContextDialog
        dialogTitle: qsTr("Erase conversation?")
        description: qsTr("The entire chat will be erased.")
        onAccepted: {
            Network.trackChatEvent("reset_context", { "length": chatModel.count });
            currentChat.reset();
        }
    }

    // FIXME: https://github.com/nomic-ai/gpt4all/issues/3334
    // function getConversation() {
    //     var conversation = "";
    //     for (var i = 0; i < chatModel.count; i++) {
    //         var item = chatModel.get(i)
    //         var string = item.name;
    //         var isResponse = item.name === "Response: "
    //         string += chatModel.get(i).value
    //         if (isResponse && item.stopped)
    //             string += " <stopped>"
    //         string += "\n"
    //         conversation += string
    //     }
    //     return conversation
    // }

    // FIXME: https://github.com/nomic-ai/gpt4all/issues/3334
    // function getConversationJson() {
    //     var str = "{\"conversation\": [";
    //     for (var i = 0; i < chatModel.count; i++) {
    //         var item = chatModel.get(i)
    //         var isResponse = item.name === "Response: "
    //         str += "{\"content\": ";
    //         str += JSON.stringify(item.value)
    //         str += ", \"role\": \"" + (isResponse ? "assistant" : "user") + "\"";
    //         if (isResponse && item.thumbsUpState !== item.thumbsDownState)
    //             str += ", \"rating\": \"" + (item.thumbsUpState ? "positive" : "negative") + "\"";
    //         if (isResponse && item.newResponse !== "")
    //             str += ", \"edited_content\": " + JSON.stringify(item.newResponse);
    //         if (isResponse && item.stopped)
    //             str += ", \"stopped\": \"true\""
    //         if (!isResponse)
    //             str += "},"
    //         else
    //             str += ((i < chatModel.count - 1) ? "}," : "}")
    //     }
    //     return str + "]}"
    // }

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
                spacing: 0

                Rectangle {
                    Layout.alignment: Qt.AlignLeft
                    Layout.leftMargin: 30
                    Layout.fillWidth: true
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

                ComboBox {
                    id: comboBox
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillHeight: true
                    Layout.preferredWidth: 550
                    Layout.leftMargin: {
                        // This function works in tandem with the preferredWidth and the layout to
                        // provide the maximum size combobox we can have at the smallest window width
                        // we allow with the largest font size we allow. It is unfortunately based
                        // upon a magic number that was produced through trial and error for something
                        // I don't fully understand.
                        return -Math.max(0, comboBox.width / 2 + collectionsButton.width + 110 /*magic*/ - comboLayout.width / 2);
                    }
                    enabled: !currentChat.isServer
                        && !currentChat.trySwitchContextInProgress
                        && !currentChat.isCurrentlyLoading
                        && ModelList.selectableModels.count !== 0
                    model: ModelList.selectableModels
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
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: contentRow.width + 20
                            visible: currentChat.isCurrentlyLoading
                            height: 10
                            value: currentChat.modelLoadingPercentage
                            background: Rectangle {
                                color: theme.progressBackground
                                radius: 10
                            }
                            contentItem: Item {
                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    width: modelProgress.visualPosition * parent.width
                                    height: 10
                                    radius: 2
                                    color: theme.progressForeground
                                }
                            }
                        }
                    }

                    contentItem: Item {
                        RowLayout {
                            id: contentRow
                            anchors.centerIn: parent
                            spacing: 0
                            Layout.maximumWidth: 550
                            RowLayout {
                                id: miniButtonsRow
                                clip: true
                                Layout.maximumWidth: 550
                                Behavior on Layout.preferredWidth {
                                    NumberAnimation {
                                        duration: 300
                                        easing.type: Easing.InOutQuad
                                    }
                                }

                                Layout.preferredWidth: {
                                    if (!(comboBox.hovered || reloadButton.hovered || ejectButton.hovered))
                                        return 0
                                    return (reloadButton.visible ? reloadButton.width : 0) + (ejectButton.visible ? ejectButton.width : 0)
                                }

                                MyMiniButton {
                                    id: reloadButton
                                    Layout.alignment: Qt.AlignCenter
                                    visible: currentChat.modelLoadingError === ""
                                        && !currentChat.trySwitchContextInProgress
                                        && !currentChat.isCurrentlyLoading
                                        && (currentChat.isModelLoaded || currentModelInstalled())
                                    source: "qrc:/gpt4all/icons/regenerate.svg"
                                    backgroundColor: theme.textColor
                                    backgroundColorHovered: theme.styledTextColor
                                    onClicked: {
                                        if (currentChat.isModelLoaded)
                                            currentChat.forceReloadModel();
                                        else
                                            currentChat.reloadModel();
                                    }
                                    ToolTip.text: qsTr("Reload the currently loaded model")
                                    ToolTip.visible: hovered
                                }

                                MyMiniButton {
                                    id: ejectButton
                                    Layout.alignment: Qt.AlignCenter
                                    visible: currentChat.isModelLoaded && !currentChat.isCurrentlyLoading
                                    source: "qrc:/gpt4all/icons/eject.svg"
                                    backgroundColor: theme.textColor
                                    backgroundColorHovered: theme.styledTextColor
                                    onClicked: {
                                        currentChat.forceUnloadModel();
                                    }
                                    ToolTip.text: qsTr("Eject the currently loaded model")
                                    ToolTip.visible: hovered
                                }
                            }

                            Text {
                                Layout.maximumWidth: 520
                                id: comboBoxText
                                leftPadding: 10
                                rightPadding: 10
                                text: {
                                    if (ModelList.selectableModels.count === 0)
                                        return qsTr("No model installed.")
                                    if (currentChat.modelLoadingError !== "")
                                        return qsTr("Model loading error.")
                                    if (currentChat.trySwitchContextInProgress === 1)
                                        return qsTr("Waiting for model...")
                                    if (currentChat.trySwitchContextInProgress === 2)
                                        return qsTr("Switching context...")
                                    if (currentModelName() === "")
                                        return qsTr("Choose a model...")
                                    if (!currentModelInstalled())
                                        return qsTr("Not found: %1").arg(currentModelName())
                                    if (currentChat.modelLoadingPercentage === 0.0)
                                        return qsTr("Reload \u00B7 %1").arg(currentModelName())
                                    if (currentChat.isCurrentlyLoading)
                                        return qsTr("Loading \u00B7 %1").arg(currentModelName())
                                    return currentModelName()
                                }
                                font.pixelSize: theme.fontSizeLarger
                                color: theme.iconBackgroundLight
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                            }
                            Item {
                                Layout.minimumWidth: updown.width
                                Layout.minimumHeight: updown.height
                                Image {
                                    id: updown
                                    anchors.verticalCenter: parent.verticalCenter
                                    sourceSize.width: comboBoxText.font.pixelSize
                                    sourceSize.height: comboBoxText.font.pixelSize
                                    mipmap: true
                                    visible: false
                                    source: "qrc:/gpt4all/icons/up_down.svg"
                                }

                                ColorOverlay {
                                    anchors.fill: updown
                                    source: updown
                                    color: comboBoxText.color
                                }
                            }
                        }
                    }
                    delegate: ItemDelegate {
                        id: comboItemDelegate
                        width: comboItemPopup.width -20
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
                    indicator: Item {
                    }
                    popup: Popup {
                        id: comboItemPopup
                        y: comboBox.height - 1
                        width: comboBox.width
                        implicitHeight: Math.min(window.height - y, contentItem.implicitHeight + 20)
                        padding: 0
                        contentItem: Rectangle {
                            implicitWidth: comboBox.width
                            implicitHeight: comboItemPopupListView.implicitHeight
                            color: "transparent"
                            radius: 10
                            ScrollView {
                                anchors.fill: parent
                                anchors.margins: 10
                                clip: true
                                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                                ListView {
                                    id: comboItemPopupListView
                                    implicitHeight: contentHeight
                                    model: comboBox.popup.visible ? comboBox.delegateModel : null
                                    currentIndex: comboBox.highlightedIndex
                                    ScrollIndicator.vertical: ScrollIndicator { }
                                }
                            }
                        }

                        background: Rectangle {
                            border.color: theme.menuBorderColor
                            border.width: 1
                            color: theme.menuBackgroundColor
                            radius: 10
                        }
                    }

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
                }

                Rectangle {
                    color: "transparent"
                    Layout.alignment: Qt.AlignRight
                    Layout.rightMargin: 30
                    Layout.fillWidth: true
                    Layout.preferredHeight: childrenRect.height
                    clip: true

                    MyButton {
                        id: collectionsButton
                        clip: true
                        anchors.right: parent.right
                        borderWidth: 0
                        backgroundColor: theme.collectionsButtonBackground
                        backgroundColorHovered: theme.collectionsButtonBackgroundHovered
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
                                    color: theme.collectionsButtonForeground
                                }
                            }

                            MyBusyIndicator {
                                visible: currentChat.collectionModel.updatingCount !== 0
                                color: theme.collectionsButtonProgress
                                size: 24
                                Layout.minimumWidth: 24
                                Layout.minimumHeight: 24
                                Text {
                                    anchors.centerIn: parent
                                    text: currentChat.collectionModel.updatingCount
                                    color: theme.collectionsButtonForeground
                                    font.pixelSize: 14 // fixed regardless of theme
                                }
                            }

                            Rectangle {
                                visible: currentChat.collectionModel.count !== 0
                                radius: 6
                                color: theme.collectionsButtonForeground
                                Layout.minimumWidth: collectionsImage.width
                                Layout.minimumHeight: collectionsImage.height
                                Text {
                                    anchors.centerIn: parent
                                    text: currentChat.collectionModel.count
                                    color: theme.collectionsButtonText
                                    font.pixelSize: 14 // fixed regardless of theme
                                }
                            }

                            Text {
                                text: qsTr("LocalDocs")
                                color: theme.collectionsButtonForeground
                                font.pixelSize: theme.fontSizeLarge
                            }
                        }

                        fontPixelSize: theme.fontSizeLarge

                        background: Rectangle {
                            radius: collectionsButton.backgroundRadius
                            // TODO(jared): either use collectionsButton-specific theming, or don't - this is inconsistent
                            color: conversation.state === "expanded" ? (
                                collectionsButton.hovered ? theme.lightButtonBackgroundHovered : theme.lightButtonBackground
                            ) : (
                                collectionsButton.hovered ? theme.lighterButtonBackground : theme.lighterButtonBackgroundHovered
                            )
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

        Rectangle {
            id: conversationDivider
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            color: theme.conversationDivider
            height: 1
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
                        z: 200
                        visible: !currentChat.isModelLoaded && (ModelList.selectableModels.count === 0 || currentModelName() === "") && !currentChat.isServer

                        ColumnLayout {
                            visible: ModelList.selectableModels.count !== 0
                            id: modelInstalledLabel
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
                        }

                        MyButton {
                            id: loadDefaultModelButton
                            visible: ModelList.selectableModels.count !== 0
                            anchors.top: modelInstalledLabel.bottom
                            anchors.topMargin: 50
                            anchors.horizontalCenter: modelInstalledLabel.horizontalCenter
                            rightPadding: 60
                            leftPadding: 60
                            property string defaultModel: ""
                            property string defaultModelName: ""
                            function updateDefaultModel() {
                                var i = comboBox.find(MySettings.userDefaultModel)
                                if (i !== -1) {
                                    defaultModel = comboBox.valueAt(i);
                                } else {
                                    defaultModel = comboBox.count ? comboBox.valueAt(0) : "";
                                }
                                if (defaultModel !== "") {
                                    defaultModelName = ModelList.modelInfo(defaultModel).name;
                                } else {
                                    defaultModelName = "";
                                }
                            }

                            text: qsTr("Load \u00B7 %1 (default) \u2192").arg(defaultModelName);
                            onClicked: {
                                var i = comboBox.find(MySettings.userDefaultModel)
                                if (i !== -1) {
                                    comboBox.changeModel(i);
                                } else {
                                    comboBox.changeModel(0);
                                }
                            }

                            // This requires a bit of work because apparently the combobox valueAt
                            // function only works after the combobox component is loaded so we have
                            // to use our own component loaded to make this work along with a signal
                            // from MySettings for when the setting for user default model changes
                            Connections {
                                target: MySettings
                                function onUserDefaultModelChanged() {
                                    loadDefaultModelButton.updateDefaultModel()
                                }
                            }
                            Component.onCompleted: {
                                loadDefaultModelButton.updateDefaultModel()
                            }
                            Accessible.role: Accessible.Button
                            Accessible.name: qsTr("Load the default model")
                            Accessible.description: qsTr("Loads the default model which can be changed in settings")
                        }

                        ColumnLayout {
                            id: noModelInstalledLabel
                            visible: ModelList.selectableModels.count === 0
                            anchors.centerIn: parent
                            spacing: 0

                            Text {
                                Layout.alignment: Qt.AlignCenter
                                text: qsTr("No Model Installed")
                                color: theme.mutedLightTextColor
                                font.pixelSize: theme.fontSizeBannerSmall
                            }

                            Text {
                                Layout.topMargin: 15
                                horizontalAlignment: Qt.AlignHCenter
                                color: theme.mutedLighterTextColor
                                text: qsTr("GPT4All requires that you install at least one\nmodel to get started")
                                font.pixelSize: theme.fontSizeLarge
                            }
                        }

                        MyButton {
                            visible: ModelList.selectableModels.count === 0
                            anchors.top: noModelInstalledLabel.bottom
                            anchors.topMargin: 50
                            anchors.horizontalCenter: noModelInstalledLabel.horizontalCenter
                            rightPadding: 60
                            leftPadding: 60
                            text: qsTr("Install a Model")
                            onClicked: {
                                addModelViewRequested();
                            }
                            Accessible.role: Accessible.Button
                            Accessible.name: qsTr("Shows the add model view")
                        }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        visible: ModelList.selectableModels.count !== 0
                        ListView {
                            id: listView
                            Layout.maximumWidth: 1280
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            Layout.margins: 20
                            Layout.leftMargin: 50
                            Layout.rightMargin: 50
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 10
                            model: chatModel
                            cacheBuffer: 2147483647

                            ScrollBar.vertical: ScrollBar {
                                policy: ScrollBar.AsNeeded
                            }

                            Accessible.role: Accessible.List
                            Accessible.name: qsTr("Conversation with the model")
                            Accessible.description: qsTr("prompt / response pairs from the conversation")

                            delegate: ChatItemView {
                                width: listView.contentItem.width - 15
                                inputBoxText: textInput.text
                                onSetInputBoxText: text => {
                                    textInput.text = text;
                                    textInput.forceActiveFocus();
                                    textInput.cursorPosition = text.length;
                                }
                                height: visible ? implicitHeight : 0
                                visible: name !== "ToolResponse: "
                            }

                            remove: Transition {
                                OpacityAnimator { to: 0; duration: 500 }
                            }

                            function scrollToEnd() {
                                listView.positionViewAtEnd()
                            }

                            onContentHeightChanged: {
                                if (atYEnd)
                                    scrollToEnd()
                            }
                        }
                    }
                }
            }

            Rectangle {
                id: conversationTrayContent
                anchors.bottom: conversationTrayButton.top
                anchors.horizontalCenter: conversationTrayButton.horizontalCenter
                width: conversationTrayContentLayout.width
                height: conversationTrayContentLayout.height
                color: theme.containerBackground
                radius: 5
                opacity: 0
                visible: false
                clip: true
                z: 400

                property bool isHovered: (
                    conversationTrayButton.isHovered || resetContextButton.hovered || copyChatButton.hovered
                )

                state: conversationTrayContent.isHovered ? "expanded" : "collapsed"
                states: [
                    State {
                        name: "expanded"
                        PropertyChanges { target: conversationTrayContent; opacity: 1 }
                    },
                    State {
                        name: "collapsed"
                        PropertyChanges { target: conversationTrayContent; opacity: 0 }
                    }
                ]
                transitions: [
                    Transition {
                        from: "collapsed"
                        to: "expanded"
                        SequentialAnimation {
                            ScriptAction {
                                script: conversationTrayContent.visible = true
                            }
                            PropertyAnimation {
                                target: conversationTrayContent
                                property: "opacity"
                                duration: 300
                                easing.type: Easing.InOutQuad
                            }
                        }
                    },
                    Transition {
                        from: "expanded"
                        to: "collapsed"
                        SequentialAnimation {
                            PropertyAnimation {
                                target: conversationTrayContent
                                property: "opacity"
                                duration: 300
                                easing.type: Easing.InOutQuad
                            }
                            ScriptAction {
                                script: conversationTrayContent.visible = false
                            }
                        }
                    }
                ]

                RowLayout {
                    id: conversationTrayContentLayout
                    spacing: 0
                    MyToolButton {
                        id: resetContextButton
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        source: "qrc:/gpt4all/icons/recycle.svg"
                        imageWidth: 20
                        imageHeight: 20
                        onClicked: resetContextDialog.open()
                        ToolTip.visible: resetContextButton.hovered
                        ToolTip.text: qsTr("Erase and reset chat session")
                    }
                    MyToolButton {
                        id: copyChatButton
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        source: "qrc:/gpt4all/icons/copy.svg"
                        imageWidth: 20
                        imageHeight: 20
                        TextEdit{
                            id: copyEdit
                            visible: false
                        }
                        onClicked: {
                            chatModel.copyToClipboard()
                            copyMessage.open()
                        }
                        ToolTip.visible: copyChatButton.hovered
                        ToolTip.text: qsTr("Copy chat session to clipboard")
                    }
                }
            }

            Item {
                id: conversationTrayButton
                anchors.bottom: textInputView.top
                anchors.horizontalCenter: textInputView.horizontalCenter
                width: 40
                height: 30
                visible: chatModel.count && !currentChat.isServer && currentChat.isModelLoaded
                property bool isHovered: conversationTrayMouseAreaButton.containsMouse
                MouseArea {
                    id: conversationTrayMouseAreaButton
                    anchors.fill: parent
                    hoverEnabled: true
                }
                Text {
                    id: conversationTrayTextButton
                    anchors.centerIn: parent
                    horizontalAlignment: Qt.AlignHCenter
                    leftPadding: 5
                    rightPadding: 5
                    text: "\u00B7\u00B7\u00B7"
                    color: theme.textColor
                    font.pixelSize: 30 // fixed size
                    font.bold: true
                }
            }

            MyButton {
                anchors.bottom: textInputView.top
                anchors.horizontalCenter: textInputView.horizontalCenter
                anchors.bottomMargin: 20
                textColor: theme.textColor
                visible: !currentChat.isServer
                    && !currentChat.isModelLoaded
                    && currentChat.modelLoadingError === ""
                    && !currentChat.trySwitchContextInProgress
                    && !currentChat.isCurrentlyLoading
                    && currentModelInstalled()

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 15
                    sourceSize.width: 15
                    sourceSize.height: 15
                    source: "qrc:/gpt4all/icons/regenerate.svg"
                }
                leftPadding: 40
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
                text: qsTr("Reload \u00B7 %1").arg(currentChat.modelInfo.name)
                fontPixelSize: theme.fontSizeSmall
                Accessible.description: qsTr("Reloads the model")
            }

            Text {
                id: statusBar
                property string externalHoveredLink: ""
                anchors.top: textInputView.bottom
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.rightMargin: 30
                anchors.left: parent.left
                anchors.leftMargin: 30
                horizontalAlignment: Qt.AlignRight
                verticalAlignment: Qt.AlignVCenter
                color: textInputView.error !== null ? theme.textErrorColor : theme.mutedTextColor
                visible: currentChat.tokenSpeed !== "" || externalHoveredLink !== "" || textInputView.error !== null
                elide: Text.ElideRight
                wrapMode: Text.WordWrap
                text: {
                    if (externalHoveredLink !== "")
                        return externalHoveredLink
                    if (textInputView.error !== null)
                        return textInputView.error;

                    const segments = [currentChat.tokenSpeed];
                    const device = currentChat.device;
                    const backend = currentChat.deviceBackend;
                    if (device !== null) { // device is null if we have no model loaded
                        var deviceSegment = device;
                        if (backend === "CUDA" || backend === "Vulkan")
                            deviceSegment += ` (${backend})`;
                        segments.push(deviceSegment);
                    }
                    const fallbackReason = currentChat.fallbackReason;
                    if (fallbackReason !== null && fallbackReason !== "")
                        segments.push(fallbackReason);
                    return segments.join(" \u00B7 ");
                }
                font.pixelSize: theme.fontSizeSmaller
                font.bold: true
                onLinkActivated: function(link) { Qt.openUrlExternally(link) }
            }

            RectangularGlow {
                id: effect
                visible: !currentChat.isServer && ModelList.selectableModels.count !== 0
                anchors.fill: textInputView
                glowRadius: 50
                spread: 0
                color: theme.sendGlow
                cornerRadius: 10
                opacity: 0.1
            }

            ListModel {
                id: attachmentModel

                function getAttachmentUrls() {
                    var urls = [];
                    for (var i = 0; i < attachmentModel.count; i++) {
                        var item = attachmentModel.get(i);
                        urls.push(item.url);
                    }
                    return urls;
                }
            }

            Rectangle {
                id: textInputView
                color: theme.controlBackground
                border.width: error === null ? 1 : 2
                border.color: error === null ? theme.controlBorder : theme.textErrorColor
                radius: 10
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 30
                anchors.leftMargin: Math.max((parent.width - 1310) / 2, 30)
                anchors.rightMargin: Math.max((parent.width - 1310) / 2, 30)
                height: textInputViewLayout.implicitHeight
                visible: !currentChat.isServer && ModelList.selectableModels.count !== 0

                property var error: null
                function checkError() {
                    const info = currentModelInfo;
                    if (info === null || !info.id) {
                        error = null;
                    } else if (info.chatTemplate.isLegacy) {
                        error = qsTr("Legacy prompt template needs to be " +
                                     "<a href=\"https://docs.gpt4all.io/gpt4all_desktop/chat_templates.html\">updated" +
                                     "</a> in Settings.");
                    } else if (!info.chatTemplate.isSet) {
                        error = qsTr("No <a href=\"https://docs.gpt4all.io/gpt4all_desktop/chat_templates.html\">" +
                                     "chat template</a> configured.");
                    } else if (/^\s*$/.test(info.chatTemplate.value)) {
                        error = qsTr("The <a href=\"https://docs.gpt4all.io/gpt4all_desktop/chat_templates.html\">" +
                                     "chat template</a> cannot be blank.");
                    } else if (info.systemMessage.isLegacy) {
                        error = qsTr("Legacy system prompt needs to be " +
                                     "<a href=\"https://docs.gpt4all.io/gpt4all_desktop/chat_templates.html\">updated" +
                                     "</a> in Settings.");
                    } else
                        error = null;
                }
                Component.onCompleted: checkError()
                Connections {
                    target: window
                    function onCurrentModelIdChanged() { textInputView.checkError(); }
                }
                Connections {
                    target: MySettings
                    function onChatTemplateChanged(info)
                    { if (info.id === window.currentModelId) textInputView.checkError(); }
                    function onSystemMessageChanged(info)
                    { if (info.id === window.currentModelId) textInputView.checkError(); }
                }

                MouseArea {
                    id: textInputViewMouseArea
                    anchors.fill: parent
                    onClicked: (mouse) => {
                        if (textInput.enabled)
                            textInput.forceActiveFocus();
                    }
                }

                GridLayout {
                    id: textInputViewLayout
                    anchors.left: parent.left
                    anchors.right: parent.right
                    rows: 2
                    columns: 3
                    rowSpacing: 10
                    columnSpacing: 0
                    Flow {
                        id: attachmentsFlow
                        visible: attachmentModel.count
                        Layout.row: 0
                        Layout.column: 1
                        Layout.topMargin: 15
                        Layout.leftMargin: 5
                        Layout.rightMargin: 15
                        spacing: 10

                        Repeater {
                            model: attachmentModel

                            Rectangle {
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
                                        fileName: model.file
                                    }

                                    Text {
                                        width: 265
                                        height: 40
                                        text: model.file
                                        color: theme.textColor
                                        horizontalAlignment: Text.AlignHLeft
                                        verticalAlignment: Text.AlignVCenter
                                        font.pixelSize: theme.fontSizeMedium
                                        font.bold: true
                                        wrapMode: Text.WrapAnywhere
                                        elide: Qt.ElideRight
                                    }
                                }

                                MyMiniButton {
                                    id: removeAttachmentButton
                                    anchors.top: parent.top
                                    anchors.right: parent.right
                                    backgroundColor: theme.textColor
                                    backgroundColorHovered: theme.iconBackgroundDark
                                    source: "qrc:/gpt4all/icons/close.svg"
                                    onClicked: {
                                        attachmentModel.remove(index)
                                        if (textInput.enabled)
                                            textInput.forceActiveFocus();
                                    }
                                }
                            }
                        }
                    }

                    MyToolButton {
                        id: plusButton
                        Layout.row: 1
                        Layout.column: 0
                        Layout.leftMargin: 15
                        Layout.rightMargin: 15
                        Layout.alignment: Qt.AlignCenter
                        backgroundColor: theme.conversationInputButtonBackground
                        backgroundColorHovered: theme.conversationInputButtonBackgroundHovered
                        imageWidth: theme.fontSizeLargest
                        imageHeight: theme.fontSizeLargest
                        visible: !currentChat.isServer && ModelList.selectableModels.count !== 0 && currentChat.isModelLoaded
                        enabled: !currentChat.responseInProgress
                        source: "qrc:/gpt4all/icons/paperclip.svg"
                        Accessible.name: qsTr("Add media")
                        Accessible.description: qsTr("Adds media to the prompt")

                        onClicked: (mouse) => {
                                       addMediaMenu.open()
                                   }
                    }

                    ScrollView {
                        id: textInputScrollView
                        Layout.row: 1
                        Layout.column: 1
                        Layout.fillWidth: true
                        Layout.leftMargin: plusButton.visible ? 5 : 15
                        Layout.margins: 15
                        height: Math.min(contentHeight, 200)

                        MyTextArea {
                            id: textInput
                            color: theme.textColor
                            padding: 0
                            enabled: currentChat.isModelLoaded && !currentChat.isServer
                            onEnabledChanged: {
                                if (textInput.enabled)
                                    textInput.forceActiveFocus();
                            }
                            font.pixelSize: theme.fontSizeLarger
                            placeholderText: currentChat.isModelLoaded ? qsTr("Send a message...") : qsTr("Load a model to continue...")
                            Accessible.role: Accessible.EditableText
                            Accessible.name: placeholderText
                            Accessible.description: qsTr("Send messages/prompts to the model")
                            Keys.onReturnPressed: event => {
                                if (event.modifiers & Qt.ControlModifier || event.modifiers & Qt.ShiftModifier) {
                                    event.accepted = false;
                                } else if (!chatModel.hasError && textInputView.error === null) {
                                    editingFinished();
                                    sendMessage();
                                }
                            }
                            function sendMessage() {
                                if ((textInput.text === "" && attachmentModel.count === 0) || currentChat.responseInProgress)
                                    return

                                currentChat.stopGenerating()
                                currentChat.newPromptResponsePair(textInput.text, attachmentModel.getAttachmentUrls())
                                attachmentModel.clear();
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

                            background: Rectangle {
                                implicitWidth: 150
                                color: "transparent"
                            }

                            MyMenu {
                                id: textInputContextMenu
                                MyMenuItem {
                                    text: qsTr("Cut")
                                    enabled: textInput.selectedText !== ""
                                    height: enabled ? implicitHeight : 0
                                    onTriggered: textInput.cut()
                                }
                                MyMenuItem {
                                    text: qsTr("Copy")
                                    enabled: textInput.selectedText !== ""
                                    height: enabled ? implicitHeight : 0
                                    onTriggered: textInput.copy()
                                }
                                MyMenuItem {
                                    text: qsTr("Paste")
                                    onTriggered: textInput.paste()
                                }
                                MyMenuItem {
                                    text: qsTr("Select All")
                                    onTriggered: textInput.selectAll()
                                }
                            }
                        }
                    }

                    Row {
                        Layout.row: 1
                        Layout.column: 2
                        Layout.rightMargin: 15
                        Layout.alignment: Qt.AlignCenter

                        MyToolButton {
                            id: stopButton
                            backgroundColor: theme.conversationInputButtonBackground
                            backgroundColorHovered: theme.conversationInputButtonBackgroundHovered
                            visible: currentChat.responseInProgress && !currentChat.isServer

                            background: Item {
                                anchors.fill: parent
                                Image {
                                    id: stopImage
                                    anchors.centerIn: parent
                                    visible: false
                                    fillMode: Image.PreserveAspectFit
                                    mipmap: true
                                    sourceSize.width: theme.fontSizeLargest
                                    sourceSize.height: theme.fontSizeLargest
                                    source: "qrc:/gpt4all/icons/stop_generating.svg"
                                }
                                Rectangle {
                                    anchors.centerIn: stopImage
                                    width: theme.fontSizeLargest + 8
                                    height: theme.fontSizeLargest + 8
                                    color: theme.viewBackground
                                    border.pixelAligned: false
                                    border.color: theme.controlBorder
                                    border.width: 1
                                    radius: width / 2
                                }
                                ColorOverlay {
                                    anchors.fill: stopImage
                                    source: stopImage
                                    color: stopButton.hovered ? stopButton.backgroundColorHovered : stopButton.backgroundColor
                                }
                            }

                            Accessible.name: qsTr("Stop generating")
                            Accessible.description: qsTr("Stop the current response generation")
                            ToolTip.visible: stopButton.hovered
                            ToolTip.text: Accessible.description

                            onClicked: {
                                // FIXME: This no longer sets a 'stopped' field so conversations that
                                // are copied to clipboard or to datalake don't indicate if the user
                                // has prematurely stopped the response. This has been broken since
                                // v3.0.0 at least.
                                currentChat.stopGenerating()
                            }
                        }

                        MyToolButton {
                            id: sendButton
                            backgroundColor: theme.conversationInputButtonBackground
                            backgroundColorHovered: theme.conversationInputButtonBackgroundHovered
                            imageWidth: theme.fontSizeLargest
                            imageHeight: theme.fontSizeLargest
                            visible: !currentChat.responseInProgress && !currentChat.isServer && ModelList.selectableModels.count !== 0
                            enabled: !chatModel.hasError && textInputView.error === null
                            source: "qrc:/gpt4all/icons/send_message.svg"
                            Accessible.name: qsTr("Send message")
                            Accessible.description: qsTr("Sends the message/prompt contained in textfield to the model")
                            ToolTip.visible: sendButton.hovered
                            ToolTip.text: Accessible.description

                            onClicked: {
                                textInput.sendMessage()
                            }
                        }
                    }
                }
            }

            MyFileDialog {
                id: fileDialog
                nameFilters: ["All Supported Files (*.txt *.md *.rst *.xlsx)", "Text Files (*.txt *.md *.rst)", "Excel Worksheets (*.xlsx)"]
            }

            MyMenu {
                id: addMediaMenu
                x: textInputView.x
                y: textInputView.y - addMediaMenu.height - 10;
                title: qsTr("Attach")
                MyMenuItem {
                    text: qsTr("Single File")
                    icon.source: "qrc:/gpt4all/icons/file.svg"
                    icon.width: 24
                    icon.height: 24
                    onClicked: {
                        fileDialog.openFileDialog(StandardPaths.writableLocation(StandardPaths.HomeLocation), function(selectedFile) {
                            if (selectedFile) {
                                var file = selectedFile.toString().split("/").pop()
                                attachmentModel.append({
                                    file: file,
                                    url: selectedFile
                                })
                            }
                            if (textInput.enabled)
                                textInput.forceActiveFocus();
                        })
                    }
                }
            }
        }
    }
}
