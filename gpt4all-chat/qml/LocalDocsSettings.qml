import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import localdocs
import modellist
import mysettings
import network

MySettingsTab {
    onRestoreDefaultsClicked: {
        MySettings.restoreLocalDocsDefaults();
    }

    showRestoreDefaultsButton: true

    title: qsTr("LocalDocs")
    contentItem: ColumnLayout {
        id: root
        spacing: 10

        Label {
            color: theme.styledTextColor
            font.pixelSize: theme.fontSizeLarge
            font.bold: true
            text: "Indexing"
        }

        Rectangle {
            Layout.bottomMargin: 15
            Layout.fillWidth: true
            height: 2
            color: theme.settingsDivider
        }

        RowLayout {
            MySettingsLabel {
                id: extsLabel
                text: qsTr("Allowed File Extensions")
                helpText: qsTr("Comma-separated list. LocalDocs will only attempt to process files with these extensions.")
            }
            MyTextField {
                id: extsField
                text: MySettings.localDocsFileExtensions.join(',')
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
                Layout.alignment: Qt.AlignRight
                Layout.minimumWidth: 200
                validator: RegularExpressionValidator {
                    regularExpression: /([^ ,\/"']+,?)*/
                }
                onEditingFinished: {
                    // split and remove empty elements
                    var exts = text.split(',').filter(e => e);
                    // normalize and deduplicate
                    exts = exts.map(e => e.toLowerCase());
                    exts = Array.from(new Set(exts));
                    /* Blacklist common unsupported file extensions. We only support plain text and PDFs, and although we
                     * reject binary data, we don't want to waste time trying to index files that we don't support. */
                    exts = exts.filter(e => ![
                        /* Microsoft documents  */ "rtf", "docx", "ppt", "pptx", "xls", "xlsx",
                        /* OpenOffice           */ "odt", "ods", "odp", "odg",
                        /* photos               */ "jpg", "jpeg", "png", "gif", "bmp", "tif", "tiff", "webp",
                        /* audio                */ "mp3", "wma", "m4a", "wav", "flac",
                        /* videos               */ "mp4", "mov", "webm", "mkv", "avi", "flv", "wmv",
                        /* executables          */ "exe", "com", "dll", "so", "dylib", "msi",
                        /* binary images        */ "iso", "img", "dmg",
                        /* archives             */ "zip", "jar", "apk", "rar", "7z", "tar", "gz", "xz", "bz2", "tar.gz",
                                                   "tgz", "tar.xz", "tar.bz2",
                        /* misc                 */ "bin",
                    ].includes(e));
                    MySettings.localDocsFileExtensions = exts;
                    extsField.text = exts.join(',');
                    focus = false;
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: extsLabel.text
                Accessible.description: extsLabel.helpText
            }
        }

        Label {
            Layout.topMargin: 15
            color: theme.grayRed900
            font.pixelSize: theme.fontSizeLarge
            font.bold: true
            text: "Embedding"
        }

        Rectangle {
            Layout.bottomMargin: 15
            Layout.fillWidth: true
            height: 2
            color: theme.grayRed500
        }

        RowLayout {
            MySettingsLabel {
                text: qsTr("Use Nomic Embed API")
                helpText: qsTr("Embed documents using the fast Nomic API instead of a private local model.")
            }

            MyCheckBox {
                id: useNomicAPIBox
                Component.onCompleted: {
                    useNomicAPIBox.checked = MySettings.localDocsUseRemoteEmbed;
                }
                onClicked: {
                    MySettings.localDocsUseRemoteEmbed = useNomicAPIBox.checked && MySettings.localDocsNomicAPIKey !== "";
                }
            }
        }

        RowLayout {
            MySettingsLabel {
                id: apiKeyLabel
                text: qsTr("Nomic API Key")
                helpText: qsTr("API key to use for Nomic Embed. Get one at https://atlas.nomic.ai/")
            }

            MyTextField {
                id: apiKeyField
                text: MySettings.localDocsNomicAPIKey
                color: apiKeyField.acceptableInput ? theme.textColor : theme.textErrorColor
                font.pixelSize: theme.fontSizeLarge
                Layout.alignment: Qt.AlignRight
                Layout.minimumWidth: 200
                enabled: useNomicAPIBox.checked
                validator: RegularExpressionValidator {
                    // may be empty
                    regularExpression: /|nk-[a-zA-Z0-9_-]{43}/
                }
                onEditingFinished: {
                    MySettings.localDocsNomicAPIKey = apiKeyField.text;
                    MySettings.localDocsUseRemoteEmbed = useNomicAPIBox.checked && MySettings.localDocsNomicAPIKey !== "";
                    focus = false;
                }
                Accessible.role: Accessible.EditableText
                Accessible.name: apiKeyLabel.text
                Accessible.description: apiKeyLabel.helpText
            }
        }

        Label {
            Layout.topMargin: 15
            color: theme.grayRed900
            font.pixelSize: theme.fontSizeLarge
            font.bold: true
            text: "Display"
        }

        Rectangle {
            Layout.bottomMargin: 15
            Layout.fillWidth: true
            height: 2
            color: theme.grayRed500
        }

        RowLayout {
            MySettingsLabel {
                id: showReferencesLabel
                text: qsTr("Show sources")
                helpText: qsTr("Shows sources in GUI generated by localdocs")
            }
            MyCheckBox {
                id: showReferencesBox
                checked: MySettings.localDocsShowReferences
                onClicked: {
                    MySettings.localDocsShowReferences = !MySettings.localDocsShowReferences
                }
            }
        }

        Label {
            Layout.topMargin: 15
            color: theme.styledTextColor
            font.pixelSize: theme.fontSizeLarge
            font.bold: true
            text: "Advanced"
        }

        Rectangle {
            Layout.bottomMargin: 15
            Layout.fillWidth: true
            height: 2
            color: theme.settingsDivider
        }

        MySettingsLabel {
            id: warningLabel
            Layout.bottomMargin: 15
            Layout.fillWidth: true
            color: theme.textErrorColor
            wrapMode: Text.WordWrap
            text: qsTr("Warning: Advanced usage only.")
            helpText: qsTr("Values too large may cause localdocs failure, extremely slow responses or failure to respond at all. Roughly speaking, the {N chars x N snippets} are added to the model's context window. More info <a href=\"https://docs.gpt4all.io/gpt4all_chat.html#localdocs-beta-plugin-chat-with-your-data\">here.</a>")
            onLinkActivated: function(link) { Qt.openUrlExternally(link) }
        }

        RowLayout {
            MySettingsLabel {
                id: chunkLabel
                Layout.fillWidth: true
                text: qsTr("Document snippet size (characters)")
                helpText: qsTr("Number of characters per document snippet. Larger numbers increase likelihood of factual responses, but also result in slower generation.")
            }

            MyTextField {
                id: chunkSizeTextField
                text: MySettings.localDocsChunkSize
                validator: IntValidator {
                    bottom: 1
                }
                onEditingFinished: {
                    var val = parseInt(text)
                    if (!isNaN(val)) {
                        MySettings.localDocsChunkSize = val
                        focus = false
                    } else {
                        text = MySettings.localDocsChunkSize
                    }
                }
            }
        }

        RowLayout {
            Layout.topMargin: 15
            MySettingsLabel {
                id: contextItemsPerPrompt
                text: qsTr("Max document snippets per prompt")
                helpText: qsTr("Max best N matches of retrieved document snippets to add to the context for prompt. Larger numbers increase likelihood of factual responses, but also result in slower generation.")

            }

            MyTextField {
                text: MySettings.localDocsRetrievalSize
                validator: IntValidator {
                    bottom: 1
                }
                onEditingFinished: {
                    var val = parseInt(text)
                    if (!isNaN(val)) {
                        MySettings.localDocsRetrievalSize = val
                        focus = false
                    } else {
                        text = MySettings.localDocsRetrievalSize
                    }
                }
            }
        }

        Rectangle {
            Layout.topMargin: 15
            Layout.fillWidth: true
            height: 2
            color: theme.settingsDivider
        }
     }
}
