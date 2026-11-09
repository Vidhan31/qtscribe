pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import QtScribe

T.Button {
    id: root

    property string variant: "secondary"
    property string size: "medium"
    property string iconText: ""
    property string iconSource: ""
    property color customAccentColor: "transparent"
    property int customRadius: Theme.radiusSm

    readonly property color resolvedAccent: customAccentColor.a > 0.001 ? customAccentColor : Theme.accentColor

    implicitHeight: {
        if (root.size === "small")
        return 28;
        if (root.size === "large")
        return 38;
        return 34;
    }

    implicitWidth: Math.max((root.size === "small" ? 64 : 76), contentRow.implicitWidth + (root.size === "small"
                                                                                           ? Theme.spacingSm * 2 :
                                                                                             Theme.spacingMd * 2))

    padding: 0
    leftPadding: root.size === "small" ? Theme.spacingSm : Theme.spacingMd
    rightPadding: root.size === "small" ? Theme.spacingSm : Theme.spacingMd
    topPadding: 0
    bottomPadding: 0

    font.pixelSize: {
        if (root.size === "small")
        return Theme.fontSizeCaption;
        if (root.size === "large")
        return Theme.fontSizeSubheading;
        return Theme.fontSizeBody;
    }
    font.weight: root.variant === "primary" ? Font.DemiBold : Font.Medium

    readonly property color contentColor: {
        if (!root.enabled)
        return Theme.textPlaceholder;
        if (root.variant === "primary" || root.variant === "danger")
        return Theme.textOnAccent;
        if (root.variant === "ghost" || root.variant === "flat")
        return root.hovered ? Theme.accentColorHover : (root.customAccentColor.a > 0.001 ? root.customAccentColor :
                                                                                           Theme.textLink);
        return Theme.textPrimary;
    }

    contentItem: Item {
        implicitWidth: contentRow.implicitWidth
        implicitHeight: contentRow.implicitHeight

        RowLayout {
            id: contentRow
            anchors.centerIn: parent
            spacing: Theme.spacingXs

            StyledIcon {
                visible: root.iconSource !== ""
                source: root.iconSource
                size: root.size === "small" ? 14 : 16
                color: root.contentColor
                Layout.alignment: Qt.AlignVCenter
            }

            StyledText {
                id: btnLabel
                visible: root.text.length > 0
                text: root.text
                customPixelSize: root.font.pixelSize
                customWeight: root.font.weight
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                Layout.alignment: Qt.AlignVCenter
                elide: Text.ElideRight
                customColor: root.contentColor
            }
        }
    }

    background: Rectangle {
        id: bgRect
        radius: root.customRadius
        opacity: root.enabled ? 1.0 : 0.55

        color: {
            if (root.variant === "primary") {
                if (root.customAccentColor.a > 0.001) {
                    return root.resolvedAccent;
                }
                if (root.down)
                return Theme.buttonPrimaryBgPressed;
                if (root.hovered)
                return Theme.buttonPrimaryBgHover;
                return Theme.buttonPrimaryBg;
            }
            if (root.variant === "danger") {
                if (root.down)
                return Theme.buttonDangerBgPressed;
                if (root.hovered)
                return Theme.buttonDangerBgHover;
                return Theme.buttonDangerBg;
            }
            if (root.variant === "ghost" || root.variant === "flat") {
                if (root.down)
                return Theme.pressedOverlay;
                if (root.hovered)
                return Theme.hoverOverlay;
                return "transparent";
            }
            if (root.down)
            return Theme.controlBgPressed;
            if (root.hovered)
            return Theme.controlBgHover;
            return Theme.controlBg;
        }

        border.color: {
            if (root.visualFocus)
            return Theme.focusRingColor;
            if (root.variant === "secondary")
            return Theme.controlBorder;
            if (root.variant === "ghost" || root.variant === "flat" || root.variant === "primary" || root.variant
                === "danger")
            return "transparent";
            return Theme.controlBorder;
        }
        border.width: root.visualFocus ? Theme.focusRingWidth : (root.variant === "secondary" ? 1 : 0)

        Behavior on color {
            ColorAnimation {
                duration: Theme.animFast
                easing.type: Easing.OutCubic
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: bgRect.radius
            color: root.down ? Theme.pressedOverlay : (root.hovered ? Theme.hoverOverlay : "transparent")
            visible: root.variant === "primary" && root.customAccentColor.a > 0.001 && (root.down || root.hovered)
        }
    }
}
