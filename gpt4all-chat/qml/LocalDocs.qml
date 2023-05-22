import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

GridLayout {
    columns: 2
    rowSpacing: 10
    columnSpacing: 10

    Label {
        text: qsTr("Collections:")
        color: theme.textColor
        Layout.row: 1
        Layout.column: 0
    }

    RowLayout {
        spacing: 10
        Layout.row: 1
        Layout.column: 1
        MyComboBox {
            id: comboBox
            Layout.minimumWidth: 350
        }
        MyButton {
            text: "Add"
        }
        MyButton {
            text: "Remove"
        }
        MyButton {
            text: "Rename"
        }
    }
}
