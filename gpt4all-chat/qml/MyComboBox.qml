import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

ComboBox {
    font.pixelSize: theme.fontSizeLarge
    spacing: 0
    padding: 10
    Accessible.role: Accessible.ComboBox
    contentItem: Text {
        anchors.horizontalCenter: parent.horizontalCenter
        leftPadding: 10
        rightPadding: 10
        text: comboBox.displayText
        font: comboBox.font
        color: theme.textColor
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
    }
    delegate: ItemDelegate {
        width: comboBox.width
        contentItem: Text {
            text: modelData
            color: theme.textColor
            font: comboBox.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: highlighted ? theme.backgroundLight : theme.backgroundDark
        }
        highlighted: comboBox.highlightedIndex === index
    }
    popup: Popup {
        y: comboBox.height - 1
        width: comboBox.width
        implicitHeight: contentItem.implicitHeight
        padding: 0

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: comboBox.popup.visible ? comboBox.delegateModel : null
            currentIndex: comboBox.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: theme.backgroundDark
        }
    }
    background: Rectangle {
        color: theme.backgroundDark
        border.width: 1
        border.color: theme.backgroundLightest
        radius: 10
    }
}
