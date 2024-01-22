import QtCore
import QtQuick
import QtQuick.Controls.Basic
import mysettings

QtObject {

    // black and white
    property color black: Qt.hsla(231/360, 0.15, 0.19)
    property color white: Qt.hsla(0, 0, 1)

    // dark mode black and white
    property color darkwhite: Qt.hsla(0, 0, 0.85)

    // gray
    property color gray0: white
    property color gray50: Qt.hsla(25/360, 0.05, 0.97)
    property color gray100: Qt.hsla(25/360,0.05, 0.95)
    property color gray200: Qt.hsla(25/360, 0.05, 0.89)
    property color gray300: Qt.hsla(25/360, 0.05, 0.82)
    property color gray400: Qt.hsla(25/360, 0.05, 0.71)
    property color gray500: Qt.hsla(25/360, 0.05, 0.60)
    property color gray600: Qt.hsla(25/360, 0.05, 0.51)
    property color gray700: Qt.hsla(25/360, 0.05, 0.42)
    property color gray800: Qt.hsla(25/360, 0.05, 0.35)
    property color gray900: Qt.hsla(25/360, 0.05, 0.31)
    property color gray950: Qt.hsla(25/360, 0.05, 0.15)

    // darkmode
    property color darkgray0: Qt.hsla(25/360, 0.05, 0.23)
    property color darkgray50: Qt.hsla(25/360, 0.05, 0.21)
    property color darkgray100: Qt.hsla(25/360, 0.05, 0.19)
    property color darkgray200: Qt.hsla(25/360, 0.05, 0.17)
    property color darkgray300: Qt.hsla(25/360, 0.05, 0.15)
    property color darkgray400: Qt.hsla(25/360, 0.05, 0.13)
    property color darkgray500: Qt.hsla(25/360, 0.05, 0.11)
    property color darkgray600: Qt.hsla(25/360, 0.05, 0.09)
    property color darkgray700: Qt.hsla(25/360, 0.05, 0.07)
    property color darkgray800: Qt.hsla(25/360, 0.05, 0.05)
    property color darkgray900: Qt.hsla(25/360, 0.05, 0.03)
    property color darkgray950: Qt.hsla(25/360, 0.05, 0.01)

    // green
    property color green50: Qt.hsla(120/360, 0.18, 0.97)
    property color green100: Qt.hsla(120/360, 0.21, 0.93)
    property color green200: Qt.hsla(124/360, 0.21, 0.85)
    property color green300: Qt.hsla(122/360, 0.20, 0.73)
    property color green400: Qt.hsla(122/360, 0.19, 0.58)
    property color green500: Qt.hsla(121/360, 0.19, 0.45)
    property color green600: Qt.hsla(122/360, 0.20, 0.33)
    property color green700: Qt.hsla(122/360, 0.19, 0.29)
    property color green800: Qt.hsla(123/360, 0.17, 0.24)
    property color green900: Qt.hsla(124/360, 0.17, 0.20)
    property color green950: Qt.hsla(125/360, 0.22, 0.10)

    // yellow
    property color yellow50: Qt.hsla(47/360, 0.90, 0.96)
    property color yellow100: Qt.hsla(46/360, 0.89, 0.89)
    property color yellow200: Qt.hsla(45/360, 0.90, 0.77)
    property color yellow300: Qt.hsla(44/360, 0.90, 0.66)
    property color yellow400: Qt.hsla(41/360, 0.89, 0.56)
    property color yellow500: Qt.hsla(36/360, 0.85, 0.50)
    property color yellow600: Qt.hsla(30/360, 0.87, 0.44)
    property color yellow700: Qt.hsla(24/360, 0.84, 0.37)
    property color yellow800: Qt.hsla(21/360, 0.76, 0.31)
    property color yellow900: Qt.hsla(20/360, 0.72, 0.26)
    property color yellow950: Qt.hsla(19/360, 0.86, 0.14)

    // red
    property color red50: Qt.hsla(0, 0.71, 0.97)
    property color red100: Qt.hsla(0, 0.87, 0.94)
    property color red200: Qt.hsla(0, 0.89, 0.89)
    property color red300: Qt.hsla(0, 0.85, 0.77)
    property color red400: Qt.hsla(0, 0.83, 0.71)
    property color red500: Qt.hsla(0, 0.76, 0.60)
    property color red600: Qt.hsla(0, 0.65, 0.51)
    property color red700: Qt.hsla(0, 0.67, 0.42)
    property color red800: Qt.hsla(0, 0.63, 0.35)
    property color red900: Qt.hsla(0, 0.56, 0.31)
    property color red950: Qt.hsla(0, 0.67, 0.15)

    // purple
    property color purple400: Qt.hsla(279/360, 1.0, 0.73)
    property color purple500: Qt.hsla(279/360, 1.0, 0.63)
    property color purple600: Qt.hsla(279/360, 1.0, 0.53)

    property color yellowAccent: MySettings.chatTheme === "Dark" ? yellow300 : yellow300;
    property color orangeAccent: MySettings.chatTheme === "Dark" ? yellow500 : yellow500;

    property color darkContrast: MySettings.chatTheme === "Dark" ? darkgray100 : gray100
    property color lightContrast: MySettings.chatTheme === "Dark" ? darkgray0 : gray0

    property color controlBorder: MySettings.chatTheme === "Dark" ? darkgray0 : gray300
    property color controlBackground: MySettings.chatTheme === "Dark" ? darkgray100 : gray100
    property color disabledControlBackground: MySettings.chatTheme === "Dark" ? darkgray200 : gray200
    property color containerForeground: MySettings.chatTheme === "Dark" ? darkgray300 : gray300
    property color containerBackground: MySettings.chatTheme === "Dark" ? darkgray200 : gray200

    property color progressForeground: yellowAccent
    property color progressBackground: MySettings.chatTheme === "Dark" ? green600 : green600

    property color buttonBackground: MySettings.chatTheme === "Dark" ? green700 : green700
    property color buttonBackgroundHovered: MySettings.chatTheme === "Dark" ? green500 : green500
    property color buttonBorder: MySettings.chatTheme === "Dark" ? yellow200 : yellow200

    property color sendButtonBackground: yellowAccent
    property color sendButtonBackgroundHovered: MySettings.chatTheme === "Dark" ? darkwhite : black

    property color conversationButtonBackground: MySettings.chatTheme === "Dark" ? darkgray100 : gray0
    property color conversationButtonBackgroundHovered: MySettings.chatTheme === "Dark" ? darkgray0 : gray100
    property color conversationButtonBorder: yellow200

    property color iconBackgroundDark: MySettings.chatTheme === "Dark" ? green400 : green400
    property color iconBackgroundLight: MySettings.chatTheme === "Dark" ? darkwhite : white
    property color iconBackgroundHovered: yellowAccent;

    property color slugBackground: MySettings.chatTheme === "Dark" ? darkgray300 : gray100

    property color textColor: MySettings.chatTheme === "Dark" ? darkwhite : black
    property color mutedTextColor: MySettings.chatTheme === "Dark" ? gray400 : gray600
    property color oppositeTextColor: MySettings.chatTheme === "Dark" ? darkwhite : white
    property color oppositeMutedTextColor: MySettings.chatTheme === "Dark" ? darkwhite : white
    property color textAccent: yellowAccent
    property color textErrorColor: MySettings.chatTheme === "Dark" ? red400 : red400
    property color titleTextColor: MySettings.chatTheme === "Dark" ? green400 : green700

    property color dialogBorder: MySettings.chatTheme === "Dark" ? darkgray0 : darkgray0
    property color linkColor: MySettings.chatTheme === "Dark" ? yellow600 : yellow600

    property color mainHeader: MySettings.chatTheme === "Dark" ? green600 : green600
    property color mainComboBackground:  MySettings.chatTheme === "Dark" ? green700 : green700
    property color sendGlow: MySettings.chatTheme === "Dark" ? green950 : green300

    property color userColor: MySettings.chatTheme === "Dark" ? green700 : green700
    property color assistantColor: yellowAccent

    property real fontSizeFixedSmall: 16

    property real fontSizeSmall: fontSizeLarge - 4
    property real  fontSizeLarge: MySettings.fontSize === "Small" ?
        Qt.application.font.pixelSize : MySettings.fontSize === "Medium" ?
            Qt.application.font.pixelSize + 5 : Qt.application.font.pixelSize + 10
    property real  fontSizeLarger: MySettings.fontSize === "Small" ?
        Qt.application.font.pixelSize + 2 : MySettings.fontSize === "Medium" ?
            Qt.application.font.pixelSize + 7 : Qt.application.font.pixelSize + 12
    property real  fontSizeLargest: MySettings.fontSize === "Small" ?
        Qt.application.font.pixelSize + 7 : MySettings.fontSize === "Medium" ?
            Qt.application.font.pixelSize + 12 : Qt.application.font.pixelSize + 14
}
