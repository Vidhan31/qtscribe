pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtScribe

T.Slider {
    id: root

    implicitWidth: 200
    implicitHeight: 24

    background: Rectangle {
        x: root.leftPadding
        y: root.topPadding + (root.availableHeight - height) / 2
        implicitWidth: 200
        implicitHeight: 6
        width: root.availableWidth
        height: implicitHeight
        radius: Theme.radiusCircle
        color: Theme.controlBg
        border.color: Theme.controlBorder
        border.width: 1

        Rectangle {
            width: root.visualPosition * parent.width
            height: parent.height
            color: root.enabled ? Theme.accentColor : Theme.textPlaceholder
            radius: Theme.radiusCircle
        }
    }

    handle: Rectangle {
        x: root.leftPadding + root.visualPosition * (root.availableWidth - width)
        y: root.topPadding + (root.availableHeight - height) / 2
        implicitWidth: 18
        implicitHeight: 18
        radius: Theme.radiusCircle
        color: root.pressed ? Theme.sliderThumbBgPressed : Theme.sliderThumbBg
        border.color: (root.activeFocus || root.visualFocus) ? Theme.focusRingColor : Theme.sliderThumbBorder
        border.width: (root.activeFocus || root.visualFocus) ? Theme.focusRingWidth : 1

        scale: root.pressed ? 1.15 : (root.hovered ? 1.08 : 1.0)

        Behavior on scale {
            NumberAnimation {
                duration: Theme.animFast
                easing.type: Easing.OutCubic
            }
        }
    }
}
