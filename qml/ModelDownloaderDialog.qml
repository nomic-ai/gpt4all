import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import download
import llm

Dialog {
    id: modelDownloaderDialog
    width: 1024
    height: 400
    title: "Model Downloader"
    modal: true
    opacity: 0.9
    closePolicy: LLM.modelList.length === 0 ? Popup.NoAutoClose : (Popup.CloseOnEscape | Popup.CloseOnPressOutside)
    background: Rectangle {
        anchors.fill: parent
        anchors.margins: -20
        color: "#202123"
        border.width: 1
        border.color: "white"
        radius: 10
    }

    Component.onCompleted: {
        if (LLM.modelList.length === 0)
            open();
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        Label {
            id: listLabel
            text: "Available Models:"
            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true
            color: "#d1d5db"
        }

        ListView {
            id: modelList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: Download.modelList
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            delegate: Item {
                id: delegateItem
                width: modelList.width
                height: 50
                objectName: "delegateItem"
                property bool downloading: false

                Rectangle {
                    anchors.fill: parent
                    color: index % 2 === 0 ? "#2c2f33" : "#1e2125"
                }

                Text {
                    id: modelName
                    objectName: "modelName"
                    property string filename: modelData.filename
                    text: filename.slice(5, filename.length - 4)
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    color: "#d1d5db"
                    Accessible.role: Accessible.Paragraph
                    Accessible.name: qsTr("Model file")
                    Accessible.description: qsTr("Model file to be downloaded")
                }

                Text {
                    id: isDefault
                    text: qsTr("(default)")
                    visible: modelData.isDefault
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: modelName.right
                    anchors.leftMargin: 10
                    color: "#d1d5db"
                    Accessible.role: Accessible.Paragraph
                    Accessible.name: qsTr("Default file")
                    Accessible.description: qsTr("Whether the file is the default model")
                }

                Text {
                    text: modelData.filesize
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: isDefault.visible ? isDefault.right : modelName.right
                    anchors.leftMargin: 10
                    color: "#d1d5db"
                    Accessible.role: Accessible.Paragraph
                    Accessible.name: qsTr("File size")
                    Accessible.description: qsTr("The size of the file")
                }

                Label {
                    id: speedLabel
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: itemProgressBar.left
                    anchors.rightMargin: 10
                    objectName: "speedLabel"
                    color: "#d1d5db"
                    text: ""
                    visible: downloading
                    Accessible.role: Accessible.Paragraph
                    Accessible.name: qsTr("Download speed")
                    Accessible.description: qsTr("Download speed in bytes/kilobytes/megabytes per second")
                }

                ProgressBar {
                    id: itemProgressBar
                    objectName: "itemProgressBar"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: downloadButton.left
                    anchors.rightMargin: 10
                    width: 100
                    visible: downloading
                    Accessible.role: Accessible.ProgressBar
                    Accessible.name: qsTr("Download progressBar")
                    Accessible.description: qsTr("Shows the progress made in the download")
                }

                Label {
                    id: installedLabel
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 15
                    objectName: "installedLabel"
                    color: "#d1d5db"
                    text: qsTr("Already installed")
                    visible: modelData.installed
                    Accessible.role: Accessible.Paragraph
                    Accessible.name: text
                    Accessible.description: qsTr("Whether the file is already installed on your system")
                }

                Button {
                    id: downloadButton
                    text: downloading ? "Cancel" : "Download"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    visible: !modelData.installed
                    onClicked: {
                        if (!downloading) {
                            downloading = true;
                            Download.downloadModel(modelData.filename);
                        } else {
                            downloading = false;
                            Download.cancelDownload(modelData.filename);
                        }
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
}
