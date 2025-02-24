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
        contentWidth: availableWidth
        clip: true
        Flow {
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 20
            bottomPadding: 20
            property int childWidth: 330 * theme.fontScale
            property int childHeight: 400 + 166 * theme.fontScale
            RemoteModelCard {
                width: parent.childWidth
                height: parent.childHeight
                providerBaseUrl: "https://api.groq.com/openai/v1/"
                providerName: qsTr("Groq")
                providerImage: "qrc:/gpt4all/icons/groq.svg"
                providerDesc: qsTr('Groq offers a high-performance AI inference engine designed for low-latency and efficient processing. Optimized for real-time applications, Groq’s technology is ideal for users who need fast responses from open large language models and other AI workloads.<br><br>Get your API key: <a href="https://console.groq.com/keys">https://groq.com/</a>')
                modelWhitelist: [
                    // last updated 2025-02-24
                    "deepseek-r1-distill-llama-70b",
                    "deepseek-r1-distill-qwen-32b",
                    "gemma2-9b-it",
                    "llama-3.1-8b-instant",
                    "llama-3.2-1b-preview",
                    "llama-3.2-3b-preview",
                    "llama-3.3-70b-specdec",
                    "llama-3.3-70b-versatile",
                    "llama3-70b-8192",
                    "llama3-8b-8192",
                    "mixtral-8x7b-32768",
                    "qwen-2.5-32b",
                    "qwen-2.5-coder-32b",
                ]
            }
            RemoteModelCard {
                width: parent.childWidth
                height: parent.childHeight
                providerBaseUrl: "https://api.openai.com/v1/"
                providerName: qsTr("OpenAI")
                providerImage: "qrc:/gpt4all/icons/openai.svg"
                providerDesc: qsTr('OpenAI provides access to advanced AI models, including GPT-4 supporting a wide range of applications, from conversational AI to content generation and code completion.<br><br>Get your API key: <a href="https://platform.openai.com/signup">https://openai.com/</a>')
                modelWhitelist: [
                    // last updated 2025-02-24
                    "gpt-3.5-turbo",
                    "gpt-3.5-turbo-16k",
                    "gpt-4",
                    "gpt-4-32k",
                    "gpt-4-turbo",
                    "gpt-4o",
                ]
            }
            RemoteModelCard {
                width: parent.childWidth
                height: parent.childHeight
                providerBaseUrl: "https://api.mistral.ai/v1/"
                providerName: qsTr("Mistral")
                providerImage: "qrc:/gpt4all/icons/mistral.svg"
                providerDesc: qsTr('Mistral AI specializes in efficient, open-weight language models optimized for various natural language processing tasks. Their models are designed for flexibility and performance, making them a solid option for applications requiring scalable AI solutions.<br><br>Get your API key: <a href="https://mistral.ai/">https://mistral.ai/</a>')
                modelWhitelist: [
                    // last updated 2025-02-24
                    "codestral-2405",
                    "codestral-2411-rc5",
                    "codestral-2412",
                    "codestral-2501",
                    "codestral-latest",
                    "codestral-mamba-2407",
                    "codestral-mamba-latest",
                    "ministral-3b-2410",
                    "ministral-3b-latest",
                    "ministral-8b-2410",
                    "ministral-8b-latest",
                    "mistral-large-2402",
                    "mistral-large-2407",
                    "mistral-large-2411",
                    "mistral-large-latest",
                    "mistral-medium-2312",
                    "mistral-medium-latest",
                    "mistral-saba-2502",
                    "mistral-saba-latest",
                    "mistral-small-2312",
                    "mistral-small-2402",
                    "mistral-small-2409",
                    "mistral-small-2501",
                    "mistral-small-latest",
                    "mistral-tiny-2312",
                    "mistral-tiny-2407",
                    "mistral-tiny-latest",
                    "open-codestral-mamba",
                    "open-mistral-7b",
                    "open-mistral-nemo",
                    "open-mistral-nemo-2407",
                    "open-mixtral-8x22b",
                    "open-mixtral-8x22b-2404",
                    "open-mixtral-8x7b",
                ]
            }
            RemoteModelCard {
                width: parent.childWidth
                height: parent.childHeight
                providerIsCustom: true
                providerName: qsTr("Custom")
                providerImage: "qrc:/gpt4all/icons/antenna_3.svg"
                providerDesc: qsTr("The custom provider option allows users to connect their own OpenAI-compatible AI models or third-party inference services. This is useful for organizations with proprietary models or those leveraging niche AI providers not listed here.")
            }
        }
    }
}
