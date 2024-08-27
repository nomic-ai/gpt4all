import QtQuick
import QtQuick.Controls

Text {
    id: text

    signal click()
    property string tooltip

    HoverHandler { id: hoverHandler }
    TapHandler { onTapped: { click() } }

    font.bold: true
    font.underline: hoverHandler.hovered
    font.pixelSize: theme.fontSizeSmall
    ToolTip.text: tooltip
    ToolTip.visible: tooltip !== "" && hoverHandler.hovered
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
}
