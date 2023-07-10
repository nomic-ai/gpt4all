import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import llm

TextField {
    id: myDirectoryField
    padding: 10
    property bool isValid: LLM.directoryExists(text)
    color: text === "" || isValid ? theme.textColor : theme.textErrorColor
    background: Rectangle {
        implicitWidth: 150
        color: theme.backgroundDark
        radius: 10
    }
}
