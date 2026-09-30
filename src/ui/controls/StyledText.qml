pragma ComponentBehavior: Bound
import QtQuick
import QtScribe

Text {
    id: root

    property string variant: "body"
    property string colorRole: "primary"
    property color customColor: "transparent"
    property string fontFamily: "sans"
    property int customPixelSize: 0
    property int customWeight: -1

    textFormat: Text.PlainText

    font.pixelSize: {
        if (root.customPixelSize > 0)
        return root.customPixelSize;
        if (root.variant === "display")
        return Theme.fontSizeDisplay;
        if (root.variant === "heading")
        return Theme.fontSizeHeading;
        if (root.variant === "subheading")
        return Theme.fontSizeSubheading;
        if (root.variant === "caption")
        return Theme.fontSizeCaption;
        if (root.variant === "small")
        return Theme.fontSizeSmall;
        return Theme.fontSizeBody;
    }

    font.weight: {
        if (root.customWeight >= 0)
        return root.customWeight;
        if (root.variant === "display" || root.variant === "heading")
        return Font.Bold;
        if (root.variant === "subheading")
        return Font.DemiBold;
        return Font.Normal;
    }

    font.family: root.fontFamily === "mono" ? "Monospace" : ""

    linkColor: Theme.accentColor

    color: {
        if (root.customColor.a > 0.001)
        return root.customColor;
        if (root.colorRole === "secondary")
        return Theme.textSecondary;
        if (root.colorRole === "tertiary")
        return Theme.textTertiary;
        if (root.colorRole === "accent" || root.colorRole === "link")
        return Theme.accentColor;
        if (root.colorRole === "danger")
        return Theme.colorDanger;
        if (root.colorRole === "success")
        return Theme.colorSuccess;
        if (root.colorRole === "warning")
        return Theme.colorWarning;
        if (root.colorRole === "placeholder")
        return Theme.textPlaceholder;
        if (root.colorRole === "onAccent")
        return Theme.textOnAccent;
        if (root.colorRole === "onDanger")
        return Theme.textOnDanger;
        return Theme.textPrimary;
    }
}
