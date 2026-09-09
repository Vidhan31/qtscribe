pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtScribe

T.ComboBox {
    id: root

    implicitWidth: 200
    implicitHeight: 36
    font.pixelSize: Theme.fontSizeBody

    contentItem: Text {
        leftPadding: Theme.spacingMd
        rightPadding: root.indicator ? root.indicator.width + Theme.spacingMd : Theme.spacingMd
        text: root.displayText
        font.pixelSize: root.font.pixelSize
        font.weight: root.font.weight
        color: root.enabled ? Theme.textPrimary : Theme.textPlaceholder
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Item {
        x: root.width - width - Theme.spacingMd
        y: (root.height - height) / 2
        width: 12
        height: 12

        Text {
            anchors.centerIn: parent
            text: "▾"
            font.pixelSize: 12
            color: root.popup.visible ? Theme.accentColor : Theme.textSecondary
            rotation: root.popup.visible ? 180 : 0

            Behavior on rotation {
                NumberAnimation {
                    duration: Theme.animFast
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    background: Rectangle {
        implicitWidth: 160
        implicitHeight: 36
        color: root.down ? Theme.controlBgPressed : (root.hovered ? Theme.controlBgHover : Theme.controlBg)
        border.color: (root.activeFocus || root.visualFocus) ? Theme.focusRingColor : Theme.controlBorder
        border.width: (root.activeFocus || root.visualFocus) ? Theme.focusRingWidth : 1
        radius: Theme.radiusSm

        Behavior on color {
            ColorAnimation {
                duration: Theme.animFast
                easing.type: Easing.OutCubic
            }
        }
    }

    popup: T.Popup {
        y: root.height + 4
        width: Math.max(root.width, 180)
        implicitHeight: Math.min(260, popupListView.contentHeight + topPadding + bottomPadding)
        padding: 4

        background: Rectangle {
            color: Theme.cardBgElevated
            border.color: Theme.cardBorder
            border.width: 1
            radius: Theme.radiusMd
        }

        contentItem: ListView {
            id: popupListView
            clip: true
            implicitHeight: contentHeight
            model: root.delegateModel
            currentIndex: root.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds

            T.ScrollBar.vertical: T.ScrollBar {
                policy: T.ScrollBar.AsNeeded
                width: 6
            }
        }
    }

    delegate: T.ItemDelegate {
        id: delegateItem
        required property int index

        readonly property string itemText: root.textAt(delegateItem.index)

        width: ListView.view ? ListView.view.width : root.width
        implicitHeight: 32
        highlighted: root.highlightedIndex === delegateItem.index
        hoverEnabled: true

        onClicked: {
            root.currentIndex = delegateItem.index;
            root.activated(delegateItem.index);
            root.popup.close();
        }

        contentItem: Text {
            text: delegateItem.itemText
            font.pixelSize: root.font.pixelSize
            font.weight: delegateItem.highlighted ? Font.DemiBold : Font.Normal
            color: delegateItem.highlighted ? Theme.accentColor : Theme.textPrimary
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            leftPadding: Theme.spacingSm
            rightPadding: Theme.spacingSm
        }

        background: Rectangle {
            radius: Theme.radiusSm
            color: {
                if (delegateItem.highlighted)
                return Theme.sidebarItemSelected;
                if (delegateItem.hovered || delegateItem.visualFocus)
                return Theme.sidebarItemHover;
                return "transparent";
            }
        }
    }
}
