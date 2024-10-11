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
    id: modelsView
    color: theme.viewBackground

    signal addModelViewRequested()

    ToastManager {
        id: messageToast
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 30

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: ModelList.installedModels.count === 0
            ColumnLayout {
                id: noInstalledLabel
                anchors.centerIn: parent
                spacing: 0

                Text {
                    Layout.alignment: Qt.AlignCenter
                    text: qsTr("No Models Installed")
                    color: theme.mutedLightTextColor
                    font.pixelSize: theme.fontSizeBannerSmall
                }

                Text {
                    Layout.topMargin: 15
                    horizontalAlignment: Qt.AlignHCenter
                    color: theme.mutedLighterTextColor
                    text: qsTr("Install a model to get started using GPT4All")
                    font.pixelSize: theme.fontSizeLarge
                }
            }

            MyButton {
                anchors.top: noInstalledLabel.bottom
                anchors.topMargin: 50
                anchors.horizontalCenter: noInstalledLabel.horizontalCenter
                rightPadding: 60
                leftPadding: 60
                text: qsTr("\uFF0B Add Model")
                onClicked: {
                    addModelViewRequested()
                }
                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Shows the add model view")
            }
        }

        RowLayout {
            visible: ModelList.installedModels.count !== 0
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
                    text: qsTr("Locally installed chat models")
                    font.pixelSize: theme.fontSizeLarge
                    color: theme.titleInfoTextColor
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

        ScrollView {
            id: scrollView
            visible: ModelList.installedModels.count !== 0
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: modelListView
                model: ModelList.installedModels
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
                                        text: isDownloading ? qsTr("Cancel") : qsTr("Resume")
                                        font.pixelSize: theme.fontSizeLarge
                                        Layout.topMargin: 20
                                        Layout.leftMargin: 20
                                        Layout.minimumWidth: 200
                                        Layout.fillWidth: true
                                        Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                                        visible: (isDownloading || isIncomplete) && downloadError === "" && !isOnline && !calcHash
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
                                        text: qsTr("RAM required")
                                        font.pixelSize: theme.fontSizeSmall
                                        color: theme.mutedDarkTextColor
                                    }
                                    Text {
                                        text: ramrequired >= 0 ? qsTr("%1 GB").arg(ramrequired) : qsTr("?")
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
                                        text: qsTr("Parameters")
                                        font.pixelSize: theme.fontSizeSmall
                                        color: theme.mutedDarkTextColor
                                    }
                                    Text {
                                        text: parameters !== "" ? parameters : "?"
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

    Connections {
        target: Download
        function onToastMessage(message) {
            messageToast.show(message);
        }
    }
}
