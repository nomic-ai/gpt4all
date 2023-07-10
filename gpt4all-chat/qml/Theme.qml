import QtCore
import QtQuick
import QtQuick.Controls.Basic

QtObject {
    property color textColor: "#d1d5db"
    property color textAccent: "#8e8ea0"
    property color mutedTextColor: backgroundLightest
    property color textErrorColor: "red"
    property color backgroundDarkest: "#1c1f21"
    property color backgroundDarker: "#1e2123"
    property color backgroundDark: "#222527"
    property color backgroundLight: "#343541"
    property color backgroundLighter: "#444654"
    property color backgroundLightest: "#7d7d8e"
    property color backgroundAccent: "#40414f"
    property color buttonBorder: "#565869"
    property color dialogBorder: "#d1d5db"
    property color userColor: "#ec86bf"
    property color assistantColor: "#10a37f"
    property color linkColor: "#55aaff"
    property color tabBorder: backgroundLight
    property real  fontSizeLarge: Qt.application.font.pixelSize
    property real  fontSizeLarger: Qt.application.font.pixelSize + 2
}
