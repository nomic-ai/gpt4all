import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

TextField {
    id: myTextField
    padding: 10
    placeholderTextColor: theme.mutedTextColor
    background: Rectangle {
        implicitWidth: 150
        color: myTextField.enabled ? theme.controlBackground : theme.disabledControlBackground
        border.width: 1
        border.color: theme.controlBorder
        radius: 10
    }
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
    color: enabled ? theme.textColor : theme.mutedTextColor
}
