pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtScribe
import "../controls"

Item {
    id: root

    property string title: ""
    property string description: ""
    property bool checked: false

    signal toggled

    Layout.fillWidth: true
    implicitHeight: Math.max(Theme.switchTrackHeight + Theme.spacingSm, switchRow.implicitHeight + Theme.spacingSm)

    activeFocusOnTab: true
    Accessible.role: Accessible.CheckBox
    Accessible.name: root.title
    Accessible.description: root.description
    Accessible.checked: root.checked

    Keys.onSpacePressed: event => {
        root.checked = !root.checked;
        root.toggled();
        event.accepted = true;
    }

    Keys.onReturnPressed: event => {
        root.checked = !root.checked;
        root.toggled();
        event.accepted = true;
    }

    RowLayout {
        id: switchRow
        anchors.fill: parent
        spacing: Theme.spacingMd

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            StyledText {
                text: root.title
                variant: "body"
                customWeight: Font.Medium
                visible: root.title.length > 0
            }

            StyledText {
                text: root.description
                variant: "caption"
                colorRole: "secondary"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                visible: root.description.length > 0
            }
        }

        Item {
            Layout.preferredWidth: Theme.switchTrackWidth
            Layout.preferredHeight: Theme.switchTrackHeight
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

            Rectangle {
                id: switchTrack
                anchors.fill: parent
                radius: Theme.radiusCircle
                color: root.checked ? Theme.switchTrackOn : Theme.switchTrackOff
                border.color: root.activeFocus ? Theme.focusRingColor : "transparent"
                border.width: root.activeFocus ? Theme.focusRingWidth : 0

                Behavior on color {
                    ColorAnimation {
                        duration: Theme.animFast
                        easing.type: Easing.OutCubic
                    }
                }

                Rectangle {
                    id: switchThumb
                    width: Theme.switchThumbSize
                    height: Theme.switchThumbSize
                    radius: Theme.radiusCircle
                    color: Theme.switchThumb
                    y: (parent.height - height) / 2
                    x: root.checked ? (parent.width - width - 2) : 2

                    Behavior on x {
                        NumberAnimation {
                            duration: Theme.animFast
                            easing.type: Easing.OutCubic
                        }
                    }
                }
            }
        }
    }

    MouseArea {
        id: switchArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true

        onClicked: {
            root.forceActiveFocus();
            root.checked = !root.checked;
            root.toggled();
        }
    }
}
