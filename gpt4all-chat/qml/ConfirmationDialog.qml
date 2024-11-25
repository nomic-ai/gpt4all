import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

MyDialog {
    id: confirmationDialog
    anchors.centerIn: parent
    modal: true
    padding: 20
    property alias dialogTitle: titleText.text
    property alias description: descriptionText.text

    Theme { id: theme }

    contentItem: ColumnLayout {
        Text {
            id: titleText
            Layout.alignment: Qt.AlignHCenter
            textFormat: Text.StyledText
            color: theme.textColor
            font.pixelSize: theme.fontSizeLarger
            font.bold: true
        }

        Text {
            id: descriptionText
            Layout.alignment: Qt.AlignHCenter
            textFormat: Text.StyledText
            color: theme.textColor
            font.pixelSize: theme.fontSizeMedium
        }
    }

    footer: DialogButtonBox {
        id: dialogBox
        padding: 20
        alignment: Qt.AlignRight
        spacing: 10
        MySettingsButton {
            text: qsTr("OK")
            textColor: theme.mediumButtonText
            backgroundColor: theme.mediumButtonBackground
            backgroundColorHovered: theme.mediumButtonBackgroundHovered
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        MySettingsButton {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        background: Rectangle {
            color: "transparent"
        }
        Keys.onEnterPressed: confirmationDialog.accept()
        Keys.onReturnPressed: confirmationDialog.accept()
    }
    Component.onCompleted: dialogBox.forceActiveFocus()
}
