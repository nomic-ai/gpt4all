import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import download
import llm
import network

Dialog {
    id: modelDownloaderDialog
    modal: true
    opacity: 0.9
    closePolicy: LLM.chatListModel.currentChat.modelList.length === 0 ? Popup.NoAutoClose : (Popup.CloseOnEscape | Popup.CloseOnPressOutside)
    background: Rectangle {
        anchors.fill: parent
        anchors.margins: -20
        color: theme.backgroundDarkest
        border.width: 1
        border.color: theme.dialogBorder
        radius: 10
    }

    onOpened: {
        Network.sendModelDownloaderDialog();
    }

    property string defaultModelPath: Download.defaultLocalModelsPath()
    property alias modelPath: settings.modelPath
    Settings {
        id: settings
        property string modelPath: modelDownloaderDialog.defaultModelPath
    }

    Component.onCompleted: {
        Download.downloadLocalModelsPath = settings.modelPath
    }

    Component.onDestruction: {
        settings.sync()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 30

        Label {
            id: listLabel
            text: "Available Models:"
            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true
            color: theme.textColor
        }

        ScrollView {
            id: scrollView
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: modelList
                model: Download.modelList
                boundsBehavior: Flickable.StopAtBounds

                delegate: Item {
                    id: delegateItem
                    width: modelList.width
                    height: modelName.height + modelName.padding
                        + description.height + description.padding
                    objectName: "delegateItem"
                    property bool downloading: false
                    Rectangle {
                        anchors.fill: parent
                        color: index % 2 === 0 ? theme.backgroundLight : theme.backgroundLighter
                    }

                    Text {
                        id: modelName
                        objectName: "modelName"
                        property string filename: modelData.filename
                        text: !modelData.isChatGPT ? filename.slice(5, filename.length - 4) : filename
                        padding: 20
                        anchors.top: parent.top
                        anchors.left: parent.left
                        font.bold: modelData.isDefault || modelData.bestGPTJ || modelData.bestLlama || modelData.bestMPT
                        color: theme.assistantColor
                        Accessible.role: Accessible.Paragraph
                        Accessible.name: qsTr("Model file")
                        Accessible.description: qsTr("Model file to be downloaded")
                    }

                    Text {
                        id: description
                        text: "    - " + modelData.description
                        leftPadding: 20
                        rightPadding: 20
                        anchors.top: modelName.bottom
                        anchors.left: modelName.left
                        anchors.right: parent.right
                        wrapMode: Text.WordWrap
                        textFormat: Text.StyledText
                        color: theme.textColor
                        linkColor: theme.textColor
                        Accessible.role: Accessible.Paragraph
                        Accessible.name: qsTr("Description")
                        Accessible.description: qsTr("The description of the file")
                        onLinkActivated: Qt.openUrlExternally(link)
                    }

                    Text {
                        id: isDefault
                        text: qsTr("(default)")
                        visible: modelData.isDefault
                        anchors.top: modelName.top
                        anchors.left: modelName.right
                        padding: 20
                        color: theme.textColor
                        Accessible.role: Accessible.Paragraph
                        Accessible.name: qsTr("Default file")
                        Accessible.description: qsTr("Whether the file is the default model")
                    }

                    Text {
                        text: modelData.filesize
                        anchors.top: modelName.top
                        anchors.left: isDefault.visible ? isDefault.right : modelName.right
                        padding: 20
                        color: theme.textColor
                        Accessible.role: Accessible.Paragraph
                        Accessible.name: qsTr("File size")
                        Accessible.description: qsTr("The size of the file")
                    }

                    Label {
                        id: speedLabel
                        anchors.top: modelName.top
                        anchors.right: itemProgressBar.left
                        padding: 20
                        objectName: "speedLabel"
                        color: theme.textColor
                        text: ""
                        visible: downloading
                        Accessible.role: Accessible.Paragraph
                        Accessible.name: qsTr("Download speed")
                        Accessible.description: qsTr("Download speed in bytes/kilobytes/megabytes per second")
                    }

                    ProgressBar {
                        id: itemProgressBar
                        objectName: "itemProgressBar"
                        anchors.top: modelName.top
                        anchors.right: downloadButton.left
                        anchors.topMargin: 20
                        anchors.rightMargin: 20
                        width: 100
                        visible: downloading
                        background: Rectangle {
                            implicitWidth: 200
                            implicitHeight: 30
                            color: theme.backgroundDarkest
                            radius: 3
                        }

                        contentItem: Item {
                            implicitWidth: 200
                            implicitHeight: 25

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

                    Item {
                        visible: modelData.calcHash
                        anchors.top: modelName.top
                        anchors.right: parent.right

                        Label {
                            id: calcHashLabel
                            anchors.right: busyCalcHash.left
                            padding: 20
                            objectName: "calcHashLabel"
                            color: theme.textColor
                            text: qsTr("Calculating MD5...")
                            Accessible.role: Accessible.Paragraph
                            Accessible.name: text
                            Accessible.description: qsTr("Whether the file hash is being calculated")
                        }

                        BusyIndicator {
                            id: busyCalcHash
                            anchors.right: parent.right
                            padding: 20
                            running: modelData.calcHash
                            Accessible.role: Accessible.Animation
                            Accessible.name: qsTr("Busy indicator")
                            Accessible.description: qsTr("Displayed when the file hash is being calculated")
                        }
                    }

                    Item {
                        anchors.top: modelName.top
                        anchors.topMargin: 15
                        anchors.right: parent.right
                        visible: modelData.installed

                        Label {
                            id: installedLabel
                            anchors.verticalCenter: removeButton.verticalCenter
                            anchors.right: removeButton.left
                            anchors.rightMargin: 15
                            objectName: "installedLabel"
                            color: theme.textColor
                            text: qsTr("Already installed")
                            Accessible.role: Accessible.Paragraph
                            Accessible.name: text
                            Accessible.description: qsTr("Whether the file is already installed on your system")
                        }

                        Button {
                            id: removeButton
                            contentItem: Text {
                                color: theme.textColor
                                text: "Remove"
                            }
                            anchors.right: parent.right
                            anchors.rightMargin: 20
                            background: Rectangle {
                                opacity: .5
                                border.color: theme.backgroundLightest
                                border.width: 1
                                radius: 10
                                color: theme.backgroundLight
                            }
                            onClicked: {
                                Download.removeModel(modelData.filename);
                            }
                            Accessible.role: Accessible.Button
                            Accessible.name: qsTr("Remove button")
                            Accessible.description: qsTr("Remove button to remove model from filesystem")
                        }
                    }

                    Item {
                        visible: modelData.isChatGPT && !modelData.installed
                        anchors.top: modelName.top
                        anchors.topMargin: 15
                        anchors.right: parent.right

                        TextField {
                            id: openaiKey
                            anchors.right: installButton.left
                            anchors.rightMargin: 15
                            color: theme.textColor
                            background: Rectangle {
                                color: theme.backgroundLighter
                                radius: 10
                            }
                            placeholderText: qsTr("enter $OPENAI_API_KEY")
                            placeholderTextColor: theme.backgroundLightest
                            Accessible.role: Accessible.EditableText
                            Accessible.name: placeholderText
                            Accessible.description: qsTr("Whether the file hash is being calculated")
                        }

                        Button {
                            id: installButton
                            contentItem: Text {
                                color: openaiKey.text === "" ? theme.backgroundLightest : theme.textColor
                                text: "Install"
                            }
                            enabled: openaiKey.text !== ""
                            anchors.right: parent.right
                            anchors.rightMargin: 20
                            background: Rectangle {
                                opacity: .5
                                border.color: theme.backgroundLightest
                                border.width: 1
                                radius: 10
                                color: theme.backgroundLight
                            }
                            onClicked: {
                                Download.installModel(modelData.filename, openaiKey.text);
                            }
                            Accessible.role: Accessible.Button
                            Accessible.name: qsTr("Install button")
                            Accessible.description: qsTr("Install button to install chatgpt model")
                        }
                    }

                    Button {
                        id: downloadButton
                        contentItem: Text {
                            color: theme.textColor
                            text: downloading ? "Cancel" : "Download"
                        }
                        anchors.top: modelName.top
                        anchors.right: parent.right
                        anchors.topMargin: 15
                        anchors.rightMargin: 20
                        visible: !modelData.isChatGPT && !modelData.installed && !modelData.calcHash
                        onClicked: {
                            if (!downloading) {
                                downloading = true;
                                Download.downloadModel(modelData.filename);
                            } else {
                                downloading = false;
                                Download.cancelDownload(modelData.filename);
                            }
                        }
                        background: Rectangle {
                            opacity: .5
                            border.color: theme.backgroundLightest
                            border.width: 1
                            radius: 10
                            color: theme.backgroundLight
                        }
                        Accessible.role: Accessible.Button
                        Accessible.name: text
                        Accessible.description: qsTr("Cancel/Download button to stop/start the download")

                    }
                }

                Component.onCompleted: {
                    Download.downloadProgress.connect(updateProgress);
                    Download.downloadFinished.connect(resetProgress);
                }

                property var lastUpdate: ({})

                function updateProgress(bytesReceived, bytesTotal, modelName) {
                    let currentTime = new Date().getTime();

                    for (let i = 0; i < modelList.contentItem.children.length; i++) {
                        let delegateItem = modelList.contentItem.children[i];
                        if (delegateItem.objectName === "delegateItem") {
                            let modelNameText = delegateItem.children.find(child => child.objectName === "modelName").filename;
                            if (modelNameText === modelName) {
                                let progressBar = delegateItem.children.find(child => child.objectName === "itemProgressBar");
                                progressBar.value = bytesReceived / bytesTotal;

                                // Calculate the download speed
                                if (lastUpdate[modelName] && lastUpdate[modelName].timestamp) {
                                    let timeDifference = currentTime - lastUpdate[modelName].timestamp;
                                    let bytesDifference = bytesReceived - lastUpdate[modelName].bytesReceived;
                                    let speed = (bytesDifference / timeDifference) * 1000; // bytes per second
                                    delegateItem.downloading = true

                                    // Update the speed label
                                    let speedLabel = delegateItem.children.find(child => child.objectName === "speedLabel");
                                    if (speed < 1024) {
                                        speedLabel.text = speed.toFixed(2) + " B/s";
                                    } else if (speed < 1024 * 1024) {
                                        speedLabel.text = (speed / 1024).toFixed(2) + " KB/s";
                                    } else {
                                        speedLabel.text = (speed / (1024 * 1024)).toFixed(2) + " MB/s";
                                    }
                                }

                                // Update the lastUpdate object for the current model
                                lastUpdate[modelName] = {"timestamp": currentTime, "bytesReceived": bytesReceived};
                                break;
                            }
                        }
                    }
                }

                function resetProgress(modelName) {
                    for (let i = 0; i < modelList.contentItem.children.length; i++) {
                        let delegateItem = modelList.contentItem.children[i];
                        if (delegateItem.objectName === "delegateItem") {
                            let modelNameText = delegateItem.children.find(child => child.objectName === "modelName").filename;
                            if (modelNameText === modelName) {
                                let progressBar = delegateItem.children.find(child => child.objectName === "itemProgressBar");
                                progressBar.value = 0;
                                delegateItem.downloading = false;

                                // Remove speed label text
                                let speedLabel = delegateItem.children.find(child => child.objectName === "speedLabel");
                                speedLabel.text = "";

                                // Remove the lastUpdate object for the canceled model
                                delete lastUpdate[modelName];
                                break;
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
                currentFolder: Download.downloadLocalModelsPath
                onAccepted: {
                    Download.downloadLocalModelsPath = selectedFolder
                    settings.modelPath = Download.downloadLocalModelsPath
                    settings.sync()
                }
            }
            Label {
                id: modelPathLabel
                text: qsTr("Download path:")
                color: theme.textColor
                Layout.row: 1
                Layout.column: 0
            }
            TextField {
                id: modelPathDisplayLabel
                text: Download.downloadLocalModelsPath
                readOnly: true
                color: theme.textColor
                Layout.fillWidth: true
                ToolTip.text: qsTr("Path where model files will be downloaded to")
                ToolTip.visible: hovered
                Accessible.role: Accessible.ToolTip
                Accessible.name: modelPathDisplayLabel.text
                Accessible.description: ToolTip.text
                background: Rectangle {
                    color: theme.backgroundLighter
                    radius: 10
                }
            }
            Button {
                text: qsTr("Browse")
                contentItem: Text {
                    text: qsTr("Browse")
                    horizontalAlignment: Text.AlignHCenter
                    color: theme.textColor
                    Accessible.role: Accessible.Button
                    Accessible.name: text
                    Accessible.description: qsTr("Opens a folder picker dialog to choose where to save model files")
                }
                background: Rectangle {
                    opacity: .5
                    border.color: theme.backgroundLightest
                    border.width: 1
                    radius: 10
                    color: theme.backgroundLight
                }
                onClicked: modelPathDialog.open()
            }
        }
    }
}
