import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import chatlistmodel
import localdocs
import llm

Rectangle {
    id: collectionsDrawer

    color: "transparent"

    signal addDocsClicked
    property var currentChat: ChatListModel.currentChat

    Rectangle {
        id: borderLeft
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 1
        color: theme.dividerColor
    }

    ScrollView {
        id: scrollView
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: borderLeft.right
        anchors.right: parent.right
        anchors.margins: 2
        anchors.bottomMargin: 10
        clip: true
        contentHeight: 300
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ListView {
            id: listView
            model: LocalDocs.localDocsModel
            anchors.fill: parent
            anchors.margins: 13
            anchors.bottomMargin: 5
            boundsBehavior: Flickable.StopAtBounds
            spacing: 15

            delegate: Rectangle {
                width: listView.width
                height: childrenRect.height + 15
                color: checkBox.checked ? theme.collectionsButtonBackground : "transparent"

                RowLayout {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 7.5
                    MyCheckBox {
                        id: checkBox
                        Layout.alignment: Qt.AlignLeft
                        checked: currentChat.hasCollection(collection)
                        onClicked: {
                            if (checkBox.checked) {
                                currentChat.addCollection(collection)
                            } else {
                                currentChat.removeCollection(collection)
                            }
                        }
                        ToolTip.text: qsTr("Warning: searching collections while indexing can return incomplete results")
                        ToolTip.visible: hovered && model.indexing
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignLeft
                        Text {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignLeft
                            text: collection
                            font.pixelSize: theme.fontSizeLarger
                            elide: Text.ElideRight
                            color: theme.textColor
                        }
                        Text {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignLeft
                            text: "%1 – %2".arg(qsTr("%n file(s)", "", model.totalDocs)).arg(qsTr("%n word(s)", "", model.totalWords))
                            elide: Text.ElideRight
                            color: theme.mutedTextColor
                            font.pixelSize: theme.fontSizeSmall
                        }
                        RowLayout {
                            visible: model.updating
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignLeft
                            MyBusyIndicator {
                                color: theme.accentColor
                                size: 24
                                Layout.minimumWidth: 24
                                Layout.minimumHeight: 24
                            }
                            Text {
                                text: qsTr("Updating")
                                elide: Text.ElideRight
                                color: theme.accentColor
                                font.pixelSize: theme.fontSizeSmall
                                font.bold: true
                            }
                        }
                    }
                }
            }

            footer: ColumnLayout {
                width: listView.width
                spacing: 30
                Rectangle {
                    visible: listView.count !== 0
                    Layout.topMargin: 30
                    Layout.fillWidth: true
                    height: 1
                    color: theme.dividerColor
                }
                MySettingsButton {
                    id: collectionSettings
                    enabled: LocalDocs.databaseValid
                    Layout.alignment: Qt.AlignCenter
                    text: qsTr("\uFF0B Add Docs")
                    font.pixelSize: theme.fontSizeLarger
                    onClicked: {
                        addDocsClicked()
                    }
                }
                Text {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignLeft
                    text: qsTr("Select a collection to make it available to the chat model.")
                    font.pixelSize: theme.fontSizeLarger
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                    color: theme.mutedTextColor
                }
            }
        }
    }
}
