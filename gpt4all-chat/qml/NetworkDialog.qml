import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import download
import network
import llm
import mysettings

MyDialog {
    id: networkDialog
    anchors.centerIn: parent
    modal: true
    padding: 20

    Theme {
        id: theme
    }

    Column {
        id: column
        spacing: 20
        Item {
            width: childrenRect.width
            height: childrenRect.height
            Image {
                id: img
                anchors.top: parent.top
                anchors.left: parent.left
                width: 60
                height: 60
                source: "qrc:/gpt4all/icons/logo.svg"
            }
            Text {
                anchors.left: img.right
                anchors.leftMargin: 30
                anchors.verticalCenter: img.verticalCenter
                text: qsTr("Contribute data to the GPT4All Opensource Datalake.")
                color: theme.textColor
                font.pixelSize: theme.fontSizeLarge
            }
        }

        ScrollView {
            clip: true
            height: 300
            width: 1024 - 40
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            MyTextArea {
                id: textOptIn
                width: 1024 - 40
                text: qsTr("By enabling this feature, you will be able to participate in the democratic process of training a large language model by contributing data for future model improvements.

When a GPT4All model responds to you and you have opted-in, your conversation will be sent to the GPT4All Open Source Datalake. Additionally, you can like/dislike its response. If you dislike a response, you can suggest an alternative response. This data will be collected and aggregated in the GPT4All Datalake.

NOTE: By turning on this feature, you will be sending your data to the GPT4All Open Source Datalake. You should have no expectation of chat privacy when this feature is enabled. You should; however, have an expectation of an optional attribution if you wish. Your chat data will be openly available for anyone to download and will be used by Nomic AI to improve future GPT4All models. Nomic AI will retain all attribution information attached to your data and you will be credited as a contributor to any GPT4All model release that uses your data!")
                focus: false
                readOnly: true
                Accessible.role: Accessible.Paragraph
                Accessible.name: qsTr("Terms for opt-in")
                Accessible.description: qsTr("Describes what will happen when you opt-in")
            }
        }

        MyTextField {
            id: attribution
            width: parent.width
            text: MySettings.networkAttribution
            placeholderText: qsTr("Please provide a name for attribution (optional)")
            Accessible.role: Accessible.EditableText
            Accessible.name: qsTr("Attribution (optional)")
            Accessible.description: qsTr("Provide attribution")
            onEditingFinished: {
                MySettings.networkAttribution = attribution.text;
            }
        }
    }

    footer: DialogButtonBox {
        id: dialogBox
        padding: 20
        alignment: Qt.AlignRight
        spacing: 10
        MySettingsButton {
            text: qsTr("Enable")
            Accessible.description: qsTr("Enable opt-in")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        MySettingsButton {
            text: qsTr("Cancel")
            Accessible.description: qsTr("Cancel opt-in")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        background: Rectangle {
            color: "transparent"
        }
    }

    onAccepted: {
        if (MySettings.networkIsActive)
            return
        MySettings.networkIsActive = true;
        Network.sendNetworkToggled(true);
    }

    onRejected: {
        if (!MySettings.networkIsActive)
            return
        MySettings.networkIsActive = false;
        Network.sendNetworkToggled(false);
    }
}
