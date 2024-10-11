import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

BusyIndicator {
    id: control

    property real size: 48
    property color color: theme.accentColor

    contentItem: Item {
        implicitWidth: control.size
        implicitHeight: control.size

        Item {
            id: item
            x: parent.width / 2 - width / 2
            y: parent.height / 2 - height / 2
            width: control.size
            height: control.size
            opacity: control.running ? 1 : 0

            Behavior on opacity {
                OpacityAnimator {
                    duration: 250
                }
            }

            RotationAnimator {
                target: item
                running: control.visible && control.running
                from: 0
                to: 360
                loops: Animation.Infinite
                duration: 1750
            }

            Repeater {
                id: repeater
                model: 6

                Rectangle {
                    id: delegate
                    x: item.width / 2 - width / 2
                    y: item.height / 2 - height / 2
                    implicitWidth: control.size * .2
                    implicitHeight: control.size * .2
                    radius: control.size * .1
                    color: control.color

                    required property int index

                    transform: [
                        Translate {
                            y: -Math.min(item.width, item.height) * 0.5 + delegate.radius
                        },
                        Rotation {
                            angle: delegate.index / repeater.count * 360
                            origin.x: delegate.radius
                            origin.y: delegate.radius
                        }
                    ]
                }
            }
        }
    }
}
