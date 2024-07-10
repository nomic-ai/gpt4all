import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

Menu {
    id: menu

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding + 20)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding + 20)

    background: Rectangle {
        implicitWidth: 220
        implicitHeight: 40
        color: theme.menuBackgroundColor
        border.color: theme.menuBorderColor
        border.width: 1
        radius: 10
    }

    contentItem: Rectangle {
        implicitWidth: myListView.contentWidth
        implicitHeight: myListView.contentHeight
        color: "transparent"
        ListView {
            id: myListView
            anchors.margins: 10
            anchors.fill: parent
            implicitHeight: contentHeight
            model: menu.contentModel
            interactive: Window.window
                         ? contentHeight + menu.topPadding + menu.bottomPadding > menu.height
                         : false
            clip: true
            currentIndex: menu.currentIndex

            ScrollIndicator.vertical: ScrollIndicator {}
        }
    }

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            easing.type: Easing.InOutQuad
            duration: 100
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            easing.type: Easing.InOutQuad
            duration: 100
        }
    }
}
