import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

CheckBox {
    id: myCheckBox

    background: Rectangle {
        color: "transparent"
    }

    indicator: Rectangle {
        implicitWidth: 26
        implicitHeight: 26
        x: myCheckBox.leftPadding
        y: parent.height / 2 - height / 2
        border.color: theme.checkboxBorder
        color: "transparent"
        radius: 3

        Rectangle {
            width: 14
            height: 14
            x: 6
            y: 6
            radius: 2
            color: theme.checkboxForeground
            visible: myCheckBox.checked
        }
    }

    contentItem: Text {
        text: myCheckBox.text
        font: myCheckBox.font
        opacity: enabled ? 1.0 : 0.3
        color: theme.textColor
        verticalAlignment: Text.AlignVCenter
        leftPadding: myCheckBox.indicator.width + myCheckBox.spacing
    }
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
}