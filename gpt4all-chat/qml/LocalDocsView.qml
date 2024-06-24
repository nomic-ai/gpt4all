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
    signal addCollectionViewRequested()

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
            visible: LocalDocs.databaseValid && LocalDocs.localDocsModel.count !== 0
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
                    color: theme.titleInfoTextColor
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 0
            }

            MyButton {
                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                text: qsTr("\uFF0B Add Doc Collection")
                onClicked: {
                    addCollectionViewRequested()
                }
            }
        }

        Rectangle {
            id: warning
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !LocalDocs.databaseValid
            Text {
                anchors.centerIn: parent
                horizontalAlignment: Qt.AlignHCenter
                text: qsTr("ERROR: The LocalDocs database is not valid.")
                color: theme.textErrorColor
                font.bold: true
                font.pixelSize: theme.fontSizeLargest
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: LocalDocs.databaseValid && LocalDocs.localDocsModel.count === 0
            ColumnLayout {
                id: noInstalledLabel
                anchors.centerIn: parent
                spacing: 0

                Text {
                    Layout.alignment: Qt.AlignCenter
                    text: qsTr("No Collections Installed")
                    color: theme.mutedLightTextColor
                    font.pixelSize: theme.fontSizeBannerSmall
                }

                Text {
                    Layout.topMargin: 15
                    horizontalAlignment: Qt.AlignHCenter
                    color: theme.mutedLighterTextColor
                    text: qsTr("Install a collection of local documents to get started using this feature")
                    font.pixelSize: theme.fontSizeLarge
                }
            }

            MyButton {
                anchors.top: noInstalledLabel.bottom
                anchors.topMargin: 50
                anchors.horizontalCenter: noInstalledLabel.horizontalCenter
                rightPadding: 60
                leftPadding: 60
                text: qsTr("\uFF0B Add Doc Collection")
                onClicked: {
                    addCollectionViewRequested()
                }
                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Shows the add model view")
            }
        }

        ScrollView {
            id: scrollView
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: LocalDocs.databaseValid && LocalDocs.localDocsModel.count !== 0

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

                                        if (model.currentEmbeddingsToIndex !== 0)
                                            return (model.totalEmbeddingsToIndex - model.currentEmbeddingsToIndex) / model.totalEmbeddingsToIndex

                                        return 0
                                    }

                                    background: Rectangle {
                                        implicitHeight: 45
                                        color: {
                                            if (model.error !== "")
                                                return "transparent"

                                            if (model.indexing)
                                                 return theme.altProgressBackground

                                            if (model.currentEmbeddingsToIndex !== 0)
                                                 return theme.altProgressBackground

                                            if (model.forceIndexing)
                                                return theme.red200

                                            return theme.lightButtonBackground
                                        }
                                        radius: 6
                                    }
                                    contentItem: Item {
                                        implicitHeight: 40

                                        Rectangle {
                                            width: itemProgressBar.visualPosition * parent.width
                                            height: parent.height
                                            radius: 2
                                            color: theme.altProgressForeground
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
                                            return theme.altProgressText

                                        if (model.currentEmbeddingsToIndex !== 0)
                                            return theme.altProgressText

                                        if (model.forceIndexing)
                                            return theme.textErrorColor

                                        return theme.lighterButtonForeground
                                    }
                                    text: {
                                        if (model.error !== "")
                                            return qsTr("ERROR")

                                        // indicates extracting snippets from documents
                                        if (model.indexing)
                                            return qsTr("INDEXING")

                                        // indicates generating the embeddings for any outstanding snippets
                                        if (model.currentEmbeddingsToIndex !== 0)
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
                                color: theme.titleTextColor2
                                font.pixelSize: theme.fontSizeSmall
                            }

                            Text {
                                Layout.alignment: Qt.AlignRight
                                text: {
                                    if (model.error !== "")
                                        return model.error

                                    if (model.indexing)
                                        return qsTr("Indexing in progress")

                                    if (model.currentEmbeddingsToIndex !== 0)
                                        return qsTr("Embedding in progress")

                                    if (model.forceIndexing)
                                        return qsTr("This collection requires an update after version change")

                                    if (model.installed)
                                        return qsTr("Automatically reindexes upon changes to the folder")

                                    return qsTr("Installation in progress")
                                }
                                elide: Text.ElideRight
                                color: theme.mutedDarkTextColor
                                font.pixelSize: theme.fontSizeSmaller
                            }
                            Text {
                                visible: {
                                    return model.indexing || model.currentEmbeddingsToIndex !== 0
                                }
                                Layout.alignment: Qt.AlignRight
                                text: {
                                    var percentComplete = Math.round(itemProgressBar.value * 100);
                                    var formattedPercent = percentComplete < 10 ? " " + percentComplete : percentComplete.toString();
                                    return formattedPercent + qsTr("%")
                                }
                                elide: Text.ElideRight
                                color: theme.mutedDarkTextColor
                                font.family: "monospace"
                                font.pixelSize: theme.fontSizeSmaller
                            }
                        }

                        RowLayout {
                            spacing: 7
                            Text {
                                text: "%1 – %2".arg(qsTr("%n file(s)", "", model.totalDocs)).arg(qsTr("%n word(s)", "", model.totalWords))
                                elide: Text.ElideRight
                                color: theme.styledTextColor2
                                font.pixelSize: theme.fontSizeSmaller
                            }
                            Text {
                                text: model.embeddingModel
                                elide: Text.ElideRight
                                color: theme.mutedDarkTextColor
                                font.bold: true
                                font.pixelSize: theme.fontSizeSmaller
                            }
                            Text {
                                visible: Qt.formatDateTime(model.lastUpdate) !== ""
                                text: Qt.formatDateTime(model.lastUpdate)
                                elide: Text.ElideRight
                                color: theme.mutedTextColor
                                font.pixelSize: theme.fontSizeSmaller
                            }
                            Text {
                                visible: model.currentEmbeddingsToIndex !== 0
                                text: (model.totalEmbeddingsToIndex - model.currentEmbeddingsToIndex) + " of "
                                      + model.totalEmbeddingsToIndex + " embeddings"
                                elide: Text.ElideRight
                                color: theme.mutedTextColor
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
                            visible: model.fileCurrentlyProcessing !== "" && (model.indexing || model.currentEmbeddingsToIndex !== 0)
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
                                text: qsTr("Remove")
                                elide: Text.ElideRight
                                color: theme.red500
                                font.bold: true
                                font.pixelSize: theme.fontSizeSmall
                                TapHandler {
                                    onTapped: {
                                        LocalDocs.removeFolder(collection, folder_path)
                                    }
                                }
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
                }
            }
        }
    }
}
