pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import QtScribe
import "controls"

T.Dialog {
    id: root

    title: qsTr("Clipboard Overwrite Notice")
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

    contentItem: T.ScrollView {
        id: scrollView
        implicitHeight: Math.min(warningLayout.implicitHeight, 420)
        clip: true

        ColumnLayout {
            id: warningLayout
            width: scrollView.availableWidth
            spacing: Theme.spacingMd

            StyledText {
                text: qsTr(
                          "QtScribe uses clipboard paste injection simulating Ctrl+Shift+V to insert transcribed speech directly into your active applications on Wayland.")
                variant: "body"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: cardContentColumn.implicitHeight + (Theme.spacingMd * 2)
                color: Theme.cardBgSubtle
                border.color: Theme.cardBorder
                border.width: 1
                radius: Theme.radiusSm

                ColumnLayout {
                    id: cardContentColumn
                    anchors.fill: parent
                    anchors.margins: Theme.spacingMd
                    spacing: Theme.spacingMd

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: Theme.spacingSm

                        StyledIcon {
                            source: "qrc:/qt/qml/QtScribe/assets/icons/check.svg"
                            color: Theme.colorSuccess
                            size: 18
                            Layout.alignment: Qt.AlignTop
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            spacing: 2

                            StyledText {
                                text: qsTr("Text is Preserved")
                                customWeight: Font.DemiBold
                                variant: "body"
                                customColor: Theme.colorSuccess
                            }

                            StyledText {
                                text: qsTr(
                                          "Copied plain text is automatically backed up and restored immediately after dictation paste completes.")
                                variant: "caption"
                                colorRole: "secondary"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: Theme.cardBorder
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: Theme.spacingSm

                        StyledIcon {
                            source: "qrc:/qt/qml/QtScribe/assets/icons/info.svg"
                            color: Theme.colorWarning
                            size: 18
                            Layout.alignment: Qt.AlignTop
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            spacing: 2

                            StyledText {
                                text: qsTr("Images & Media are Overwritten")
                                customWeight: Font.DemiBold
                                variant: "body"
                                customColor: Theme.colorWarning
                            }

                            StyledText {
                                text: qsTr(
                                          "Non-text clipboard formats (such as images, screenshots, or copied files) cannot be restored and will be overwritten.")
                                variant: "caption"
                                colorRole: "secondary"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: Theme.cardBorder
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: Theme.spacingSm

                        StyledIcon {
                            source: "qrc:/qt/qml/QtScribe/assets/icons/keyboard.svg"
                            color: Theme.accentColor
                            size: 18
                            Layout.alignment: Qt.AlignTop
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            spacing: 2

                            StyledText {
                                text: qsTr("Ctrl+Shift+V Application Compatibility")
                                customWeight: Font.DemiBold
                                variant: "body"
                                customColor: Theme.accentColor
                            }

                            StyledText {
                                text: qsTr(
                                          "Text is injected by simulating the Ctrl+Shift+V shortcut (standard for terminals, code editors, and plain-text pasting). Please ensure your active application supports Ctrl+Shift+V for pasting.")
                                variant: "caption"
                                colorRole: "secondary"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: Theme.cardBorder
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: Theme.spacingSm

                        StyledIcon {
                            source: "qrc:/qt/qml/QtScribe/assets/icons/copy.svg"
                            color: Theme.accentColor
                            size: 18
                            Layout.alignment: Qt.AlignTop
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            spacing: 2

                            StyledText {
                                text: qsTr("Install a Clipboard Manager")
                                customWeight: Font.DemiBold
                                variant: "body"
                                customColor: Theme.accentColor
                            }

                            StyledText {
                                text: qsTr(
                                          "To avoid losing copied images, screenshots, or files during transcription, consider installing or enabling a clipboard manager (such as Clipboard Indicator or GPaste on GNOME, Klipper on KDE, or cliphist on wlroots).")
                                variant: "caption"
                                colorRole: "secondary"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }

            StyledText {
                text: qsTr(
                          "Before transcribing, please paste and save any important copied media files, or install a desktop clipboard manager extension.")
                variant: "caption"
                colorRole: "secondary"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }

    footer: Rectangle {
        implicitHeight: 52
        color: "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingMd

            Item {
                Layout.fillWidth: true
            }

            StyledButton {
                id: understandBtn
                text: qsTr("I Understand")
                variant: "primary"
                size: "medium"
                onClicked: {
                    DaemonDiagnosticModel.clipboardWarningAcknowledged = true;
                    root.accept();
                }
            }
        }
    }
}
