import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import Qt5Compat.GraphicalEffects
import mysettings

MyButton {
    id: fancyLink
    property alias imageSource: myimage.source

    Image {
        id: myimage
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 12
        sourceSize: Qt.size(15, 15)
        mipmap: true
        visible: false
    }

    ColorOverlay {
        anchors.fill: myimage
        source: myimage
        color: fancyLink.hovered ? theme.fancyLinkTextHovered : theme.fancyLinkText
    }

    borderWidth: 0
    backgroundColor: "transparent"
    backgroundColorHovered: "transparent"
    fontPixelBold: true
    leftPadding: 35
    rightPadding: 8
    topPadding: 1
    bottomPadding: 1
    textColor: fancyLink.hovered ? theme.fancyLinkTextHovered : theme.fancyLinkText
    fontPixelSize: theme.fontSizeSmall
    background: Rectangle {
        color: "transparent"
    }

    Accessible.name: qsTr("Fancy link")
    Accessible.description: qsTr("A stylized link")
}
