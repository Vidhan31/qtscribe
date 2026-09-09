pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtScribe

Rectangle {
    id: root

    property string variant: "default"
    property int padding: Theme.spacingMd
    property int topPadding: root.padding
    property int bottomPadding: root.padding
    property int leftPadding: root.padding
    property int rightPadding: root.padding
    property int customRadius: Theme.radiusNone
    property color customBgColor: "transparent"
    property color customBorderColor: "transparent"
    property int customBorderWidth: 1
    property bool hasBorder: true

    property string title: ""
    property string description: ""
    property bool showHeaderDivider: true

    default property alias content: contentContainer.data

    Layout.fillWidth: true
    implicitHeight: mainLayout.implicitHeight + root.topPadding + root.bottomPadding

    radius: root.customRadius

    color: {
        if (root.customBgColor.a > 0.001)
        return root.customBgColor;
        if (root.variant === "elevated")
        return Theme.cardBgElevated;
        if (root.variant === "subtle")
        return Theme.cardBgSubtle;
        return Theme.cardBgDefault;
    }

    border.color: {
        if (!root.hasBorder)
        return "transparent";
        if (root.customBorderColor.a > 0.001)
        return root.customBorderColor;
        return Theme.cardBorder;
    }
    border.width: root.hasBorder ? root.customBorderWidth : 0

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.topMargin: root.topPadding
        anchors.bottomMargin: root.bottomPadding
        anchors.leftMargin: root.leftPadding
        anchors.rightMargin: root.rightPadding
        spacing: Theme.spacingSm

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            visible: root.title.length > 0 || root.description.length > 0

            StyledText {
                text: root.title
                variant: "subheading"
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

        StyledDivider {
            Layout.topMargin: 2
            Layout.bottomMargin: Theme.spacingXs
            visible: root.showHeaderDivider && (root.title.length > 0 || root.description.length > 0)
        }

        ColumnLayout {
            id: contentContainer
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingSm
        }
    }
}
