import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

Dialog {
    id: popupDialog
    anchors.centerIn: parent
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
        spacing: 20

        Label {
            id: textField
            width: Math.min(1024, implicitWidth)
            height: Math.min(600, implicitHeight)
            anchors.verticalCenter: shouldShowBusy ? busyIndicator.verticalCenter : parent.verticalCenter
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            textFormat: Text.StyledText
            wrapMode: Text.WordWrap
            color: theme.textColor
            linkColor: theme.linkColor
            Accessible.role: Accessible.HelpBalloon
            Accessible.name: text
            Accessible.description: qsTr("Reveals a shortlived help balloon")
            onLinkActivated: function(link) { Qt.openUrlExternally(link) }
        }

        MyBusyIndicator {
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
        color: theme.containerBackground
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