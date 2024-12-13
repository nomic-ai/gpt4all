import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.folderlistmodel
import Qt5Compat.GraphicalEffects

import llm
import chatlistmodel
import download
import modellist
import network
import gpt4all
import mysettings
import localdocs

ColumnLayout {
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.alignment: Qt.AlignTop
    spacing: 5

    Label {
        Layout.topMargin: 0
        Layout.bottomMargin: 25
        Layout.rightMargin: 150 * theme.fontScale
        Layout.alignment: Qt.AlignTop
        Layout.fillWidth: true
        verticalAlignment: Text.AlignTop
        text: qsTr("Use the search to find and download models from HuggingFace. There is NO GUARANTEE that these " +
                   "will work. Many will require additional configuration before they can be used.")
        font.pixelSize: theme.fontSizeLarger
        color: theme.textColor
        wrapMode: Text.WordWrap
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignCenter
        Layout.margins: 0
        spacing: 10
        MyTextField {
            id: discoverField
            property string textBeingSearched: ""
            readOnly: ModelList.discoverInProgress
            Layout.alignment: Qt.AlignCenter
            Layout.fillWidth: true
            font.pixelSize: theme.fontSizeLarger
            placeholderText: qsTr("Discover and download models by keyword search...")
            Accessible.role: Accessible.EditableText
            Accessible.name: placeholderText
            Accessible.description: qsTr("Text field for discovering and filtering downloadable models")
            Connections {
                target: ModelList
                function onDiscoverInProgressChanged() {
                    if (ModelList.discoverInProgress) {
                        discoverField.textBeingSearched = discoverField.text;
                        discoverField.text = qsTr("Searching \u00B7 %1").arg(discoverField.textBeingSearched);
                    } else {
                        discoverField.text = discoverField.textBeingSearched;
                        discoverField.textBeingSearched = "";
                    }
                }
            }
            background: ProgressBar {
                id: discoverProgressBar
                indeterminate: ModelList.discoverInProgress && ModelList.discoverProgress === 0.0
                value: ModelList.discoverProgress
                background: Rectangle {
                    color: theme.controlBackground
                    border.color: theme.controlBorder
                    radius: 10
                }
                contentItem: Item {
                    Rectangle {
                        visible: ModelList.discoverInProgress
                        anchors.bottom: parent.bottom
                        width: discoverProgressBar.visualPosition * parent.width
                        height: 10
                        radius: 2
                        color: theme.progressForeground
                    }
                }
            }

            Keys.onReturnPressed: (event)=> {
                                      if (event.modifiers & Qt.ControlModifier || event.modifiers & Qt.ShiftModifier)
                                      event.accepted = false;
                                      else {
                                          editingFinished();
                                          sendDiscovery()
                                      }
                                  }
            function sendDiscovery() {
                ModelList.huggingFaceDownloadableModels.discoverAndFilter(discoverField.text);
            }
            RowLayout {
                spacing: 0
                anchors.right: discoverField.right
                anchors.verticalCenter: discoverField.verticalCenter
                anchors.rightMargin: 15
                visible: !ModelList.discoverInProgress
                MyMiniButton {
                    id: clearDiscoverButton
                    backgroundColor: theme.textColor
                    backgroundColorHovered: theme.iconBackgroundDark
                    visible: discoverField.text !== ""
                    source: "qrc:/gpt4all/icons/close.svg"
                    onClicked: {
                        discoverField.text = ""
                        discoverField.sendDiscovery() // should clear results
                    }
                }
                MyMiniButton {
                    backgroundColor: theme.textColor
                    backgroundColorHovered: theme.iconBackgroundDark
                    source: "qrc:/gpt4all/icons/settings.svg"
                    onClicked: {
                        discoveryTools.visible = !discoveryTools.visible
                    }
                }
                MyMiniButton {
                    id: sendButton
                    enabled: !ModelList.discoverInProgress
                    backgroundColor: theme.textColor
                    backgroundColorHovered: theme.iconBackgroundDark
                    source: "qrc:/gpt4all/icons/send_message.svg"
                    Accessible.name: qsTr("Initiate model discovery and filtering")
                    Accessible.description: qsTr("Triggers discovery and filtering of models")
                    onClicked: {
                        discoverField.sendDiscovery()
                    }
                }
            }
        }
    }

    RowLayout {
        id: discoveryTools
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignCenter
        Layout.margins: 0
        spacing: 20
        visible: false
        MyComboBox {
            id: comboSort
            model: ListModel {
                ListElement { name: qsTr("Default") }
                ListElement { name: qsTr("Likes") }
                ListElement { name: qsTr("Downloads") }
                ListElement { name: qsTr("Recent") }
            }
            currentIndex: ModelList.discoverSort
            contentItem: Text {
                anchors.horizontalCenter: parent.horizontalCenter
                rightPadding: 30
                color: theme.textColor
                text: {
                    return qsTr("Sort by: %1").arg(comboSort.displayText)
                }
                font.pixelSize: theme.fontSizeLarger
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }
            onActivated: function (index) {
                ModelList.discoverSort = index;
            }
        }
        MyComboBox {
            id: comboSortDirection
            model: ListModel {
                ListElement { name: qsTr("Asc") }
                ListElement { name: qsTr("Desc") }
            }
            currentIndex: {
                if (ModelList.discoverSortDirection === 1)
                    return 0
                else
                    return 1;
            }
            contentItem: Text {
                anchors.horizontalCenter: parent.horizontalCenter
                rightPadding: 30
                color: theme.textColor
                text: {
                    return qsTr("Sort dir: %1").arg(comboSortDirection.displayText)
                }
                font.pixelSize: theme.fontSizeLarger
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }
            onActivated: function (index) {
                if (index === 0)
                    ModelList.discoverSortDirection = 1;
                else
                    ModelList.discoverSortDirection = -1;
            }
        }
        MyComboBox {
            id: comboLimit
            model: ListModel {
                ListElement { name: "5" }
                ListElement { name: "10" }
                ListElement { name: "20" }
                ListElement { name: "50" }
                ListElement { name: "100" }
                ListElement { name: qsTr("None") }
            }

            currentIndex: {
                if (ModelList.discoverLimit === 5)
                    return 0;
                else if (ModelList.discoverLimit === 10)
                    return 1;
                else if (ModelList.discoverLimit === 20)
                    return 2;
                else if (ModelList.discoverLimit === 50)
                    return 3;
                else if (ModelList.discoverLimit === 100)
                    return 4;
                else if (ModelList.discoverLimit === -1)
                    return 5;
            }
            contentItem: Text {
                anchors.horizontalCenter: parent.horizontalCenter
                rightPadding: 30
                color: theme.textColor
                text: {
                    return qsTr("Limit: %1").arg(comboLimit.displayText)
                }
                font.pixelSize: theme.fontSizeLarger
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }
            onActivated: function (index) {
                switch (index) {
                case 0:
                    ModelList.discoverLimit = 5; break;
                case 1:
                    ModelList.discoverLimit = 10; break;
                case 2:
                    ModelList.discoverLimit = 20; break;
                case 3:
                    ModelList.discoverLimit = 50; break;
                case 4:
                    ModelList.discoverLimit = 100; break;
                case 5:
                    ModelList.discoverLimit = -1; break;
                }
            }
        }
    }

    ScrollView {
        id: scrollView
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true

        ListView {
            id: modelListView
            model: ModelList.huggingFaceDownloadableModels
            boundsBehavior: Flickable.StopAtBounds
            spacing: 30

            delegate: Rectangle {
                id: delegateItem
                width: modelListView.width
                height: childrenRect.height + 60
                color: theme.conversationBackground
                radius: 10
                border.width: 1
                border.color: theme.controlBorder

                ColumnLayout {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 30

                    Text {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignLeft
                        text: name
                        elide: Text.ElideRight
                        color: theme.titleTextColor
                        font.pixelSize: theme.fontSizeLargest
                        font.bold: true
                        Accessible.role: Accessible.Paragraph
                        Accessible.name: qsTr("Model file")
                        Accessible.description: qsTr("Model file to be downloaded")
                    }


                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: theme.dividerColor
                    }

                    RowLayout {
                        Layout.topMargin: 10
                        Layout.fillWidth: true
                        Text {
                            id: descriptionText
                            text: description
                            font.pixelSize: theme.fontSizeLarge
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            textFormat: Text.StyledText
                            color: theme.textColor
                            linkColor: theme.textColor
                            Accessible.role: Accessible.Paragraph
                            Accessible.name: qsTr("Description")
                            Accessible.description: qsTr("File description")
                            onLinkActivated: function(link) { Qt.openUrlExternally(link); }
                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.NoButton // pass clicks to parent
                                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                            }
                        }

                        // FIXME Need to overhaul design here which must take into account
                        // features not present in current figma including:
                        // * Ability to cancel a current download
                        // * Ability to resume a download
                        // * The presentation of an error if encountered
                        // * Whether to show already installed models
                        // * Install of remote models with API keys
                        // * The presentation of the progress bar
                        Rectangle {
                            id: actionBox
                            width: childrenRect.width + 20
                            color: "transparent"
                            border.width: 1
                            border.color: theme.dividerColor
                            radius: 10
                            Layout.rightMargin: 20
                            Layout.bottomMargin: 20
                            Layout.minimumHeight: childrenRect.height + 20
                            Layout.alignment: Qt.AlignRight | Qt.AlignTop

                            ColumnLayout {
                                spacing: 0
                                MySettingsButton {
                                    id: downloadButton
                                    text: isDownloading ? qsTr("Cancel") : isIncomplete ? qsTr("Resume") : qsTr("Download")
                                    font.pixelSize: theme.fontSizeLarge
                                    Layout.topMargin: 20
                                    Layout.leftMargin: 20
                                    Layout.minimumWidth: 200
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                                    visible: !isOnline && !installed && !calcHash && downloadError === ""
                                    Accessible.description: qsTr("Stop/restart/start the download")
                                    onClicked: {
                                        if (!isDownloading) {
                                            Download.downloadModel(filename);
                                        } else {
                                            Download.cancelDownload(filename);
                                        }
                                    }
                                }

                                MySettingsDestructiveButton {
                                    id: removeButton
                                    text: qsTr("Remove")
                                    Layout.topMargin: 20
                                    Layout.leftMargin: 20
                                    Layout.minimumWidth: 200
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                                    visible: !isDownloading && (installed || isIncomplete)
                                    Accessible.description: qsTr("Remove model from filesystem")
                                    onClicked: {
                                        Download.removeModel(filename);
                                    }
                                }

                                MySettingsButton {
                                    id: installButton
                                    visible: !installed && isOnline
                                    Layout.topMargin: 20
                                    Layout.leftMargin: 20
                                    Layout.minimumWidth: 200
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                                    text: qsTr("Install")
                                    font.pixelSize: theme.fontSizeLarge
                                    onClicked: {
                                        var apiKeyText = apiKey.text.trim(),
                                        baseUrlText = baseUrl.text.trim(),
                                        modelNameText = modelName.text.trim();

                                        var apiKeyOk = apiKeyText !== "",
                                        baseUrlOk = !isCompatibleApi || baseUrlText !== "",
                                        modelNameOk = !isCompatibleApi || modelNameText !== "";

                                        if (!apiKeyOk)
                                            apiKey.showError();
                                        if (!baseUrlOk)
                                            baseUrl.showError();
                                        if (!modelNameOk)
                                            modelName.showError();

                                        if (!apiKeyOk || !baseUrlOk || !modelNameOk)
                                            return;

                                        if (!isCompatibleApi)
                                            Download.installModel(
                                                        filename,
                                                        apiKeyText,
                                                        );
                                        else
                                            Download.installCompatibleModel(
                                                        modelNameText,
                                                        apiKeyText,
                                                        baseUrlText,
                                                        );
                                    }
                                    Accessible.role: Accessible.Button
                                    Accessible.name: qsTr("Install")
                                    Accessible.description: qsTr("Install online model")
                                }

                                ColumnLayout {
                                    spacing: 0
                                    Label {
                                        Layout.topMargin: 20
                                        Layout.leftMargin: 20
                                        visible: downloadError !== ""
                                        textFormat: Text.StyledText
                                        text: qsTr("<strong><font size=\"1\"><a href=\"#error\">Error</a></strong></font>")
                                        color: theme.textColor
                                        font.pixelSize: theme.fontSizeLarge
                                        linkColor: theme.textErrorColor
                                        Accessible.role: Accessible.Paragraph
                                        Accessible.name: text
                                        Accessible.description: qsTr("Describes an error that occurred when downloading")
                                        onLinkActivated: {
                                            downloadingErrorPopup.text = downloadError;
                                            downloadingErrorPopup.open();
                                        }
                                    }

                                    Label {
                                        visible: LLM.systemTotalRAMInGB() < ramrequired
                                        Layout.topMargin: 20
                                        Layout.leftMargin: 20
                                        Layout.maximumWidth: 300
                                        textFormat: Text.StyledText
                                        text: qsTr("<strong><font size=\"2\">WARNING: Not recommended for your hardware. Model requires more memory (%1 GB) than your system has available (%2).</strong></font>").arg(ramrequired).arg(LLM.systemTotalRAMInGBString())
                                        color: theme.textErrorColor
                                        font.pixelSize: theme.fontSizeLarge
                                        wrapMode: Text.WordWrap
                                        Accessible.role: Accessible.Paragraph
                                        Accessible.name: text
                                        Accessible.description: qsTr("Error for incompatible hardware")
                                        onLinkActivated: {
                                            downloadingErrorPopup.text = downloadError;
                                            downloadingErrorPopup.open();
                                        }
                                    }
                                }

                                ColumnLayout {
                                    visible: isDownloading && !calcHash
                                    Layout.topMargin: 20
                                    Layout.leftMargin: 20
                                    Layout.minimumWidth: 200
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                                    spacing: 20

                                    ProgressBar {
                                        id: itemProgressBar
                                        Layout.fillWidth: true
                                        width: 200
                                        value: bytesReceived / bytesTotal
                                        background: Rectangle {
                                            implicitHeight: 45
                                            color: theme.progressBackground
                                            radius: 3
                                        }
                                        contentItem: Item {
                                            implicitHeight: 40

                                            Rectangle {
                                                width: itemProgressBar.visualPosition * parent.width
                                                height: parent.height
                                                radius: 2
                                                color: theme.progressForeground
                                            }
                                        }
                                        Accessible.role: Accessible.ProgressBar
                                        Accessible.name: qsTr("Download progressBar")
                                        Accessible.description: qsTr("Shows the progress made in the download")
                                    }

                                    Label {
                                        id: speedLabel
                                        color: theme.textColor
                                        Layout.alignment: Qt.AlignRight
                                        text: speed
                                        font.pixelSize: theme.fontSizeLarge
                                        Accessible.role: Accessible.Paragraph
                                        Accessible.name: qsTr("Download speed")
                                        Accessible.description: qsTr("Download speed in bytes/kilobytes/megabytes per second")
                                    }
                                }

                                RowLayout {
                                    visible: calcHash
                                    Layout.topMargin: 20
                                    Layout.leftMargin: 20
                                    Layout.minimumWidth: 200
                                    Layout.maximumWidth: 200
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                                    clip: true

                                    Label {
                                        id: calcHashLabel
                                        color: theme.textColor
                                        text: qsTr("Calculating...")
                                        font.pixelSize: theme.fontSizeLarge
                                        Accessible.role: Accessible.Paragraph
                                        Accessible.name: text
                                        Accessible.description: qsTr("Whether the file hash is being calculated")
                                    }

                                    MyBusyIndicator {
                                        id: busyCalcHash
                                        running: calcHash
                                        Accessible.role: Accessible.Animation
                                        Accessible.name: qsTr("Busy indicator")
                                        Accessible.description: qsTr("Displayed when the file hash is being calculated")
                                    }
                                }

                                MyTextField {
                                    id: apiKey
                                    visible: !installed && isOnline
                                    Layout.topMargin: 20
                                    Layout.leftMargin: 20
                                    Layout.minimumWidth: 200
                                    Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                                    wrapMode: Text.WrapAnywhere
                                    function showError() {
                                        messageToast.show(qsTr("ERROR: $API_KEY is empty."));
                                        apiKey.placeholderTextColor = theme.textErrorColor;
                                    }
                                    onTextChanged: {
                                        apiKey.placeholderTextColor = theme.mutedTextColor;
                                    }
                                    placeholderText: qsTr("enter $API_KEY")
                                    Accessible.role: Accessible.EditableText
                                    Accessible.name: placeholderText
                                    Accessible.description: qsTr("Whether the file hash is being calculated")
                                }

                                MyTextField {
                                    id: baseUrl
                                    visible: !installed && isOnline && isCompatibleApi
                                    Layout.topMargin: 20
                                    Layout.leftMargin: 20
                                    Layout.minimumWidth: 200
                                    Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                                    wrapMode: Text.WrapAnywhere
                                    function showError() {
                                        messageToast.show(qsTr("ERROR: $BASE_URL is empty."));
                                        baseUrl.placeholderTextColor = theme.textErrorColor;
                                    }
                                    onTextChanged: {
                                        baseUrl.placeholderTextColor = theme.mutedTextColor;
                                    }
                                    placeholderText: qsTr("enter $BASE_URL")
                                    Accessible.role: Accessible.EditableText
                                    Accessible.name: placeholderText
                                    Accessible.description: qsTr("Whether the file hash is being calculated")
                                }

                                MyTextField {
                                    id: modelName
                                    visible: !installed && isOnline && isCompatibleApi
                                    Layout.topMargin: 20
                                    Layout.leftMargin: 20
                                    Layout.minimumWidth: 200
                                    Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                                    wrapMode: Text.WrapAnywhere
                                    function showError() {
                                        messageToast.show(qsTr("ERROR: $MODEL_NAME is empty."))
                                        modelName.placeholderTextColor = theme.textErrorColor;
                                    }
                                    onTextChanged: {
                                        modelName.placeholderTextColor = theme.mutedTextColor;
                                    }
                                    placeholderText: qsTr("enter $MODEL_NAME")
                                    Accessible.role: Accessible.EditableText
                                    Accessible.name: placeholderText
                                    Accessible.description: qsTr("Whether the file hash is being calculated")
                                }
                            }
                        }
                    }

                    Item  {
                        Layout.minimumWidth: childrenRect.width
                        Layout.minimumHeight: childrenRect.height
                        Layout.bottomMargin: 10
                        RowLayout {
                            id: paramRow
                            anchors.centerIn: parent
                            ColumnLayout {
                                Layout.topMargin: 10
                                Layout.bottomMargin: 10
                                Layout.leftMargin: 20
                                Layout.rightMargin: 20
                                Text {
                                    text: qsTr("File size")
                                    font.pixelSize: theme.fontSizeSmall
                                    color: theme.mutedDarkTextColor
                                }
                                Text {
                                    text: filesize
                                    color: theme.textColor
                                    font.pixelSize: theme.fontSizeSmall
                                    font.bold: true
                                }
                            }
                            Rectangle {
                                width: 1
                                Layout.fillHeight: true
                                color: theme.dividerColor
                            }
                            ColumnLayout {
                                Layout.topMargin: 10
                                Layout.bottomMargin: 10
                                Layout.leftMargin: 20
                                Layout.rightMargin: 20
                                Text {
                                    text: qsTr("Quant")
                                    font.pixelSize: theme.fontSizeSmall
                                    color: theme.mutedDarkTextColor
                                }
                                Text {
                                    text: quant
                                    color: theme.textColor
                                    font.pixelSize: theme.fontSizeSmall
                                    font.bold: true
                                }
                            }
                            Rectangle {
                                width: 1
                                Layout.fillHeight: true
                                color: theme.dividerColor
                            }
                            ColumnLayout {
                                Layout.topMargin: 10
                                Layout.bottomMargin: 10
                                Layout.leftMargin: 20
                                Layout.rightMargin: 20
                                Text {
                                    text: qsTr("Type")
                                    font.pixelSize: theme.fontSizeSmall
                                    color: theme.mutedDarkTextColor
                                }
                                Text {
                                    text: type
                                    color: theme.textColor
                                    font.pixelSize: theme.fontSizeSmall
                                    font.bold: true
                                }
                            }
                        }

                        Rectangle {
                            color: "transparent"
                            anchors.fill: paramRow
                            border.color: theme.dividerColor
                            border.width: 1
                            radius: 10
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: theme.dividerColor
                    }
                }
            }
        }
    }
}
