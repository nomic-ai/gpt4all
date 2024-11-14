import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

TextArea {
    id: myTextArea

    property bool hasError: false

    color: enabled ? theme.textColor : theme.mutedTextColor
    placeholderTextColor: theme.mutedTextColor
    font.pixelSize: theme.fontSizeLarge
    background: Rectangle {
        implicitWidth: 150
        color: theme.controlBackground
        border.width: myTextArea.hasError ? 2 : 1
        border.color: myTextArea.hasError ? theme.textErrorColor : theme.controlBorder
        radius: 10
    }
    padding: 10
    wrapMode: TextArea.Wrap

    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
}
