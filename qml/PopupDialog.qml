import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: copyMessage
    anchors.centerIn: parent
    modal: false
    opacity: 0.9
    padding: 20
    property alias text: textField.text
    Text {
        id: textField
        horizontalAlignment: Text.AlignJustify
        color: "#d1d5db"
        Accessible.role: Accessible.HelpBalloon
        Accessible.name: text
        Accessible.description: qsTr("Reveals a shortlived help balloon")
    }
    background: Rectangle {
        anchors.fill: parent
        color: "#202123"
        border.width: 1
        border.color: "white"
        radius: 10
    }

    exit: Transition {
        NumberAnimation { duration: 500; property: "opacity"; from: 1.0; to: 0.0 }
    }
}