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
        anchors.leftMargin: 15
        sourceSize.width: 24
        sourceSize.height: 24
        mipmap: true
        visible: false
    }

    ColorOverlay {
        anchors.fill: myimage
        source: myimage
        color: fancyLink.hovered ? theme.fancyLinkTextHovered : theme.fancyLinkText
    }

    leftPadding: 50
    borderWidth: 0
    backgroundColor: "transparent"
    backgroundColorHovered: "transparent"
    fontPixelBold: true
    padding: 15
    topPadding: 8
    bottomPadding: 8
    textColor: fancyLink.hovered ? theme.fancyLinkTextHovered : theme.fancyLinkText
    fontPixelSize: theme.fontSizeSmall
    background: Rectangle {
        color: "transparent"
    }

    Accessible.name: qsTr("Fancy link")
    Accessible.description: qsTr("A stylized link")
}
