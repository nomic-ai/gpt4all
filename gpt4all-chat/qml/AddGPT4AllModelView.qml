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
    Layout.alignment: Qt.AlignTop
    spacing: 5

    Label {
        Layout.topMargin: 0
        Layout.bottomMargin: 25
        Layout.rightMargin: 150 * theme.fontScale
        Layout.alignment: Qt.AlignTop
        Layout.fillWidth: true
        verticalAlignment: Text.AlignTop
        text: qsTr("These models have been specifically configured for use in GPT4All. The first few models on the " +
                   "list are known to work the best, but you should only attempt to use models that will fit in your " +
                   "available memory.")
        font.pixelSize: theme.fontSizeLarger
        color: theme.textColor
        wrapMode: Text.WordWrap
    }

    Label {
        visible: !ModelList.gpt4AllDownloadableModels.count && !ModelList.asyncModelRequestOngoing
        Layout.fillWidth: true
        Layout.fillHeight: true
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        text: qsTr("Network error: could not retrieve %1").arg("http://gpt4all.io/models/models3.json")
        font.pixelSize: theme.fontSizeLarge
        color: theme.mutedTextColor
    }

    MyBusyIndicator {
        visible: !ModelList.gpt4AllDownloadableModels.count && ModelList.asyncModelRequestOngoing
        running: ModelList.asyncModelRequestOngoing
        Accessible.role: Accessible.Animation
        Layout.alignment: Qt.AlignCenter
        Accessible.name: qsTr("Busy indicator")
        Accessible.description: qsTr("Displayed when the models request is ongoing")
    }

    RowLayout {
        ButtonGroup {
            id: buttonGroup
            exclusive: true
        }
        MyButton {
            text: qsTr("All")
            checked: true
            borderWidth: 0
            backgroundColor: checked ? theme.lightButtonBackground : "transparent"
            backgroundColorHovered: theme.lighterButtonBackgroundHovered
            backgroundRadius: 5
            padding: 15
            topPadding: 8
            bottomPadding: 8
            textColor: theme.lighterButtonForeground
            fontPixelSize: theme.fontSizeLarge
            fontPixelBold: true
            checkable: true
            ButtonGroup.group: buttonGroup
            onClicked: {
                ModelList.gpt4AllDownloadableModels.filter("");
            }

        }
        MyButton {
            text: qsTr("Reasoning")
            borderWidth: 0
            backgroundColor: checked ? theme.lightButtonBackground : "transparent"
            backgroundColorHovered: theme.lighterButtonBackgroundHovered
            backgroundRadius: 5
            padding: 15
            topPadding: 8
            bottomPadding: 8
            textColor: theme.lighterButtonForeground
            fontPixelSize: theme.fontSizeLarge
            fontPixelBold: true
            checkable: true
            ButtonGroup.group: buttonGroup
            onClicked: {
                ModelList.gpt4AllDownloadableModels.filter("#reasoning");
            }
        }
        Layout.bottomMargin: 10
    }

    ScrollView {
        id: scrollView
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true

        ListView {
            id: modelListView
            model: ModelList.gpt4AllDownloadableModels
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
                                    text: parameters !== "" ? parameters : qsTr("?")
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
