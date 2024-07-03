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
    signal addModelViewRequested()

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
                     + "<li>Read more about what models are supported in our <a href=\"https://docs.gpt4all.io/\">documentation</a> for the gui"
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
                                        && (currentChat.isModelLoaded || currentModelName() !== "")
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
                                        return qsTr("No model installed...")
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
                        width: comboItemPopup.width
                        contentItem: Text {
                            text: name
                            color: theme.textColor
                            font: comboBox.font
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: highlighted ? theme.lightContrast : theme.darkContrast
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
                            color: theme.controlBackground
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
                            function updateDefaultModel() {
                                var i = comboBox.find(MySettings.userDefaultModel)
                                if (i !== -1) {
                                    defaultModel = comboBox.valueAt(i);
                                } else {
                                    defaultModel = comboBox.valueAt(0);
                                }
                            }

                            text: qsTr("Load \u00B7 ") + defaultModel + qsTr(" (default) \u2192");
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
                        visible: ModelList.selectableModels.count !== 0 && chatModel.count !== 0
                        ListView {
                            id: listView
                            Layout.maximumWidth: 1280
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            Layout.margins: 20
                            Layout.leftMargin: 50
                            Layout.rightMargin: 50
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 25
                            model: chatModel

                            ScrollBar.vertical: ScrollBar {
                                policy: ScrollBar.AsNeeded
                            }

                            Accessible.role: Accessible.List
                            Accessible.name: qsTr("Conversation with the model")
                            Accessible.description: qsTr("prompt / response pairs from the conversation")

                            delegate: GridLayout {
                                width: listView.contentItem.width - 15
                                rows: 3
                                columns: 2

                                Item {
                                    Layout.row: 0
                                    Layout.column: 0
                                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                                    Layout.preferredWidth: 32
                                    Layout.preferredHeight: 32
                                    Image {
                                        id: logo
                                        sourceSize: Qt.size(32, 32)
                                        fillMode: Image.PreserveAspectFit
                                        mipmap: true
                                        visible: false
                                        source: name !== qsTr("Response: ") ? "qrc:/gpt4all/icons/you.svg" : "qrc:/gpt4all/icons/gpt4all_transparent.svg"
                                    }

                                    ColorOverlay {
                                        anchors.fill: logo
                                        source: logo
                                        color: theme.conversationHeader
                                    }
                                }

                                Item {
                                    Layout.row: 0
                                    Layout.column: 1
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 38
                                    RowLayout {
                                        spacing: 5
                                        anchors.left: parent.left
                                        anchors.top: parent.top
                                        anchors.bottom: parent.bottom

                                        TextArea {
                                            text: name === qsTr("Response: ") ? qsTr("GPT4All") : qsTr("You")
                                            padding: 0
                                            font.pixelSize: theme.fontSizeLarger
                                            font.bold: true
                                            color: theme.conversationHeader
                                            readOnly: true
                                        }
                                        Text {
                                            visible: name === qsTr("Response: ")
                                            font.pixelSize: theme.fontSizeLarger
                                            text: currentModelName()
                                            color: theme.mutedTextColor
                                        }
                                        RowLayout {
                                            visible: (currentResponse ? true : false) && ((value === "" && currentChat.responseInProgress) || currentChat.isRecalc)
                                            MyBusyIndicator {
                                                size: 24
                                                color: theme.conversationProgress
                                                Accessible.role: Accessible.Animation
                                                Accessible.name: qsTr("Busy indicator")
                                                Accessible.description: qsTr("The model is thinking")
                                            }
                                            Text {
                                                color: theme.mutedTextColor
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
                                }

                                ColumnLayout {
                                    Layout.row: 1
                                    Layout.column: 1
                                    Layout.fillWidth: true
                                    TextArea {
                                        id: myTextArea
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
                                            if (!currentResponse || !currentChat.responseInProgress)
                                                Qt.openUrlExternally(link)
                                        }

                                        onLinkHovered: function (link) {
                                            if (!currentResponse || !currentChat.responseInProgress)
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
                                                    myTextArea.text = value
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
                                            textProcessor.setValue(value);
                                        }

                                        Component.onCompleted: {
                                            resetChatViewTextProcessor();
                                            chatModel.valueChanged.connect(function(i, value) {
                                                if (index === i)
                                                    textProcessor.setValue(value);
                                                }
                                            );
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
                                        Accessible.description: name === qsTr("Response: ") ? "The response by the model" : "The prompt by the user"
                                    }

                                    ThumbsDownDialog {
                                        id: thumbsDownDialog
                                        x: Math.round((parent.width - width) / 2)
                                        y: Math.round((parent.height - height) / 2)
                                        width: 640
                                        height: 300
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
                                                anchors.topMargin: 3
                                                anchors.left: thumbsUp.right
                                                anchors.leftMargin: 3
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

                                Item {
                                    Layout.row: 2
                                    Layout.column: 1
                                    Layout.topMargin: 5
                                    Layout.alignment: Qt.AlignVCenter
                                    Layout.preferredWidth: childrenRect.width
                                    Layout.preferredHeight: childrenRect.height
                                    visible: consolidatedSources.length !== 0 && MySettings.localDocsShowReferences && (!currentResponse || !currentChat.responseInProgress)

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
                                                text: qsTr("%1 Sources").arg(consolidatedSources.length)
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
                                    visible: consolidatedSources.length !== 0 && MySettings.localDocsShowReferences && (!currentResponse || !currentChat.responseInProgress)
                                    clip: true
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 0
                                    state: "collapsed"
                                    states: [
                                        State {
                                            name: "expanded"
                                            PropertyChanges { target: sourcesLayout; Layout.preferredHeight: childrenRect.height }
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
                                                        Item {
                                                            Layout.preferredWidth: 24
                                                            Layout.preferredHeight: 24
                                                            Image {
                                                                id: fileIcon
                                                                anchors.fill: parent
                                                                visible: false
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
                                                            ColorOverlay {
                                                                anchors.fill: fileIcon
                                                                source: fileIcon
                                                                color: theme.textColor
                                                            }
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
                            }

                            property bool shouldAutoScroll: true
                            property bool isAutoScrolling: false

                            Connections {
                                target: currentChat
                                function onResponseChanged() {
                                    listView.scrollToEnd()
                                }
                            }

                            function scrollToEnd() {
                                if (listView.shouldAutoScroll) {
                                    listView.isAutoScrolling = true
                                    listView.positionViewAtEnd()
                                    listView.isAutoScrolling = false
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
                                height: 0
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

                property bool isHovered: {
                    return conversationTrayButton.isHovered ||
                        resetContextButton.hovered || copyChatButton.hovered ||
                        regenerateButton.hovered || stopButton.hovered
                }

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
                        onClicked: {
                            Network.trackChatEvent("reset_context", { "length": chatModel.count })
                            currentChat.reset();
                            currentChat.processSystemPrompt();
                        }
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
                            var conversation = getConversation()
                            copyEdit.text = conversation
                            copyEdit.selectAll()
                            copyEdit.copy()
                            copyMessage.open()
                        }
                        ToolTip.visible: copyChatButton.hovered
                        ToolTip.text: qsTr("Copy chat session to clipboard")
                    }
                    MyToolButton {
                        id: regenerateButton
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        source: "qrc:/gpt4all/icons/regenerate.svg"
                        imageWidth: 20
                        imageHeight: 20
                        visible: chatModel.count && !currentChat.isServer && currentChat.isModelLoaded && !currentChat.responseInProgress
                        onClicked: {
                            var index = Math.max(0, chatModel.count - 1);
                            var listElement = chatModel.get(index);
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
                        ToolTip.visible: regenerateButton.hovered
                        ToolTip.text: qsTr("Redo last chat response")
                    }
                    MyToolButton {
                        id: stopButton
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        source: "qrc:/gpt4all/icons/stop_generating.svg"
                        imageWidth: 20
                        imageHeight: 20
                        visible: currentChat.responseInProgress
                        onClicked: {
                            var index = Math.max(0, chatModel.count - 1);
                            var listElement = chatModel.get(index);
                            listElement.stopped = true
                            currentChat.stopGenerating()
                        }
                        ToolTip.visible: stopButton.hovered
                        ToolTip.text: qsTr("Stop the current response generation")
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
                    && currentModelName() !== ""

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
                text: qsTr("Reload \u00B7 ") + currentChat.modelInfo.name
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
                color: theme.mutedTextColor
                visible: currentChat.tokenSpeed !== "" || externalHoveredLink !== ""
                elide: Text.ElideRight
                wrapMode: Text.WordWrap
                text: {
                    if (externalHoveredLink !== "")
                        return externalHoveredLink

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

            ScrollView {
                id: textInputView
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 30
                anchors.leftMargin: Math.max((parent.width - 1310) / 2, 30)
                anchors.rightMargin: Math.max((parent.width - 1310) / 2, 30)
                height: Math.min(contentHeight, 200)
                visible: !currentChat.isServer && ModelList.selectableModels.count !== 0
                MyTextArea {
                    id: textInput
                    color: theme.textColor
                    topPadding: 15
                    bottomPadding: 15
                    leftPadding: 20
                    rightPadding: 40
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

            MyToolButton {
                id: sendButton
                backgroundColor: theme.sendButtonBackground
                backgroundColorHovered: theme.sendButtonBackgroundHovered
                anchors.right: textInputView.right
                anchors.verticalCenter: textInputView.verticalCenter
                anchors.rightMargin: 15
                imageWidth: theme.fontSizeLargest
                imageHeight: theme.fontSizeLargest
                visible: !currentChat.isServer && ModelList.selectableModels.count !== 0
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
