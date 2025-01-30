import Qt5Compat.GraphicalEffects
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

import gpt4all
import mysettings
import toolenums

ColumnLayout {
    property alias textContent: innerTextItem.textContent
    property bool isCurrent: false
    property bool isError: false
    property bool isThinking: false
    property int  thinkingTime: 0

    Layout.topMargin: 10
    Layout.bottomMargin: 10

    Item {
        Layout.preferredWidth: childrenRect.width
        Layout.preferredHeight: 38
        RowLayout {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            Item {
                Layout.preferredWidth: myTextArea.implicitWidth
                Layout.preferredHeight: myTextArea.implicitHeight
                TextArea {
                    id: myTextArea
                    text: {
                        if (isError)
                            return qsTr("Analysis encountered error");
                        if (isCurrent)
                            return isThinking ? qsTr("Thinking") : qsTr("Analyzing");
                        return isThinking
                            ? qsTr("Thought for %1 %2")
                                  .arg(Math.ceil(thinkingTime / 1000.0))
                                  .arg(Math.ceil(thinkingTime / 1000.0) === 1 ? qsTr("second") : qsTr("seconds"))
                            : qsTr("Analyzed");
                    }
                    padding: 0
                    font.pixelSize: theme.fontSizeLarger
                    enabled: false
                    focus: false
                    readOnly: true
                    color: headerMA.containsMouse ? theme.mutedDarkTextColorHovered : theme.mutedTextColor
                    hoverEnabled: false
                }

                Item {
                    id: textColorOverlay
                    anchors.fill: parent
                    clip: true
                    visible: false
                    Rectangle {
                        id: animationRec
                        width: myTextArea.width * 0.3
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        color: theme.textColor

                        SequentialAnimation {
                            running: isCurrent
                            loops: Animation.Infinite
                            NumberAnimation {
                                target: animationRec;
                                property: "x";
                                from: -animationRec.width;
                                to: myTextArea.width * 3;
                                duration: 2000
                            }
                        }
                    }
                }
                OpacityMask {
                    visible: isCurrent
                    anchors.fill: parent
                    maskSource: myTextArea
                    source: textColorOverlay
                }
            }

            Item {
                id: caret
                Layout.preferredWidth: contentCaret.width
                Layout.preferredHeight: contentCaret.height
                Image {
                    id: contentCaret
                    anchors.centerIn: parent
                    visible: false
                    sourceSize.width: theme.fontSizeLarge
                    sourceSize.height: theme.fontSizeLarge
                    mipmap: true
                    source: {
                        if (contentLayout.state === "collapsed")
                            return "qrc:/gpt4all/icons/caret_right.svg";
                        else
                            return "qrc:/gpt4all/icons/caret_down.svg";
                    }
                }

                ColorOverlay {
                    anchors.fill: contentCaret
                    source: contentCaret
                    color: headerMA.containsMouse ? theme.mutedDarkTextColorHovered : theme.mutedTextColor
                }
            }
        }

        MouseArea {
            id: headerMA
            hoverEnabled: true
            anchors.fill: parent
            onClicked: {
                if (contentLayout.state === "collapsed")
                    contentLayout.state = "expanded";
                else
                    contentLayout.state = "collapsed";
            }
        }
    }

    ColumnLayout {
        id: contentLayout
        spacing: 0
        state: "collapsed"
        clip: true

        states: [
            State {
                name: "expanded"
                PropertyChanges { target: contentLayout; Layout.preferredHeight: innerContentLayout.height }
            },
            State {
                name: "collapsed"
                PropertyChanges { target: contentLayout; Layout.preferredHeight: 0 }
            }
        ]

        transitions: [
            Transition {
                SequentialAnimation {
                    PropertyAnimation {
                        target: contentLayout
                        property: "Layout.preferredHeight"
                        duration: 300
                        easing.type: Easing.InOutQuad
                    }
                }
            }
        ]

        ColumnLayout {
            id: innerContentLayout
            Layout.leftMargin: 30
            ChatTextItem {
                id: innerTextItem
            }
        }
    }
}
