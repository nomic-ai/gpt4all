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

    Label {
        id: mainTextLabel
        color: theme.settingsTitleTextColor
        font.pixelSize: theme.fontSizeLarger
        font.bold: true
        onLinkActivated: function(link) {
            root.linkActivated(link);
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
