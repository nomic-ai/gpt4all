import QtCore
import QtQuick
import QtQuick.Controls.Basic

QtObject {
    property color textColor: "#d1d5db"
    property color mutedTextColor: backgroundLightest
    property color textErrorColor: "red"
    property color backgroundDarkest: "#202123"
    property color backgroundDarker: "#222326"
    property color backgroundDark: "#242528"
    property color backgroundLight: "#343541"
    property color backgroundLighter: "#444654"
    property color backgroundLightest: "#7d7d8e"
    property color buttonBorder: "#565869"
    property color dialogBorder: "#d1d5db"
    property color userColor: "#ec86bf"
    property color assistantColor: "#10a37f"
    property color linkColor: "white"
    property color tabBorder: "#2C2D35"
    property real  fontSizeLarge: Qt.application.font.pixelSize
    property real  fontSizeLarger: Qt.application.font.pixelSize + 2
}
