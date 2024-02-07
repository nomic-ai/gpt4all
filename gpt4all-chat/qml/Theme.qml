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

    // gray // FIXME: These are slightly less red than what atlas uses. should resolve diff
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

    // purple // FIXME: These are slightly more uniform than what atlas uses. should resolve diff
    property color purple50: Qt.hsla(279/360, 1.0, 0.98)
    property color purple100: Qt.hsla(279/360, 1.0, 0.95)
    property color purple200: Qt.hsla(279/360, 1.0, 0.91)
    property color purple300: Qt.hsla(279/360, 1.0, 0.84)
    property color purple400: Qt.hsla(279/360, 1.0, 0.73)
    property color purple500: Qt.hsla(279/360, 1.0, 0.63)
    property color purple600: Qt.hsla(279/360, 1.0, 0.53)
    property color purple700: Qt.hsla(279/360, 1.0, 0.47)
    property color purple800: Qt.hsla(279/360, 1.0, 0.39)
    property color purple900: Qt.hsla(279/360, 1.0, 0.32)
    property color purple950: Qt.hsla(279/360, 1.0, 0.22)

    property color blue0: "#d0d5db"
    property color blue100: "#8e8ea0"
    property color blue200: "#7d7d8e"
    property color blue400: "#444654"
    property color blue500: "#343541"
    property color blue600: "#2c2d37"
    property color blue800: "#232628"
    property color blue900: "#222527"
    property color blue950: "#1c1f21"
    property color blue1000: "#0e1011"

    property color accentColor: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue200;
            case "Dark":
                return yellow300;
            default:
                return yellow300;
        }
    }

    property color darkContrast: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue500;
            case "Dark":
                return darkgray100;
            default:
                return gray100;
        }
    }

    property color lightContrast: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue400;
            case "Dark":
                return darkgray0;
            default:
                return gray0;
        }
    }

    property color controlBorder: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue800;
            case "Dark":
                return darkgray0;
            default:
                return gray300;
        }
    }

    property color controlBackground: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue950;
            case "Dark":
                return darkgray100;
            default:
                return gray100;
        }
    }

    property color disabledControlBackground: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue950;
            case "Dark":
                return darkgray200;
            default:
                return gray200;
        }
    }

    property color conversationBackground: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue500;
            default:
                return containerBackground;
        }
    }

    property color containerForeground: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue950;
            case "Dark":
                return darkgray300;
            default:
                return gray300;
        }
    }

    property color containerBackground: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue900;
            case "Dark":
                return darkgray200;
            default:
                return gray200;
        }
    }

    property color progressForeground: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return purple400;
            case "Dark":
                return accentColor;
            default:
                return accentColor;
        }
    }

    property color progressBackground: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue900;
            case "Dark":
                return green600;
            default:
                return green600;
        }
    }

    property color progressText: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return "#ffffff";
            case "Dark":
                return "#000000";
            default:
                return "#000000";
        }
    }

    property color checkboxBorder: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return accentColor;
            case "Dark":
                return gray200;
            default:
                return gray600;
        }
    }

    property color checkboxForeground: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return accentColor;
            case "Dark":
                return green600;
            default:
                return green600;
        }
    }

    property color buttonBackground: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue950;
            case "Dark":
                return green700;
            default:
                return green700;
        }
    }

    property color buttonBackgroundHovered: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue900;
            case "Dark":
                return green500;
            default:
                return green500;
        }
    }

    property color buttonBorder: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return accentColor;
            case "Dark":
                return yellow200;
            default:
                return yellow200;
        }
    }

    property color sendButtonBackground: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return accentColor;
            case "Dark":
                return accentColor;
            default:
                return accentColor;
        }
    }

    property color sendButtonBackgroundHovered: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue0;
            case "Dark":
                return darkwhite;
            default:
                return black;
        }
    }

    property color conversationButtonBackground: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue500;
            case "Dark":
                return darkgray100;
            default:
                return gray0;
        }
    }

    property color conversationButtonBackgroundHovered: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue400;
            case "Dark":
                return darkgray0;
            default:
                return gray100;
        }
    }

    property color conversationButtonBorder: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return accentColor;
            case "Dark":
                return yellow200;
            default:
                return yellow200;
        }
    }

    property color iconBackgroundDark: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue200;
            case "Dark":
                return green400;
            default:
                return green400;
        }
    }

    property color iconBackgroundLight: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue200;
            case "Dark":
                return darkwhite;
            default:
                return white;
        }
    }

    property color iconBackgroundHovered: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue0;
            case "Dark":
                return accentColor;
            default:
                return accentColor;
        }
    }

    property color slugBackground: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue600;
            case "Dark":
                return darkgray300;
            default:
                return gray100;
        }
    }

    property color textColor: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue0;
            case "Dark":
                return darkwhite;
            default:
                return black;
        }
    }

    property color mutedTextColor: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue200;
            case "Dark":
                return gray400;
            default:
                return gray600;
        }
    }

    property color oppositeTextColor: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return white;
            case "Dark":
                return darkwhite;
            default:
                return white;
        }
    }

    property color oppositeMutedTextColor: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return white;
            case "Dark":
                return darkwhite;
            default:
                return white;
        }
    }

    property color textAccent: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return accentColor;
            case "Dark":
                return accentColor;
            default:
                return accentColor;
        }
    }

    property color textErrorColor: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return red400;
            case "Dark":
                return red400;
            default:
                return red400;
        }
    }

    property color settingsTitleTextColor: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue100;
            case "Dark":
                return green400;
            default:
                return green700;
        }
    }

    property color titleTextColor: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return purple400;
            case "Dark":
                return green400;
            default:
                return green700;
        }
    }

    property color dialogBorder: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return accentColor;
            case "Dark":
                return darkgray0;
            default:
                return darkgray0;
        }
    }

    property color linkColor: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return yellow600;
            case "Dark":
                return yellow600;
            default:
                return yellow600;
        }
    }

    property color mainHeader: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue900;
            case "Dark":
                return green600;
            default:
                return green600;
        }
    }

    property color mainComboBackground: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue950;
            case "Dark":
                return green700;
            default:
                return green700;
        }
    }

    property color sendGlow: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue1000;
            case "Dark":
                return green950;
            default:
                return green300;
        }
    }

    property color userColor: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return blue800;
            case "Dark":
                return green700;
            default:
                return green700;
        }
    }

    property color assistantColor: {
        switch (MySettings.chatTheme) {
            case "LegacyDark":
                return purple400;
            case "Dark":
                return accentColor;
            default:
                return accentColor;
        }
    }

    property real fontSizeFixedSmall: 16
    property real fontSize: Qt.application.font.pixelSize

    property real fontSizeSmall: fontSizeLarge - 2
    property real  fontSizeLarge: MySettings.fontSize === "Small" ?
        fontSize : MySettings.fontSize === "Medium" ?
            fontSize + 5 : fontSize + 10
    property real  fontSizeLarger: MySettings.fontSize === "Small" ?
        fontSize + 2 : MySettings.fontSize === "Medium" ?
            fontSize + 7 : fontSize + 12
    property real  fontSizeLargest: MySettings.fontSize === "Small" ?
        fontSize + 7 : MySettings.fontSize === "Medium" ?
            fontSize + 12 : fontSize + 14
}
