import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import chatlistmodel
import download
import llm
import modellist
import network
import mysettings

Rectangle {
    id: modelDownloaderDialog
    color: theme.containerBackground

    function showEmbeddingModels() {
        Network.sendModelDownloaderDialog();
        ModelList.downloadableModels.expanded = true
        var targetModelIndex = ModelList.defaultEmbeddingModelIndex
        modelListView.positionViewAtIndex(targetModelIndex, ListView.Beginning)
    }

    PopupDialog {
        id: downloadingErrorPopup
        anchors.centerIn: parent
        shouldTimeOut: false
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 30

        Label {
            id: listLabel
            text: qsTr("Discover and Download Models")
            visible: true
            Layout.fillWidth: true
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            color: theme.titleTextColor
            font.pixelSize: theme.fontSizeLargest
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignCenter
            Layout.margins: 0
            spacing: 10
            MyTextField {
                id: discoverField
                property string textBeingSearched: ""
                readOnly: ModelList.discoverInProgress
                Layout.alignment: Qt.AlignCenter
                Layout.preferredWidth: 720
                Layout.preferredHeight: 90
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
                            discoverField.text = qsTr("Searching \u00B7 ") + discoverField.textBeingSearched;
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
                    ModelList.downloadableModels.discoverAndFilter(discoverField.text);
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
                        contentItem: Text {
                            color: clearDiscoverButton.hovered ? theme.iconBackgroundDark : theme.textColor
                            text: "\u2715"
                            font.pixelSize: theme.fontSizeLarge
                        }
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
                model: [qsTr("Default"), qsTr("Likes"), qsTr("Downloads"), qsTr("Recent")]
                currentIndex: ModelList.discoverSort
                contentItem: Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    rightPadding: 30
                    color: theme.textColor
                    text: {
                        return qsTr("Sort by: ") + comboSort.displayText
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
                model: [qsTr("Asc"), qsTr("Desc")]
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
                        return qsTr("Sort dir: ") + comboSortDirection.displayText
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
                model: ["5", "10", "20", "50", "100", qsTr("None")]
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
                        return qsTr("Limit: ") + comboLimit.displayText
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

        Label {
            visible: !ModelList.downloadableModels.count && !ModelList.asyncModelRequestOngoing
            Layout.fillWidth: true
            Layout.fillHeight: true
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            text: qsTr("Network error: could not retrieve http://gpt4all.io/models/models3.json")
            font.pixelSize: theme.fontSizeLarge
            color: theme.mutedTextColor
        }

        MyBusyIndicator {
            visible: !ModelList.downloadableModels.count && ModelList.asyncModelRequestOngoing
            running: ModelList.asyncModelRequestOngoing
            Accessible.role: Accessible.Animation
            Layout.alignment: Qt.AlignCenter
            Accessible.name: qsTr("Busy indicator")
            Accessible.description: qsTr("Displayed when the models request is ongoing")
        }

        ScrollView {
            id: scrollView
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: modelListView
                model: ModelList.downloadableModels
                boundsBehavior: Flickable.StopAtBounds

                delegate: Rectangle {
                    id: delegateItem
                    width: modelListView.width
                    height: childrenRect.height
                    color: index % 2 === 0 ? theme.darkContrast : theme.lightContrast

                    GridLayout {
                        columns: 2
                        width: parent.width

                        Text {
                            textFormat: Text.StyledText
                            text: "<h2>" + name + "</h2>"
                            font.pixelSize: theme.fontSizeLarger
                            Layout.row: 0
                            Layout.column: 0
                            Layout.topMargin: 20
                            Layout.leftMargin: 20
                            Layout.columnSpan: 2
                            color: theme.titleTextColor
                            Accessible.role: Accessible.Paragraph
                            Accessible.name: qsTr("Model file")
                            Accessible.description: qsTr("Model file to be downloaded")
                        }

                        Rectangle {
                            id: actionBox
                            width: childrenRect.width + 20
                            color: theme.containerBackground
                            border.color: theme.accentColor
                            border.width: 1
                            radius: 10
                            Layout.row: 1
                            Layout.column: 1
                            Layout.rightMargin: 20
                            Layout.bottomMargin: 20
                            Layout.fillHeight: true
                            Layout.minimumHeight: childrenRect.height + 20
                            Layout.alignment: Qt.AlignRight | Qt.AlignTop
                            Layout.rowSpan: 2

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
                                    visible: installed || downloadError !== ""
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
                                        if (apiKey.text === "")
                                            apiKey.showError();
                                        else
                                            Download.installModel(filename, apiKey.text);
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
                                        textFormat: Text.StyledText
                                        text: "<strong><font size=\"1\">"
                                            + (qsTr("Status: ")
                                            + (installed ? qsTr("Installed")
                                            : (downloadError !== "" ? qsTr("<a href=\"#error\">Error</a>")
                                            : (isDownloading ? qsTr("Downloading") : qsTr("Available")))))
                                            + "</strong></font>"
                                        color: theme.textColor
                                        font.pixelSize: theme.fontSizeLarge
                                        linkColor: theme.textErrorColor
                                        Accessible.role: Accessible.Paragraph
                                        Accessible.name: text
                                        Accessible.description: qsTr("Whether the file is already installed on your system")
                                        onLinkActivated: {
                                            downloadingErrorPopup.text = downloadError;
                                            downloadingErrorPopup.open();
                                        }
                                    }

                                    Label {
                                        Layout.leftMargin: 20
                                        textFormat: Text.StyledText
                                        text: "<strong><font size=\"1\">"
                                            + (qsTr("File size: ") + filesize)
                                            + (ramrequired < 0 ? "" : "<br>" + (qsTr("RAM required: ") + (ramrequired > 0 ? ramrequired + " GB" : qsTr("minimal"))))
                                            + (parameters === "" ? "" : "<br>" + qsTr("Parameters: ") + parameters)
                                            + (quant === "" ? "" : "<br>" + (qsTr("Quantization: ") + quant))
                                            + (type === "" ? "" : "<br>" + (qsTr("Type: ") + type))
                                            + "</strong></font>"
                                        color: theme.textColor
                                        font.pixelSize: theme.fontSizeLarge
                                        Accessible.role: Accessible.Paragraph
                                        Accessible.name: text
                                        Accessible.description: qsTr("Metadata about the model")
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
                                        text: qsTr("<strong><font size=\"2\">WARNING: Not recommended for your hardware.")
                                            + qsTr(" Model requires more memory (") + ramrequired
                                            + qsTr(" GB) than your system has available (")
                                            + LLM.systemTotalRAMInGBString() + ").</strong></font>"
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
                                        apiKey.placeholderTextColor = theme.textErrorColor
                                    }
                                    onTextChanged: {
                                        apiKey.placeholderTextColor = theme.mutedTextColor
                                    }
                                    placeholderText: qsTr("enter $API_KEY")
                                    Accessible.role: Accessible.EditableText
                                    Accessible.name: placeholderText
                                    Accessible.description: qsTr("Whether the file hash is being calculated")
                                }
                            }
                        }

                        Text {
                            id: descriptionText
                            text: description
                            font.pixelSize: theme.fontSizeLarge
                            Layout.row: 1
                            Layout.column: 0
                            Layout.leftMargin: 20
                            Layout.bottomMargin: 20
                            Layout.maximumWidth: modelListView.width - actionBox.width - 60
                            wrapMode: Text.WordWrap
                            textFormat: Text.StyledText
                            color: theme.textColor
                            linkColor: theme.textColor
                            Accessible.role: Accessible.Paragraph
                            Accessible.name: qsTr("Description")
                            Accessible.description: qsTr("File description")
                            onLinkActivated: Qt.openUrlExternally(link)
                        }
                    }
                }

                footer: Component {
                    Rectangle {
                        width: modelListView.width
                        height: expandButton.height + 80
                        color: ModelList.downloadableModels.count % 2 === 0 ? theme.darkContrast : theme.lightContrast
                        MySettingsButton {
                            id: expandButton
                            anchors.centerIn: parent
                            padding: 40
                            text: ModelList.downloadableModels.expanded ? qsTr("Show fewer models") : qsTr("Show more models")
                            onClicked: {
                                ModelList.downloadableModels.expanded = !ModelList.downloadableModels.expanded;
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignCenter
            Layout.fillWidth: true
            spacing: 20
            FolderDialog {
                id: modelPathDialog
                title: "Please choose a directory"
                currentFolder: "file://" + MySettings.modelPath
                onAccepted: {
                    MySettings.modelPath = selectedFolder
                }
            }
            MySettingsLabel {
                id: modelPathLabel
                text: qsTr("Download path")
                font.pixelSize: theme.fontSizeLarge
                color: theme.textColor
                Layout.row: 1
                Layout.column: 0
            }
            MyDirectoryField {
                id: modelPathDisplayField
                text: MySettings.modelPath
                font.pixelSize: theme.fontSizeLarge
                Layout.fillWidth: true
                ToolTip.text: qsTr("Path where model files will be downloaded to")
                ToolTip.visible: hovered
                Accessible.role: Accessible.ToolTip
                Accessible.name: modelPathDisplayField.text
                Accessible.description: ToolTip.text
                onEditingFinished: {
                    if (isValid) {
                        MySettings.modelPath = modelPathDisplayField.text
                    } else {
                        text = MySettings.modelPath
                    }
                }
            }
            MySettingsButton {
                text: qsTr("Browse")
                Accessible.description: qsTr("Choose where to save model files")
                onClicked: modelPathDialog.open()
            }
        }
    }
}
