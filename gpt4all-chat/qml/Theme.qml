import QtCore
import QtQuick
import QtQuick.Controls.Basic
import mysettings

QtObject {
    property color textColor: MySettings.chatTheme == "Dark" ? "#d1d5db" : "#2e2e34"
    property color textAccent: MySettings.chatTheme == "Dark" ? "#8e8ea0" : "#71717f"
    property color mutedTextColor: MySettings.chatTheme == "Dark" ? backgroundLightest : "#AFAFB5"
    property color backgroundDarkest: MySettings.chatTheme == "Dark" ? "#1c1f21" : "#e3e3e5"
    property color backgroundDarker: MySettings.chatTheme == "Dark" ? "#1e2123" : "#e0dedc"
    property color backgroundDark: MySettings.chatTheme == "Dark" ? "#222527" : "#D2D1D5"
    property color backgroundLight: MySettings.chatTheme == "Dark" ? "#343541" : "#FFFFFF"
    property color backgroundLighter: MySettings.chatTheme == "Dark" ? "#444654" : "#F7F7F8"
    property color backgroundLightest: MySettings.chatTheme == "Dark" ? "#7d7d8e" : "#82827a"
    property color backgroundAccent: MySettings.chatTheme == "Dark" ? "#40414f" : "#E3E3E6"
    property color buttonBorder: MySettings.chatTheme == "Dark" ? "#565869" : "#a9a9b0"
    property color dialogBorder: MySettings.chatTheme == "Dark" ? "#d1d5db" : "#2e2e34"
    property color userColor: MySettings.chatTheme == "Dark" ? "#ec86bf" : "#137382"
    property color linkColor: MySettings.chatTheme == "Dark" ? "#55aaff" : "#aa5500"
    property color tabBorder: MySettings.chatTheme == "Dark" ? backgroundLight : backgroundDark
    property color assistantColor: "#10a37f"
    property color textErrorColor: "red"
    property real  fontSizeLarge: MySettings.fontSize == "Small" ? Qt.application.font.pixelSize : MySettings.fontSize == "Medium" ? Qt.application.font.pixelSize + 5 : Qt.application.font.pixelSize + 10
    property real  fontSizeLarger: MySettings.fontSize == "Small" ? Qt.application.font.pixelSize + 2 : MySettings.fontSize == "Medium" ? Qt.application.font.pixelSize + 7 : Qt.application.font.pixelSize + 12
}

