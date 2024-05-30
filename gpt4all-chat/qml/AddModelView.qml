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

Rectangle {
    id: addModelView

    Theme {
        id: theme
    }

    color: theme.viewBackground
    signal modelViewRequested()

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

            MyButton {
                id: backButton
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                text: qsTr("\u2190 Existing Models")

                borderWidth: 0
                backgroundColor: theme.collectionButtonBackground
                backgroundColorHovered: theme.collectionButtonBackgroundHovered
                backgroundRadius: 5
                padding: 15
                topPadding: 8
                bottomPadding: 8
                textColor: theme.green600
                fontPixelSize: theme.fontSizeLarge
                fontPixelBold: true

                background: Rectangle {
                    radius: backButton.backgroundRadius
                    color: backButton.toggled ? backButton.backgroundColorHovered : collectionsButton.backgroundColor
                }

                onClicked: {
                    modelViewRequested()
                }
            }
        }

        ColumnLayout {
            id: root
            Layout.alignment: Qt.AlignTop | Qt.AlignCenter
            spacing: 50

            /*
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
        */
        }
    }
}
