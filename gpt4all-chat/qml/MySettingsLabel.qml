import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: root
    property alias text: mainTextLabel.text
    property alias helpText: helpTextLabel.text

    property alias textFormat: mainTextLabel.textFormat
    property alias wrapMode: mainTextLabel.wrapMode
    property alias font: mainTextLabel.font
    property alias horizontalAlignment: mainTextLabel.horizontalAlignment
    signal linkActivated(link : url);
    property alias color: mainTextLabel.color
    property alias linkColor: mainTextLabel.linkColor

    property var onReset: null
    property alias canReset: resetButton.enabled
    property bool resetClears: false

    Item {
        anchors.margins: 5
        width: childrenRect.width
        height: mainTextLabel.contentHeight

        Label {
            id: mainTextLabel
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            color: theme.settingsTitleTextColor
            font.pixelSize: theme.fontSizeLarger
            font.bold: true
            verticalAlignment: Text.AlignVCenter
            onLinkActivated: function(link) {
                root.linkActivated(link);
            }
        }

        MySettingsButton {
            id: resetButton
            anchors.baseline: mainTextLabel.baseline
            anchors.left: mainTextLabel.right
            height: mainTextLabel.contentHeight
            anchors.leftMargin: 10
            padding: 2
            leftPadding: 10
            rightPadding: 10
            backgroundRadius: 5
            text: resetClears ? qsTr("Clear") : qsTr("Reset")
            visible: root.onReset !== null
            onClicked: root.onReset()
        }
    }
    Label {
        id: helpTextLabel
        visible: text !== ""
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: theme.settingsTitleTextColor
        font.pixelSize: theme.fontSizeLarge
        font.bold: false

        onLinkActivated: function(link) {
            root.linkActivated(link);
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton // pass clicks to parent
            cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
    }
}
