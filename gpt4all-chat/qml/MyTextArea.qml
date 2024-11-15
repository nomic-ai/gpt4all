import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

TextArea {
    id: myTextArea

    property string errState: "ok"  // one of "ok", "error", "warning"

    color: enabled ? theme.textColor : theme.mutedTextColor
    placeholderTextColor: theme.mutedTextColor
    font.pixelSize: theme.fontSizeLarge
    background: Rectangle {
        implicitWidth: 150
        color: theme.controlBackground
        border.width: errState === "ok" ? 1 : 2
        border.color: {
            switch (errState) {
                case "ok":      return theme.controlBorder;
                case "warning": return theme.textWarningColor;
                case "error":   return theme.textErrorColor;
            }
        }
        radius: 10
    }
    padding: 10
    wrapMode: TextArea.Wrap

    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
}
