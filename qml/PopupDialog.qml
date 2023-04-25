import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

Dialog {
    id: popupDialog
    anchors.centerIn: parent
    opacity: 0.9
    padding: 20
    property alias text: textField.text
    property bool shouldTimeOut: true
    property bool shouldShowBusy: false
    modal: shouldShowBusy
    closePolicy: shouldShowBusy ? Popup.NoAutoClose : (Popup.CloseOnEscape | Popup.CloseOnPressOutside)

    Theme {
        id: theme
    }

    Row {
        anchors.centerIn: parent
        width: childrenRect.width
        height: childrenRect.height
        spacing: 20

        Text {
            id: textField
            anchors.verticalCenter: busyIndicator.verticalCenter
            horizontalAlignment: Text.AlignJustify
            color: theme.textColor
            Accessible.role: Accessible.HelpBalloon
            Accessible.name: text
            Accessible.description: qsTr("Reveals a shortlived help balloon")
        }

        BusyIndicator {
            id: busyIndicator
            visible: shouldShowBusy
            running: shouldShowBusy

            Accessible.role: Accessible.Animation
            Accessible.name: qsTr("Busy indicator")
            Accessible.description: qsTr("Displayed when the popup is showing busy")
        }
    }

    background: Rectangle {
        anchors.fill: parent
        color: theme.backgroundDarkest
        border.width: 1
        border.color: theme.dialogBorder
        radius: 10
    }

    exit: Transition {
        NumberAnimation { duration: 500; property: "opacity"; from: 1.0; to: 0.0 }
    }

    onOpened: {
        if (shouldTimeOut)
            timer.start()
    }

    Timer {
        id: timer
        interval: 500; running: false; repeat: false
        onTriggered: popupDialog.close()
    }
}