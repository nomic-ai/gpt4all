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

MyDialog {
    id: modelDownloaderDialog
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 10
    property bool showEmbeddingModels: false

    onOpened: {
        Network.sendModelDownloaderDialog();

        if (showEmbeddingModels) {
            ModelList.downloadableModels.expanded = true
            var targetModelIndex = ModelList.defaultEmbeddingModelIndex
            modelListView.positionViewAtIndex(targetModelIndex, ListView.Contain)
        }
    }

    PopupDialog {
        id: downloadingErrorPopup
        anchors.centerIn: parent
        shouldTimeOut: false
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 30

        Label {
            id: listLabel
            text: qsTr("Available Models:")
            font.pixelSize: theme.fontSizeLarge
            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true
            color: theme.textColor
        }

        Label {
            visible: !ModelList.downloadableModels.count && !ModelList.asyncModelRequestOngoing
            Layout.fillWidth: true
            Layout.fillHeight: true
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            text: qsTr("Network error: could not retrieve http://gpt4all.io/models/models2.json")
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
                    color: index % 2 === 0 ? theme.backgroundLight : theme.backgroundLighter

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
                            color: theme.assistantColor
                            Accessible.role: Accessible.Paragraph
                            Accessible.name: qsTr("Model file")
                            Accessible.description: qsTr("Model file to be downloaded")
                        }

                        Rectangle {
                            id: actionBox
                            width: childrenRect.width + 20
                            color: theme.backgroundDark
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
                                MyButton {
                                    id: downloadButton
                                    text: isDownloading ? qsTr("Cancel") : isIncomplete ? qsTr("Resume") : qsTr("Download")
                                    font.pixelSize: theme.fontSizeLarge
                                    Layout.topMargin: 20
                                    Layout.leftMargin: 20
                                    Layout.minimumWidth: openaiKey.width
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                                    visible: !isChatGPT && !installed && !calcHash && downloadError === ""
                                    Accessible.description: qsTr("Stop/restart/start the download")
                                    background: Rectangle {
                                        border.color: downloadButton.down ? theme.backgroundLightest : theme.buttonBorder
                                        border.width: 2
                                        radius: 10
                                        color: downloadButton.hovered ? theme.backgroundDark : theme.backgroundDarkest
                                    }
                                    onClicked: {
                                        if (!isDownloading) {
                                            Download.downloadModel(filename);
                                        } else {
                                            Download.cancelDownload(filename);
                                        }
                                    }
                                }

                                MyButton {
                                    id: removeButton
                                    text: qsTr("Remove")
                                    font.pixelSize: theme.fontSizeLarge
                                    Layout.topMargin: 20
                                    Layout.leftMargin: 20
                                    Layout.minimumWidth: openaiKey.width
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                                    visible: installed || downloadError !== ""
                                    Accessible.description: qsTr("Remove model from filesystem")
                                    background: Rectangle {
                                        border.color: removeButton.down ? theme.backgroundLightest : theme.buttonBorder
                                        border.width: 2
                                        radius: 10
                                        color: removeButton.hovered ? theme.backgroundDark : theme.backgroundDarkest
                                    }
                                    onClicked: {
                                        Download.removeModel(filename);
                                    }
                                }

                                MyButton {
                                    id: installButton
                                    visible: !installed && isChatGPT
                                    Layout.topMargin: 20
                                    Layout.leftMargin: 20
                                    Layout.minimumWidth: openaiKey.width
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                                    text: qsTr("Install")
                                    font.pixelSize: theme.fontSizeLarge
                                    background: Rectangle {
                                        border.color: installButton.down ? theme.backgroundLightest : theme.buttonBorder
                                        border.width: 2
                                        radius: 10
                                        color: installButton.hovered ? theme.backgroundDark : theme.backgroundDarkest
                                    }
                                    onClicked: {
                                        if (openaiKey.text === "")
                                            openaiKey.showError();
                                        else
                                            Download.installModel(filename, openaiKey.text);
                                    }
                                    Accessible.role: Accessible.Button
                                    Accessible.name: qsTr("Install")
                                    Accessible.description: qsTr("Install chatGPT model")
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
                                            + (qsTr("Download size: ") + filesize)
                                            + "<br>"
                                            + (qsTr("RAM required: ") + (ramrequired > 0 ? ramrequired + " GB" : qsTr("minimal")))
                                            + "<br>"
                                            + (qsTr("Parameters: ") + parameters)
                                            + "<br>"
                                            + (qsTr("Quantization: ") + quant)
                                            + "<br>"
                                            + (qsTr("Type: ") + type)
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
                                        Layout.maximumWidth: openaiKey.width
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
                                    Layout.minimumWidth: openaiKey.width
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                                    spacing: 20

                                    ProgressBar {
                                        id: itemProgressBar
                                        Layout.fillWidth: true
                                        width: openaiKey.width
                                        value: bytesReceived / bytesTotal
                                        background: Rectangle {
                                            implicitHeight: 45
                                            color: theme.backgroundDarkest
                                            radius: 3
                                        }
                                        contentItem: Item {
                                            implicitHeight: 40

                                            Rectangle {
                                                width: itemProgressBar.visualPosition * parent.width
                                                height: parent.height
                                                radius: 2
                                                color: theme.assistantColor
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
                                    Layout.minimumWidth: openaiKey.width
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignTop | Qt.AlignHCenter

                                    Label {
                                        id: calcHashLabel
                                        color: theme.textColor
                                        text: qsTr("Calculating MD5...")
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
                                    id: openaiKey
                                    visible: !installed && isChatGPT
                                    Layout.topMargin: 20
                                    Layout.leftMargin: 20
                                    Layout.minimumWidth: 150
                                    Layout.maximumWidth: textMetrics.width + 25
                                    Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                                    color: theme.textColor
                                    background: Rectangle {
                                        color: theme.backgroundLighter
                                        radius: 10
                                    }
                                    wrapMode: Text.WrapAnywhere
                                    function showError() {
                                        openaiKey.placeholderTextColor = theme.textErrorColor
                                    }
                                    onTextChanged: {
                                        openaiKey.placeholderTextColor = theme.backgroundLightest
                                    }
                                    placeholderText: qsTr("enter $OPENAI_API_KEY")
                                    font.pixelSize: theme.fontSizeLarge
                                    placeholderTextColor: theme.backgroundLightest
                                    Accessible.role: Accessible.EditableText
                                    Accessible.name: placeholderText
                                    Accessible.description: qsTr("Whether the file hash is being calculated")
                                    TextMetrics {
                                        id: textMetrics
                                        font: openaiKey.font
                                        text: openaiKey.placeholderText
                                    }
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
                        color: ModelList.downloadableModels.count % 2 === 0 ? theme.backgroundLight : theme.backgroundLighter
                        MyButton {
                            id: expandButton
                            anchors.centerIn: parent
                            padding: 40
                            text: ModelList.downloadableModels.expanded ? qsTr("Show fewer models") : qsTr("Show more models")
                            background: Rectangle {
                                border.color: expandButton.down ? theme.backgroundLightest : theme.buttonBorder
                                border.width: 2
                                radius: 10
                                color: expandButton.hovered ? theme.backgroundDark : theme.backgroundDarkest
                            }
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
            Label {
                id: modelPathLabel
                text: qsTr("Download path:")
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
            MyButton {
                text: qsTr("Browse")
                Accessible.description: qsTr("Choose where to save model files")
                onClicked: modelPathDialog.open()
            }
        }
    }
}
