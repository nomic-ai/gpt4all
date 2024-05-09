import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import llm
import chatlistmodel
import download
import modellist
import network
import gpt4all
import mysettings
import localdocs

Rectangle {
    id: localDocsView

    Theme {
        id: theme
    }

    color: theme.viewBackground
    signal chatViewRequested()
    signal localDocsViewRequested()
    signal settingsViewRequested(int page)
    signal downloadViewRequested(bool showEmbeddingModels)

    ColumnLayout {
        id: mainArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 30
        spacing: 50

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
                    text: qsTr("LocalDocs")
                    font.pixelSize: theme.fontSizeBanner
                    color: theme.titleTextColor
                }

                Text {
                    text: qsTr("Chat with your local files")
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
                text: qsTr("\uFF0B Add Doc Collection")
            }
        }

        ScrollView {
            id: scrollView
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ListView {
                id: collectionListView
                model: LocalDocs.localDocsModel
                boundsBehavior: Flickable.StopAtBounds
                spacing: 30

                delegate: Rectangle {
                    width: collectionListView.width
                    height: childrenRect.height + 60
                    color: theme.conversationBackground
                    radius: 10
                    border.width: 1
                    border.color: theme.controlBorder

                    property bool removing: false

                    ColumnLayout {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 30
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignLeft
                                text: collection
                                elide: Text.ElideRight
                                color: theme.titleTextColor
                                font.pixelSize: theme.fontSizeLargest
                                font.bold: true
                            }

                            Item {
                                Layout.alignment: Qt.AlignRight
                                Layout.preferredWidth: state.contentWidth + 50
                                Layout.preferredHeight: state.contentHeight + 10
                                ProgressBar {
                                    id: itemProgressBar
                                    anchors.fill: parent
                                    value: {
                                        if (model.error !== "")
                                            return 0

                                        if (model.indexing)
                                            return (model.totalBytesToIndex - model.currentBytesToIndex) / model.totalBytesToIndex

                                        if (model.currentEmbeddingsToIndex !== model.totalEmbeddingsToIndex)
                                            return (model.currentEmbeddingsToIndex / model.totalEmbeddingsToIndex)

                                        return 0
                                    }

                                    background: Rectangle {
                                        implicitHeight: 45
                                        color: {
                                            if (model.error !== "")
                                                return "transparent"

                                            if (model.indexing)
                                                 return "#fff9d2"//theme.progressBackground

                                            if (model.currentEmbeddingsToIndex !== model.totalEmbeddingsToIndex)
                                                 return "#fff9d2"//theme.progressBackground

                                            if (model.forceIndexing)
                                                return theme.red200

                                            return theme.green200
                                        }
                                        radius: 6
                                    }
                                    contentItem: Item {
                                        implicitHeight: 40

                                        Rectangle {
                                            width: itemProgressBar.visualPosition * parent.width
                                            height: parent.height
                                            radius: 2
                                            color: "#fcf0c9" //theme.progressForeground
                                        }
                                    }
                                    Accessible.role: Accessible.ProgressBar
                                    Accessible.name: qsTr("Indexing progressBar")
                                    Accessible.description: qsTr("Shows the progress made in the indexing")
                                    ToolTip.text: model.error
                                    ToolTip.visible: hovered && model.error !== ""
                                }
                                Label {
                                    id: state
                                    anchors.centerIn: itemProgressBar
                                    horizontalAlignment: Text.AlignHCenter
                                    color: {
                                        if (model.error !== "")
                                            return theme.textErrorColor

                                        if (model.indexing)
                                            return "#d16f0e"//theme.progressText

                                        if (model.currentEmbeddingsToIndex !== model.totalEmbeddingsToIndex)
                                            return "#d16f0e"//theme.progressText

                                        if (model.forceIndexing)
                                            return theme.textErrorColor

                                        return theme.green600
                                    }
                                    text: {
                                        if (model.error !== "")
                                            return qsTr("ERROR")

                                        // indicates extracting snippets from documents
                                        if (model.indexing)
                                            return qsTr("INDEXING")

                                        // indicates generating the embeddings for any outstanding snippets
                                        if (model.currentEmbeddingsToIndex !== model.totalEmbeddingsToIndex)
                                            return qsTr("EMBEDDING")

                                        if (model.forceIndexing)
                                            return qsTr("REQUIRES UPDATE")

                                        if (model.installed)
                                            return qsTr("READY")

                                        return qsTr("INSTALLING")
                                    }
                                    elide: Text.ElideRight
                                    font.bold: true
                                    font.pixelSize: theme.fontSizeSmaller
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignLeft
                                text: folder_path
                                elide: Text.ElideRight
                                color: theme.titleTextColor
                                font.pixelSize: theme.fontSizeSmall
                            }

                            Text {
                                Layout.alignment: Qt.AlignRight
                                text: {
                                    if (model.error !== "")
                                        return model.error

                                    if (model.indexing)
                                        return qsTr("Indexing in progress")

                                    if (model.currentEmbeddingsToIndex !== model.totalEmbeddingsToIndex)
                                        return qsTr("Embedding in progress")

                                    if (model.forceIndexing)
                                        return qsTr("This collection requires an update after version change")

                                    if (model.installed)
                                        return qsTr("Automatically reindexes upon changes to the folder")

                                    return qsTr("Installation in progress")
                                }
                                elide: Text.ElideRight
                                color: theme.logoColor
                                font.pixelSize: theme.fontSizeSmaller
                            }
                            Text {
                                visible: {
                                    return model.indexing || model.currentEmbeddingsToIndex !== model.totalEmbeddingsToIndex
                                }
                                Layout.alignment: Qt.AlignRight
                                text: {
                                    var percentComplete = Math.round(itemProgressBar.value * 100);
                                    var formattedPercent = percentComplete < 10 ? " " + percentComplete : percentComplete.toString();
                                    return formattedPercent + qsTr("%")
                                }
                                elide: Text.ElideRight
                                color: theme.logoColor
                                font.family: "monospace"
                                font.pixelSize: theme.fontSizeSmaller
                            }
                        }

                        RowLayout {
                            Text {
                                text: model.totalDocs + (model.totalDocs > 1 ? qsTr(" files – ") : qsTr(" file – ")) + model.totalWords + " words"
                                elide: Text.ElideRight
                                color: theme.green500
                                font.pixelSize: theme.fontSizeSmaller
                            }
                            Text {
                                text: model.lastUpdate
                                elide: Text.ElideRight
                                color: theme.gray500
                                font.pixelSize: theme.fontSizeSmaller
                            }
                            Text {
                                text: model.embeddingModel
                                elide: Text.ElideRight
                                color: theme.logoColor
                                font.bold: true
                                font.pixelSize: theme.fontSizeSmaller
                            }
                            Text {
                                Layout.leftMargin: 15
                                visible: model.currentEmbeddingsToIndex !== model.totalEmbeddingsToIndex
                                text: model.currentEmbeddingsToIndex + " of " + model.totalEmbeddingsToIndex + " embeddings"
                                elide: Text.ElideRight
                                color: theme.gray500
                                font.pixelSize: theme.fontSizeSmaller
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: theme.dividerColor
                        }

                        RowLayout {
                            id: fileProcessingRow
                            Layout.topMargin: 15
                            Layout.bottomMargin: 15
                            visible: model.fileCurrentlyProcessing !== "" && (model.indexing || model.currentEmbeddingsToIndex !== model.totalEmbeddingsToIndex)
                            MyBusyIndicator {
                                Layout.alignment: Qt.AlignCenter
                                Layout.preferredWidth: 12
                                Layout.preferredHeight: 12
                                running: true
                                size: 12
                                color: theme.textColor
                            }

                            Text {
                                id: filename
                                Layout.alignment: Qt.AlignCenter
                                text: model.fileCurrentlyProcessing
                                elide: Text.ElideRight
                                color: theme.textColor
                                font.bold: true
                                font.pixelSize: theme.fontSizeLarge
                            }
                        }

                        Rectangle {
                            visible: fileProcessingRow.visible
                            Layout.fillWidth: true
                            height: 1
                            color: theme.dividerColor
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 30
                            Layout.leftMargin: 15
                            Layout.topMargin: 15
                            Text {
                                text: qsTr("Local Folder")
                                elide: Text.ElideRight
                                color: theme.green600
                                font.bold: true
                                font.pixelSize: theme.fontSizeSmall
                            }
                            Text {
                                text: qsTr("Remove")
                                elide: Text.ElideRight
                                color: theme.red500
                                font.bold: true
                                font.pixelSize: theme.fontSizeSmall
                            }
                            Item {
                                Layout.fillWidth: true
                            }
                            Text {
                                Layout.alignment: Qt.AlignRight
                                visible: model.forceIndexing
                                text: qsTr("Update")
                                elide: Text.ElideRight
                                color: theme.red500
                                font.bold: true
                                font.pixelSize: theme.fontSizeSmall
                                TapHandler {
                                    onTapped: {
                                        LocalDocs.forceIndexing(collection)
                                    }
                                }
                            }
                        }
                    }

//                    Item {
//                        id: buttons
//                        anchors.right: parent.right
//                        anchors.verticalCenter: parent.verticalCenter
//                        anchors.margins: 20
//                        width: removeButton.width
//                        height:removeButton.height
//                        MySettingsButton {
//                            id: removeButton
//                            anchors.centerIn: parent
//                            text: qsTr("Remove")
//                            visible: !item.removing
//                            onClicked: {
//                                item.removing = true
//                                LocalDocs.removeFolder(collection, folder_path)
//                            }
//                        }
//                    }
                }
            }
        }
    }
}
