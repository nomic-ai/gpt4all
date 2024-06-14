import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    property alias text: mainTextLabel.text
    property alias helpText: helpTextLabel.text

    property alias textFormat: mainTextLabel.textFormat
    property alias wrapMode: mainTextLabel.wrapMode
    property alias font: mainTextLabel.font
    property alias horizontalAlignment: mainTextLabel.horizontalAlignment
//    property alias linkActivated: mainTextLabel.linkActivated
    property alias color: mainTextLabel.color
    property alias linkColor: mainTextLabel.linkColor

    Label {
        id: mainTextLabel
        color: theme.settingsTitleTextColor
        font.pixelSize: theme.fontSizeSmall
        font.bold: true
    }
    Label {
        id: helpTextLabel
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: theme.settingsTitleTextColor
        text: mainTextLabel.text
        font.pixelSize: theme.fontSizeSmaller
        font.bold: false
    }
}
