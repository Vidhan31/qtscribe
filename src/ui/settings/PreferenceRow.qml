pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtScribe
import "../controls"

ColumnLayout {
    id: root

    property string title: ""
    property string description: ""
    property bool layoutVertical: false

    default property alias content: dynamicContainer.data

    Layout.fillWidth: true
    spacing: Theme.spacingXs

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spacingMd
        visible: !root.layoutVertical && (root.title.length > 0 || root.description.length > 0)

        ColumnLayout {
            Layout.fillWidth: true
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
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2
        visible: root.layoutVertical && (root.title.length > 0 || root.description.length > 0)

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

    ColumnLayout {
        id: dynamicContainer
        Layout.fillWidth: true
        spacing: Theme.spacingSm
    }
}
