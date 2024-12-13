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
    signal modelsViewRequested()

    ToastManager {
        id: messageToast
    }

    PopupDialog {
        id: downloadingErrorPopup
        anchors.centerIn: parent
        shouldTimeOut: false
    }

    ColumnLayout {
        id: mainArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 30
        spacing: 10

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: 10

            MyButton {
                id: backButton
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                text: qsTr("\u2190 Existing Models")

                borderWidth: 0
                backgroundColor: theme.lighterButtonBackground
                backgroundColorHovered: theme.lighterButtonBackgroundHovered
                backgroundRadius: 5
                padding: 15
                topPadding: 8
                bottomPadding: 8
                textColor: theme.lighterButtonForeground
                fontPixelSize: theme.fontSizeLarge
                fontPixelBold: true

                onClicked: {
                    modelsViewRequested()
                }
            }

            Text {
                id: welcome
                text: qsTr("Explore Models")
                font.pixelSize: theme.fontSizeBanner
                color: theme.titleTextColor
            }
        }

        RowLayout {
            id: bar
            implicitWidth: 600
            spacing: 10
            MyTabButton {
                text: qsTr("GPT4All")
                isSelected: gpt4AllModelView.isShown()
                onPressed: {
                    gpt4AllModelView.show();
                }
            }
            MyTabButton {
                text: qsTr("HuggingFace")
                isSelected: huggingfaceModelView.isShown()
                onPressed: {
                    huggingfaceModelView.show();
                }
            }
        }

        StackLayout {
            id: stackLayout
            Layout.fillWidth: true
            Layout.fillHeight: true

            AddGPT4AllModelView {
                id: gpt4AllModelView
                Layout.fillWidth: true
                Layout.fillHeight: true

                function show() {
                    stackLayout.currentIndex = 0;
                }
                function isShown() {
                    return stackLayout.currentIndex === 0
                }
            }

            AddHFModelView {
                id: huggingfaceModelView
                Layout.fillWidth: true
                Layout.fillHeight: true
                // FIXME: This generates a warning and should not be used inside a layout, but without
                // it the text field inside this qml does not display at full width so it looks like
                // a bug in stacklayout
                anchors.fill: parent

                function show() {
                    stackLayout.currentIndex = 1;
                }
                function isShown() {
                    return stackLayout.currentIndex === 1
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
