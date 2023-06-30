import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import localdocs
import mysettings

MySettingsTab {
    title: qsTr("Models")
    contentItem: GridLayout {
        id: generationSettingsTabInner
        columns: 2
        rowSpacing: 10
        columnSpacing: 10

        MyButton {
            Layout.row: 8
            Layout.column: 1
            Layout.fillWidth: true
            text: qsTr("Restore Defaults")
            Accessible.description: qsTr("Restores the settings dialog to a default state")
            onClicked: {
                MySettings.restoreGenerationDefaults();
                templateTextArea.text = MySettings.promptTemplate
            }
        }
    }
}