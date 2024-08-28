import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

TextArea {
    id: myTextArea
    color: enabled ? theme.textColor : theme.mutedTextColor
    placeholderTextColor: theme.mutedTextColor
    font.pixelSize: theme.fontSizeLarge
    background: Rectangle {
        implicitWidth: 150
        color: theme.controlBackground
        border.width: 1
        border.color: theme.controlBorder
        radius: 10
    }
    padding: 10
    wrapMode: TextArea.Wrap

    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
}