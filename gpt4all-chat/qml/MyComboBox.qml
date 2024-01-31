import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

ComboBox {
    id: comboBox
    font.pixelSize: theme.fontSizeLarge
    spacing: 0
    padding: 10
    Accessible.role: Accessible.ComboBox
    contentItem: Text {
        id: text
        leftPadding: 10
        rightPadding: 20
        text: comboBox.displayText
        font: comboBox.font
        color: theme.textColor
        verticalAlignment: Text.AlignVCenter
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
            color: highlighted ? theme.lightContrast : theme.darkContrast
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
            color: theme.black
        }
    }
    indicator: Canvas {
        id: canvas
        x: comboBox.width - width - comboBox.rightPadding
        y: comboBox.topPadding + (comboBox.availableHeight - height) / 2
        width: 12
        height: 18
        contextType: "2d"

        Connections {
            target: comboBox
            function onPressedChanged() { canvas.requestPaint(); }
        }

        onPaint: {
            var context = getContext("2d");
            context.reset();
            context.lineWidth = 2;
            context.moveTo(0, height / 2 - 2);
            context.lineTo(width / 2, 0);
            context.lineTo(width, height / 2 - 2);
            context.moveTo(0, height / 2 + 2);
            context.lineTo(width / 2, height);
            context.lineTo(width, height / 2 + 2);
            context.strokeStyle = comboBox.pressed ? theme.gray400 : theme.gray300;
            context.stroke();

        }
    }
    background: Rectangle {
        color: theme.controlBackground
        border.width: 1
        border.color: theme.controlBorder
        radius: 10
    }
    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
}
