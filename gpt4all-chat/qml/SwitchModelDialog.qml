import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import llm
import mysettings

MyDialog {
    id: switchModelDialog
    anchors.centerIn: parent
    modal: true
    padding: 20
    property int index: -1

    Theme {
        id: theme
    }

    contentItem: Text {
        textFormat: Text.StyledText
        text: qsTr("<b>Warning:</b> changing the model will erase the current conversation. Do you wish to continue?")
        color: theme.textColor
        font.pixelSize: theme.fontSizeLarge
    }

    footer: DialogButtonBox {
        id: dialogBox
        padding: 20
        alignment: Qt.AlignRight
        spacing: 10
        MySettingsButton {
            text: qsTr("Continue")
            Accessible.description: qsTr("Continue with model loading")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        MySettingsButton {
            text: qsTr("Cancel")
            Accessible.description: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        background: Rectangle {
            color: "transparent"
        }
    }
}
