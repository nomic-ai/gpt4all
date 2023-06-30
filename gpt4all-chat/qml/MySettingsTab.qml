import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

Item {
    property string title: ""
    property Item contentItem: null

    onContentItemChanged: function() {
        if (contentItem) {
            contentItem.parent = tabInner;
            contentItem.anchors.left = tabInner.left;
            contentItem.anchors.right = tabInner.right;
        }
    }

    ScrollView {
        id: root
        width: parent.width
        height: parent.height
        padding: 15
        rightPadding: 20
        contentWidth: availableWidth
        contentHeight: tabInner.height
        ScrollBar.vertical.policy: ScrollBar.AlwaysOn

        Theme {
            id: theme
        }

//        background: Rectangle {
//            color: 'transparent'
//            border.color: theme.tabBorder
//            border.width: 1
//            radius: 10
//        }

        Column {
            id: tabInner
            anchors.left: parent.left
            anchors.right: parent.right
        }
    }
}
