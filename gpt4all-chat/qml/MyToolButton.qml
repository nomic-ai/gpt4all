import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import Qt5Compat.GraphicalEffects

Button {
    id: myButton
    padding: 10
    property color backgroundColor: theme.iconBackgroundDark
    property color backgroundColorHovered: theme.iconBackgroundHovered
    property color toggledColor: theme.accentColor
    property real toggledWidth: 1
    property bool toggled: false
    property alias source: image.source
    property alias fillMode: image.fillMode
    property alias imageWidth: image.sourceSize.width
    property alias imageHeight: image.sourceSize.height
    property alias bgTransform: background.transform
    contentItem: Text {
        text: myButton.text
        horizontalAlignment: Text.AlignHCenter
        color: myButton.enabled ? theme.textColor : theme.mutedTextColor
        font.pixelSize: theme.fontSizeLarge
        Accessible.role: Accessible.Button
        Accessible.name: text
    }

    background: Item {
        id: background
        anchors.fill: parent
        Rectangle {
            anchors.fill: parent
            color: myButton.toggledColor
            visible: myButton.toggled
            border.color: myButton.toggledColor
            border.width: myButton.toggledWidth
            radius: 8
        }
        Image {
            id: image
            anchors.centerIn: parent
            visible: false
            fillMode: Image.PreserveAspectFit
            mipmap: true
            sourceSize.width: 32
            sourceSize.height: 32
        }
        ColorOverlay {
            anchors.fill: image
            source: image
            color: !myButton.enabled ? theme.mutedTextColor : myButton.hovered ? backgroundColorHovered : backgroundColor
        }
    }
    Accessible.role: Accessible.Button
    Accessible.name: text
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
}
