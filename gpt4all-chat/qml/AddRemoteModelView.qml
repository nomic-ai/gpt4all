import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.folderlistmodel
import Qt5Compat.GraphicalEffects

import llm
import chatlistmodel
import download
import modellist
import network
import gpt4all
import mysettings
import localdocs

ColumnLayout {
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop
    spacing: 5

    Label {
        Layout.topMargin: 0
        Layout.bottomMargin: 25
        Layout.rightMargin: 150 * theme.fontScale
        Layout.alignment: Qt.AlignTop
        Layout.fillWidth: true
        verticalAlignment: Text.AlignTop
        text: qsTr("Various remote model providers that use network resources for inference.")
        font.pixelSize: theme.fontSizeLarger
        color: theme.textColor
        wrapMode: Text.WordWrap
    }

    ScrollView {
        id: scrollView
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        GridLayout {
            rows: 2
            columns: 3
            rowSpacing: 20
            columnSpacing: 20
            RemoteModelCard {
                Layout.preferredWidth: 600
                Layout.minimumHeight: 700
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                providerBaseUrl: "https://api.groq.com/openai/v1/"
                providerName: qsTr("Groq")
                providerImage: "qrc:/gpt4all/icons/groq.svg"
                providerDesc: qsTr('Groq offers a high-performance AI inference engine designed for low-latency and efficient processing. Optimized for real-time applications, Groq’s technology is ideal for users who need fast responses from open large language models and other AI workloads.<br><br>Get your API key: <a href="https://console.groq.com/keys">https://groq.com/</a>')
            }
            RemoteModelCard {
                Layout.preferredWidth: 600
                Layout.minimumHeight: 700
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                providerBaseUrl: "https://api.openai.com/v1/"
                providerName: qsTr("OpenAI")
                providerImage: "qrc:/gpt4all/icons/openai.svg"
                providerDesc: qsTr('OpenAI provides access to advanced AI models, including GPT-4 supporting a wide range of applications, from conversational AI to content generation and code completion.<br><br>Get your API key: <a href="https://platform.openai.com/signup">https://openai.com/</a>')
                Component.onCompleted: {
                    filterModels = function(names) {
                        // Define a whitelist of allowed names
                        var whitelist = ["gpt-3.5-turbo", "gpt-4o", "gpt-4", "gpt-4-turbo", "gpt-4-32k", "gpt-3.5-turbo-16k"];

                        // Filter names based on the whitelist
                        return names.filter(name => whitelist.includes(name));
                    }
                }
            }
            RemoteModelCard {
                Layout.preferredWidth: 600
                Layout.minimumHeight: 700
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                providerBaseUrl: "https://api.mistral.ai/v1/"
                providerName: qsTr("Mistral")
                providerImage: "qrc:/gpt4all/icons/mistral.svg"
                providerDesc: qsTr('Mistral AI specializes in efficient, open-weight language models optimized for various natural language processing tasks. Their models are designed for flexibility and performance, making them a solid option for applications requiring scalable AI solutions.<br><br>Get your API key: <a href="https://mistral.ai/">https://mistral.ai/</a>')
            }
            RemoteModelCard {
                Layout.preferredWidth: 600
                Layout.minimumHeight: 700
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                providerIsCustom: true
                providerName: qsTr("Custom")
                providerImage: "qrc:/gpt4all/icons/antenna_3.svg"
                providerDesc: qsTr("The custom provider option allows users to connect their own OpenAI-compatible AI models or third-party inference services. This is useful for organizations with proprietary models or those leveraging niche AI providers not listed here.")
            }
        }
    }
}
