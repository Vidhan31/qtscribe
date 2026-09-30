pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtScribe
import "controls"

Rectangle {
    id: root

    property string title: ""
    property string message: ""
    property string bannerType: "info"
    property color customBorderColor: "transparent"

    property string actionText: ""
    property bool actionHighlighted: true
    property bool actionEnabled: true

    property string secondaryActionText: ""
    property bool secondaryActionHighlighted: false
    property bool secondaryActionEnabled: true

    default property alias extraContent: customContentContainer.data

    signal actionClicked
    signal secondaryActionClicked

    readonly property color resolvedAccentColor: {
        if (bannerType === "warning")
        return Theme.colorWarning;
        if (bannerType === "danger")
        return Theme.colorDanger;
        if (bannerType === "success")
        return Theme.colorSuccess;
        return Theme.accentColor;
    }

    readonly property bool isCompact: root.width > 0 && root.width < 500

    implicitHeight: bannerLayout.implicitHeight + (Theme.spacingMd * 2)
    color: {
        if (bannerType === "warning")
        return Theme.statusWarningBg;
        if (bannerType === "danger")
        return Theme.statusDangerBg;
        if (bannerType === "success")
        return Theme.statusSuccessBg;
        return Theme.statusAccentBg;
    }
    border.color: {
        if (customBorderColor.a > 0.001)
        return customBorderColor;
        if (bannerType === "warning")
        return Theme.statusWarningBorder;
        if (bannerType === "danger")
        return Theme.statusDangerBorder;
        if (bannerType === "success")
        return Theme.statusSuccessBorder;
        return Theme.statusAccentBorder;
    }
    border.width: 1
    radius: 0

    Behavior on color {
        ColorAnimation {
            duration: Theme.animFast
            easing.type: Easing.OutCubic
        }
    }

    Behavior on border.color {
        ColorAnimation {
            duration: Theme.animFast
            easing.type: Easing.OutCubic
        }
    }

    Layout.fillWidth: true

    GridLayout {
        id: bannerLayout
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: Theme.spacingMd
        }
        columns: root.isCompact ? 1 : 2
        columnSpacing: Theme.spacingSm
        rowSpacing: Theme.spacingSm

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingXs

            StyledText {
                text: root.title
                variant: "body"
                customWeight: Font.DemiBold
                customColor: root.resolvedAccentColor
                visible: root.title.length > 0
            }

            StyledText {
                text: root.message
                variant: "caption"
                colorRole: "secondary"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                visible: root.message.length > 0
            }

            ColumnLayout {
                id: customContentContainer
                Layout.fillWidth: true
                spacing: Theme.spacingXs
            }
        }

        RowLayout {
            spacing: Theme.spacingSm
            visible: root.actionText.length > 0 || root.secondaryActionText.length > 0
            Layout.alignment: root.isCompact ? Qt.AlignLeft : Qt.AlignVCenter

            StyledButton {
                id: bannerPrimaryBtn
                text: root.actionText
                enabled: root.actionEnabled
                size: "small"
                variant: root.actionHighlighted ? "primary" : "secondary"
                customAccentColor: root.resolvedAccentColor
                visible: root.actionText.length > 0
                onClicked: root.actionClicked()
            }

            StyledButton {
                id: bannerSecondaryBtn
                text: root.secondaryActionText
                enabled: root.secondaryActionEnabled
                size: "small"
                variant: root.secondaryActionHighlighted ? "primary" : "secondary"
                visible: root.secondaryActionText.length > 0
                onClicked: root.secondaryActionClicked()
            }
        }
    }
}
