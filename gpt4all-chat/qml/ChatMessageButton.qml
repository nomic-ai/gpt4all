import QtQuick
import QtQuick.Controls

import gpt4all

MyToolButton {
    property string name

    width: 24
    height: 24
    imageWidth: width
    imageHeight: height
    ToolTip {
        visible: parent.hovered
        y: parent.height * 1.5
        text: name
        delay: Qt.styleHints.mousePressAndHoldInterval
    }
    Accessible.name: name
}
