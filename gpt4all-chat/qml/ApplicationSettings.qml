import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import modellist
import mysettings
import network
import llm

MySettingsTab {
    onRestoreDefaults: {
        MySettings.restoreApplicationDefaults();
    }
    title: qsTr("Application")

    NetworkDialog {
        id: networkDialog
        anchors.centerIn: parent
        width: Math.min(1024, window.width - (window.width * .2))
        height: Math.min(600, window.height - (window.height * .2))
        Item {
            Accessible.role: Accessible.Dialog
            Accessible.name: qsTr("Network dialog")
            Accessible.description: qsTr("opt-in to share feedback/conversations")
        }
    }

    Dialog {
        id: checkForUpdatesError
        anchors.centerIn: parent
        modal: false
        padding: 20
        width: 40 + 400 * theme.fontScale
        Text {
            anchors.fill: parent
            horizontalAlignment: Text.AlignJustify
            text: qsTr("ERROR: Update system could not find the MaintenanceTool used to check for updates!<br/><br/>"
                  + "Did you install this application using the online installer? If so, the MaintenanceTool "
                  + "executable should be located one directory above where this application resides on your "
                  + "filesystem.<br/><br/>If you can't start it manually, then I'm afraid you'll have to reinstall.")
            wrapMode: Text.WordWrap
            color: theme.textErrorColor
            font.pixelSize: theme.fontSizeLarge
            Accessible.role: Accessible.Dialog
            Accessible.name: text
            Accessible.description: qsTr("Error dialog")
        }
        background: Rectangle {
            anchors.fill: parent
            color: theme.containerBackground
            border.width: 1
            border.color: theme.dialogBorder
            radius: 10
        }
    }

    contentItem: GridLayout {
        id: applicationSettingsTabInner
        columns: 3
        rowSpacing: 30
        columnSpacing: 10

        Label {
            Layout.row: 0
            Layout.column: 0
            Layout.bottomMargin: 10
            color: theme.settingsTitleTextColor
            font.pixelSize: theme.fontSizeBannerSmall
            font.bold: true
            text: qsTr("Application Settings")
        }

        ColumnLayout {
            Layout.row: 1
            Layout.column: 0
            Layout.columnSpan: 3
            Layout.fillWidth: true
            spacing: 10
            Label {
                color: theme.styledTextColor
                font.pixelSize: theme.fontSizeLarge
                font.bold: true
                text: qsTr("General")
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: theme.settingsDivider
            }
        }

        MySettingsLabel {
            id: themeLabel
            text: qsTr("Theme")
            helpText: qsTr("The application color scheme.")
            Layout.row: 2
            Layout.column: 0
        }
        MyComboBox {
            id: themeBox
            Layout.row: 2
            Layout.column: 2
            Layout.minimumWidth: 200
            Layout.maximumWidth: 200
            Layout.fillWidth: false
            Layout.alignment: Qt.AlignRight
            // NOTE: indices match values of ChatTheme enum, keep them in sync
            model: ListModel {
                ListElement { name: qsTr("Light") }
                ListElement { name: qsTr("Dark") }
                ListElement { name: qsTr("LegacyDark") }
            }
            Accessible.name: themeLabel.text
            Accessible.description: themeLabel.helpText
            function updateModel() {
                themeBox.currentIndex = MySettings.chatTheme;
            }
            Component.onCompleted: {
                themeBox.updateModel()
            }
            Connections {
                target: MySettings
                function onChatThemeChanged() {
                    themeBox.updateModel()
                }
            }
            onActivated: {
                MySettings.chatTheme = themeBox.currentIndex
            }
        }
        MySettingsLabel {
            id: fontLabel
            text: qsTr("Font Size")
            helpText: qsTr("The size of text in the application.")
            Layout.row: 3
            Layout.column: 0
        }
        MyComboBox {
            id: fontBox
            Layout.row: 3
            Layout.column: 2
            Layout.minimumWidth: 200
            Layout.maximumWidth: 200
            Layout.fillWidth: false
            Layout.alignment: Qt.AlignRight
            // NOTE: indices match values of FontSize enum, keep them in sync
            model: ListModel {
                ListElement { name: qsTr("Small") }
                ListElement { name: qsTr("Medium") }
                ListElement { name: qsTr("Large") }
            }
            Accessible.name: fontLabel.text
            Accessible.description: fontLabel.helpText
            function updateModel() {
                fontBox.currentIndex = MySettings.fontSize;
            }
            Component.onCompleted: {
                fontBox.updateModel()
            }
            Connections {
                target: MySettings
                function onFontSizeChanged() {
                    fontBox.updateModel()
                }
            }
            onActivated: {
                MySettings.fontSize = fontBox.currentIndex
            }
        }
        MySettingsLabel {
            id: languageLabel
            visible: MySettings.uiLanguages.length > 1
            text: qsTr("Language and Locale")
            helpText: qsTr("The language and locale you wish to use.")
            Layout.row: 4
            Layout.column: 0
        }
        MyComboBox {
            id: languageBox
            visible: MySettings.uiLanguages.length > 1
            Layout.row: 4
            Layout.column: 2
            Layout.minimumWidth: 200
            Layout.maximumWidth: 200
            Layout.fillWidth: false
            Layout.alignment: Qt.AlignRight
            model: ListModel {
                Component.onCompleted: {
                    for (var i = 0; i < MySettings.uiLanguages.length; ++i)
                        append({"text": MySettings.uiLanguages[i]});
                    languageBox.updateModel();
                }
                ListElement { text: qsTr("System Locale") }
            }

            Accessible.name: languageLabel.text
            Accessible.description: languageLabel.helpText
            function updateModel() {
                // This usage of 'System Locale' should not be translated
                // FIXME: Make this refer to a string literal variable accessed by both QML and C++
                if (MySettings.languageAndLocale === "System Locale")
                    languageBox.currentIndex = 0
                else
                    languageBox.currentIndex = languageBox.indexOfValue(MySettings.languageAndLocale);
            }
            Component.onCompleted: {
                languageBox.updateModel()
            }
            onActivated: {
                // This usage of 'System Locale' should not be translated
                // FIXME: Make this refer to a string literal variable accessed by both QML and C++
                if (languageBox.currentIndex === 0)
                    MySettings.languageAndLocale = "System Locale";
                else
                    MySettings.languageAndLocale = languageBox.currentText;
            }
        }
        MySettingsLabel {
            id: deviceLabel
            text: qsTr("Device")
            helpText: qsTr('The compute device used for text generation.')
            Layout.row: 5
            Layout.column: 0
        }
        MyComboBox {
            id: deviceBox
            Layout.row: 5
            Layout.column: 2
            Layout.minimumWidth: 400
            Layout.maximumWidth: 400
            Layout.fillWidth: false
            Layout.alignment: Qt.AlignRight
            model: ListModel {
                Component.onCompleted: {
                    for (var i = 0; i < MySettings.deviceList.length; ++i)
                        append({"text": MySettings.deviceList[i]});
                    deviceBox.updateModel();
                }
                ListElement { text: qsTr("Application default") }
            }

            Accessible.name: deviceLabel.text
            Accessible.description: deviceLabel.helpText
            function updateModel() {
                // This usage of 'Auto' should not be translated
                // FIXME: Make this refer to a string literal variable accessed by both QML and C++
                if (MySettings.device === "Auto")
                    deviceBox.currentIndex = 0
                else
                    deviceBox.currentIndex = deviceBox.indexOfValue(MySettings.device);
            }
            Component.onCompleted: {
                deviceBox.updateModel();
            }
            Connections {
                target: MySettings
                function onDeviceChanged() {
                    deviceBox.updateModel();
                }
            }
            onActivated: {
                // This usage of 'Auto' should not be translated
                // FIXME: Make this refer to a string literal variable accessed by both QML and C++
                if (deviceBox.currentIndex === 0)
                    MySettings.device = "Auto";
                else
                    MySettings.device = deviceBox.currentText;
            }
        }
        MySettingsLabel {
            id: defaultModelLabel
            text: qsTr("Default Model")
            helpText: qsTr("The preferred model for new chats. Also used as the local server fallback.")
            Layout.row: 6
            Layout.column: 0
        }
        MyComboBox {
            id: defaultModelBox
            Layout.row: 6
            Layout.column: 2
            Layout.minimumWidth: 400
            Layout.maximumWidth: 400
            Layout.alignment: Qt.AlignRight
            model: ListModel {
                id: defaultModelBoxModel
                Component.onCompleted: {
                    defaultModelBox.rebuildModel()
                }
            }
            Accessible.name: defaultModelLabel.text
            Accessible.description: defaultModelLabel.helpText
            function rebuildModel() {
                defaultModelBoxModel.clear();
                defaultModelBoxModel.append({"text": qsTr("Application default")});
                for (var i = 0; i < ModelList.selectableModelList.length; ++i)
                    defaultModelBoxModel.append({"text": ModelList.selectableModelList[i].name});
                defaultModelBox.updateModel();
            }
            function updateModel() {
                // This usage of 'Application default' should not be translated
                // FIXME: Make this refer to a string literal variable accessed by both QML and C++
                if (MySettings.userDefaultModel === "Application default")
                    defaultModelBox.currentIndex = 0
                else
                    defaultModelBox.currentIndex = defaultModelBox.indexOfValue(MySettings.userDefaultModel);
            }
            onActivated: {
                // This usage of 'Application default' should not be translated
                // FIXME: Make this refer to a string literal variable accessed by both QML and C++
                if (defaultModelBox.currentIndex === 0)
                    MySettings.userDefaultModel = "Application default";
                else
                    MySettings.userDefaultModel = defaultModelBox.currentText;
            }
            Connections {
                target: MySettings
                function onUserDefaultModelChanged() {
                    defaultModelBox.updateModel()
                }
            }
            Connections {
                target: MySettings
                function onLanguageAndLocaleChanged() {
                    defaultModelBox.rebuildModel()
                }
            }
            Connections {
                target: ModelList
                function onSelectableModelListChanged() {
                    defaultModelBox.rebuildModel()
                }
            }
        }
        MySettingsLabel {
            id: suggestionModeLabel
            text: qsTr("Suggestion Mode")
            helpText: qsTr("Generate suggested follow-up questions at the end of responses.")
            Layout.row: 7
            Layout.column: 0
        }
        MyComboBox {
            id: suggestionModeBox
            Layout.row: 7
            Layout.column: 2
            Layout.minimumWidth: 400
            Layout.maximumWidth: 400
            Layout.alignment: Qt.AlignRight
            // NOTE: indices match values of SuggestionMode enum, keep them in sync
            model: ListModel {
                ListElement { name: qsTr("When chatting with LocalDocs") }
                ListElement { name: qsTr("Whenever possible") }
                ListElement { name: qsTr("Never") }
            }
            Accessible.name: suggestionModeLabel.text
            Accessible.description: suggestionModeLabel.helpText
            onActivated: {
                MySettings.suggestionMode = suggestionModeBox.currentIndex;
            }
            Component.onCompleted: {
                suggestionModeBox.currentIndex = MySettings.suggestionMode;
            }
        }
        MySettingsLabel {
            id: modelPathLabel
            text: qsTr("Download Path")
            helpText: qsTr("Where to store local models and the LocalDocs database.")
            Layout.row: 8
            Layout.column: 0
        }

        RowLayout {
            Layout.row: 8
            Layout.column: 2
            Layout.alignment: Qt.AlignRight
            Layout.minimumWidth: 400
            Layout.maximumWidth: 400
            spacing: 10
            MyDirectoryField {
                id: modelPathDisplayField
                text: MySettings.modelPath
                font.pixelSize: theme.fontSizeLarge
                implicitWidth: 300
                Layout.fillWidth: true
                Accessible.name: modelPathLabel.text
                Accessible.description: modelPathLabel.helpText
                onEditingFinished: {
                    if (isValid) {
                        MySettings.modelPath = modelPathDisplayField.text
                    } else {
                        text = MySettings.modelPath
                    }
                }
            }
            MyFolderDialog {
                id: folderDialog
            }
            MySettingsButton {
                text: qsTr("Browse")
                Accessible.description: qsTr("Choose where to save model files")
                onClicked: {
                    folderDialog.openFolderDialog("file://" + MySettings.modelPath, function(selectedFolder) {
                        MySettings.modelPath = selectedFolder
                    })
                }
            }
        }

        MySettingsLabel {
            id: dataLakeLabel
            text: qsTr("Enable Datalake")
            helpText: qsTr("Send chats and feedback to the GPT4All Open-Source Datalake.")
            Layout.row: 9
            Layout.column: 0
        }
        MyCheckBox {
            id: dataLakeBox
            Layout.row: 9
            Layout.column: 2
            Layout.alignment: Qt.AlignRight
            Component.onCompleted: { dataLakeBox.checked = MySettings.networkIsActive; }
            Connections {
                target: MySettings
                function onNetworkIsActiveChanged() { dataLakeBox.checked = MySettings.networkIsActive; }
            }
            onClicked: {
                if (MySettings.networkIsActive)
                    MySettings.networkIsActive = false;
                else
                    networkDialog.open();
                dataLakeBox.checked = MySettings.networkIsActive;
            }
        }

        ColumnLayout {
            Layout.row: 10
            Layout.column: 0
            Layout.columnSpan: 3
            Layout.fillWidth: true
            spacing: 10
            Label {
                color: theme.styledTextColor
                font.pixelSize: theme.fontSizeLarge
                font.bold: true
                text: qsTr("Advanced")
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: theme.settingsDivider
            }
        }

        MySettingsLabel {
            id: nThreadsLabel
            text: qsTr("CPU Threads")
            helpText: qsTr("The number of CPU threads used for inference and embedding.")
            Layout.row: 11
            Layout.column: 0
        }
        MyTextField {
            text: MySettings.threadCount
            color: theme.textColor
            font.pixelSize: theme.fontSizeLarge
            Layout.alignment: Qt.AlignRight
            Layout.row: 11
            Layout.column: 2
            Layout.minimumWidth: 200
            Layout.maximumWidth: 200
            validator: IntValidator {
                bottom: 1
            }
            onEditingFinished: {
                var val = parseInt(text)
                if (!isNaN(val)) {
                    MySettings.threadCount = val
                    focus = false
                } else {
                    text = MySettings.threadCount
                }
            }
            Accessible.role: Accessible.EditableText
            Accessible.name: nThreadsLabel.text
            Accessible.description: ToolTip.text
        }
        MySettingsLabel {
            id: trayLabel
            text: qsTr("Enable System Tray")
            helpText: qsTr("The application will minimize to the system tray when the window is closed.")
            Layout.row: 13
            Layout.column: 0
        }
        MyCheckBox {
            id: trayBox
            Layout.row: 13
            Layout.column: 2
            Layout.alignment: Qt.AlignRight
            checked: MySettings.systemTray
            onClicked: {
                MySettings.systemTray = !MySettings.systemTray
            }
        }
        MySettingsLabel {
            id: serverChatLabel
            text: qsTr("Enable Local API Server")
            helpText: qsTr("Expose an OpenAI-Compatible server to localhost. WARNING: Results in increased resource usage.")
            Layout.row: 14
            Layout.column: 0
        }
        MyCheckBox {
            id: serverChatBox
            Layout.row: 14
            Layout.column: 2
            Layout.alignment: Qt.AlignRight
            checked: MySettings.serverChat
            onClicked: {
                MySettings.serverChat = !MySettings.serverChat
            }
        }
        MySettingsLabel {
            id: serverPortLabel
            text: qsTr("API Server Port")
            helpText: qsTr("The port to use for the local server. Requires restart.")
            Layout.row: 15
            Layout.column: 0
        }
        MyTextField {
            id: serverPortField
            text: MySettings.networkPort
            color: theme.textColor
            font.pixelSize: theme.fontSizeLarge
            Layout.row: 15
            Layout.column: 2
            Layout.minimumWidth: 200
            Layout.maximumWidth: 200
            Layout.alignment: Qt.AlignRight
            validator: IntValidator {
                bottom: 1
            }
            onEditingFinished: {
                var val = parseInt(text)
                if (!isNaN(val)) {
                    MySettings.networkPort = val
                    focus = false
                } else {
                    text = MySettings.networkPort
                }
            }
            Accessible.role: Accessible.EditableText
            Accessible.name: serverPortLabel.text
            Accessible.description: serverPortLabel.helpText
        }

        /*MySettingsLabel {
            id: gpuOverrideLabel
            text: qsTr("Force Metal (macOS+arm)")
            Layout.row: 13
            Layout.column: 0
        }
        MyCheckBox {
            id: gpuOverrideBox
            Layout.row: 13
            Layout.column: 2
            Layout.alignment: Qt.AlignRight
            checked: MySettings.forceMetal
            onClicked: {
                MySettings.forceMetal = !MySettings.forceMetal
            }
            ToolTip.text: qsTr("WARNING: On macOS with arm (M1+) this setting forces usage of the GPU. Can cause crashes if the model requires more RAM than the system supports. Because of crash possibility the setting will not persist across restarts of the application. This has no effect on non-macs or intel.")
            ToolTip.visible: hovered
        }*/

        MySettingsLabel {
            id: updatesLabel
            text: qsTr("Check For Updates")
            helpText: qsTr("Manually check for an update to GPT4All.");
            Layout.row: 16
            Layout.column: 0
        }

        MySettingsButton {
            Layout.row: 16
            Layout.column: 2
            Layout.alignment: Qt.AlignRight
            text: qsTr("Updates");
            onClicked: {
                if (!LLM.checkForUpdates())
                    checkForUpdatesError.open()
            }
        }

        Rectangle {
            Layout.row: 17
            Layout.column: 0
            Layout.columnSpan: 3
            Layout.fillWidth: true
            height: 1
            color: theme.settingsDivider
        }
    }
}

