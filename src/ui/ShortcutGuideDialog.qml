pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import QtScribe
import "controls"

T.Dialog {
    id: root

    title: qsTr("Setup Custom Desktop Shortcut")
    modal: true
    width: 480
    x: Math.round(((Overlay.overlay ? Overlay.overlay.width : 960) - width) / 2)
    y: Math.round(((Overlay.overlay ? Overlay.overlay.height : 660) - height) / 2)
    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside

    background: Rectangle {
        color: Theme.cardBgElevated
        border.color: Theme.cardBorder
        border.width: 1
        radius: Theme.radiusMd
    }

    header: Rectangle {
        implicitHeight: 44
        color: "transparent"

        StyledText {
            anchors.left: parent.left
            anchors.leftMargin: Theme.spacingMd
            anchors.verticalCenter: parent.verticalCenter
            text: root.title
            variant: "subheading"
            customWeight: Font.Bold
        }
    }

    ColumnLayout {
        width: parent.width
        spacing: Theme.spacingMd

        StyledText {
            text: qsTr(
                      "Your desktop environment does not support the XDG Global Shortcuts portal directly, but you can easily set up a system-wide hotkey using the QtScribe CLI command:")
            variant: "body"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 38
            color: Theme.inputBg
            border.color: Theme.inputBorder
            border.width: 1
            radius: Theme.radiusSm

            RowLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingSm

                StyledText {
                    text: "qtscribe --toggle"
                    fontFamily: "mono"
                    customWeight: Font.Bold
                    colorRole: "accent"
                    Layout.fillWidth: true
                }

                StyledButton {
                    id: copyCmdBtn
                    text: qsTr("Copy Command")
                    iconSource: "qrc:/qt/qml/QtScribe/assets/icons/copy.svg"
                    variant: "flat"
                    size: "small"
                    onClicked: {
                        TranscriptionModel.copyToClipboard("qtscribe --toggle");
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            StyledText {
                text: qsTr("Steps for GNOME:")
                customWeight: Font.DemiBold
                variant: "body"
            }

            StyledText {
                text: qsTr(
                          "1. Open <b>Settings → Keyboard → View and Customize Shortcuts</b>.<br>2. Scroll down to <b>Custom Shortcuts</b> and click <b>+</b>.<br>3. Set Name to <i>QtScribe</i>, Command to <code>qtscribe --toggle</code>, and Shortcut to <b>Ctrl+Shift+Space</b>.")
                textFormat: Text.StyledText
                variant: "caption"
                colorRole: "secondary"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        StyledText {
            text: qsTr(
                      "<b>Note:</b> Custom desktop shortcuts operate in <b>Toggle mode</b>. Push-to-Talk requires native support for the <code>org.freedesktop.portal.GlobalShortcuts</code> portal (available on KDE Plasma 6+ and compatible Wayland compositors).")
            textFormat: Text.StyledText
            variant: "caption"
            colorRole: "secondary"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
