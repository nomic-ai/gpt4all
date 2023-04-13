import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: window
    width: 400
    height: 600
    visible: true
    title: "GPT4ALL_GUI"
    property bool isResponding: false

    function sendMessage() {
        if (messageInput.text) {
            messages.add_message(messageInput.text, true)
            backend.runWorker(messageInput.text)
            messageInput.clear()
            window.isResponding = true
        }
    }

    ColumnLayout {
        anchors.fill: parent

        ListView {
            id: messages
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: ListModel {}

            delegate: Rectangle {
                width: parent.width
                height: textLabel.implicitHeight + 10
                color: model.isOwnMessage ? "#e0f0ff" : "#e0ffe0"

                Text {
                    id: textLabel
                    anchors.right: model.isOwnMessage ? parent.right : undefined
                    anchors.left: !model.isOwnMessage ? parent.left : undefined
                    anchors.margins: 5
                    text: model.text
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    width: parent.width - anchors.margins * 2
                }
            }

            function add_message(text, isOwnMessage) {
                model.append({"text": text, "isOwnMessage": isOwnMessage})
                positionViewAtEnd()
            }
        }

        RowLayout {
            Layout.fillWidth: true

            TextField {
                id: messageInput
                Layout.fillWidth: true
                placeholderText: qsTr("Enter message")
                onAccepted: sendMessage()

                Rectangle {
                    anchors.fill: parent
                    z: -1
                    gradient: Gradient {
                        GradientStop {
                            position: 0.0 + Math.sin(timeLine.time / 1000) * 0.5
                            color: "#e0f0ff"
                        }
                        GradientStop {
                            position: 0.2 + Math.sin(timeLine.time / 1000) * 0.5
                            color: "#e0ffe0"
                        }
                        GradientStop {
                            position: 1.0 + Math.sin(timeLine.time / 1000) * 0.5
                            color: "#e0f0ff"
                        }
                    }
                    visible: true;
                }

                Timer {
                    id: timeLine
                    interval: 20; running:true; repeat:true;
                    onTriggered:messageInput.update();
                }
            }

            Button {
                id: sendButton
                text: qsTr("Send")
                onClicked: sendMessage()
            }
        }
    }

    Connections {
        target: backend

        function onWorkerResultChanged() {
            messages.add_message(backend.workerResult, false)
            window.isResponding = false
        }
    }
}