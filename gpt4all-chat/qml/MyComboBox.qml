import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

ComboBox {
    id: comboBox
    font.pixelSize: theme.fontSizeLarge
    spacing: 0
    padding: 10
    Accessible.role: Accessible.ComboBox
    contentItem: RowLayout {
        id: contentRow
        spacing: 0
        Text {
            id: text
            Layout.fillWidth: true
            leftPadding: 10
            rightPadding: 20
            text: comboBox.displayText
            font: comboBox.font
            color: theme.textColor
            verticalAlignment: Text.AlignLeft
            elide: Text.ElideRight
        }
        Item {
            Layout.preferredWidth: updown.width
            Layout.preferredHeight: updown.height
            Image {
                id: updown
                anchors.verticalCenter: parent.verticalCenter
                sourceSize.width: comboBox.font.pixelSize
                sourceSize.height: comboBox.font.pixelSize
                mipmap: true
                visible: false
                source: "qrc:/gpt4all/icons/up_down.svg"
            }

            ColorOverlay {
                anchors.fill: updown
                source: updown
                color: theme.textColor
            }
        }
    }
    delegate: ItemDelegate {
        width: comboBox.width -20
        contentItem: Text {
            text: modelData
            color: theme.textColor
            font: comboBox.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 10
            color: highlighted ? theme.menuHighlightColor : theme.menuBackgroundColor
        }
        highlighted: comboBox.highlightedIndex === index
    }
    popup: Popup {
        // FIXME This should be made much nicer to take into account lists that are very long so
        // that it is scrollable and also sized optimally taking into account the x,y and the content
        // width and height as well as the window width and height
        y: comboBox.height - 1
        width: comboBox.width
        implicitHeight: contentItem.implicitHeight + 20
        padding: 0

        contentItem: Rectangle {
            implicitWidth: myListView.contentWidth
            implicitHeight: myListView.contentHeight
            color: "transparent"
            ListView {
                id: myListView
                anchors.fill: parent
                anchors.margins: 10
                clip: true
                implicitHeight: contentHeight
                model: comboBox.popup.visible ? comboBox.delegateModel : null
                currentIndex: comboBox.highlightedIndex
                ScrollIndicator.vertical: ScrollIndicator { }
            }
        }

        background: Rectangle {
            color: theme.menuBackgroundColor//theme.controlBorder
            border.color: theme.menuBorderColor //theme.controlBorder
            border.width: 1
            radius: 10
        }
    }
    indicator: Item {
    }
    background: Rectangle {
        color: theme.controlBackground
        border.width: 1
        border.color: theme.controlBorder
        radius: 10
    }
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
}
