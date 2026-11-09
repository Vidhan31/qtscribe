pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtScribe

T.TextField {
    id: root

    property bool isSearchPill: false

    implicitHeight: 34
    placeholderTextColor: Theme.textPlaceholder
    color: Theme.textPrimary
    selectionColor: Theme.accentColor
    selectedTextColor: Theme.textOnAccent
    selectByMouse: true
    font.pixelSize: Theme.fontSizeBody

    clip: true
    leftPadding: isSearchPill ? Theme.spacingMd : Theme.spacingSm + 2
    rightPadding: isSearchPill ? Theme.spacingMd : Theme.spacingSm + 2
    topPadding: Math.max(0, Math.floor((height - contentHeight) / 2))
    bottomPadding: topPadding
    verticalAlignment: TextInput.AlignVCenter

    background: Rectangle {
        color: Theme.inputBg
        border.color: root.activeFocus ? Theme.focusRingColor : Theme.inputBorder
        border.width: root.activeFocus ? Theme.focusRingWidth : 1
        radius: root.isSearchPill ? Theme.radiusCircle : Theme.radiusSm

        Behavior on border.color {
            ColorAnimation {
                duration: Theme.animFast
                easing.type: Easing.OutCubic
            }
        }
    }
}
