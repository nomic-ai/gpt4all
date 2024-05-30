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

    signal addModelViewRequested()

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

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: 50

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft
                Layout.minimumWidth: 200
                spacing: 5

                Text {
                    id: welcome
                    text: qsTr("Installed Models")
                    font.pixelSize: theme.fontSizeBanner
                    color: theme.titleTextColor
                }

                Text {
                    text: qsTr("Locally installed large language models")
                    font.pixelSize: theme.fontSizeLarge
                    color: theme.mutedTextColor
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 0
            }

            MyButton {
                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                text: qsTr("\uFF0B Add Model")
                onClicked: {
                    addModelViewRequested()
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
                spacing: 30

                delegate: Rectangle {
                    id: delegateItem
                    width: modelListView.width
                    height: childrenRect.height + 60
                    color: theme.conversationBackground
                    radius: 10
                    border.width: 1
                    border.color: theme.controlBorder

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
                            visible: false
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

//                footer: Component {
//                    Rectangle {
//                        width: modelListView.width
//                        height: expandButton.height + 80
//                        color: ModelList.downloadableModels.count % 2 === 0 ? theme.darkContrast : theme.lightContrast
//                        MySettingsButton {
//                            id: expandButton
//                            anchors.centerIn: parent
//                            padding: 40
//                            text: ModelList.downloadableModels.expanded ? qsTr("Show fewer models") : qsTr("Show more models")
//                            onClicked: {
//                                ModelList.downloadableModels.expanded = !ModelList.downloadableModels.expanded;
//                            }
//                        }
//                    }
//                }
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
