pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtScribe
import "controls"

Item {
    id: root

    signal shortcutGuideRequested

    visible: DaemonClient.systemShortcutHasIssue || DaemonClient.textInjectionHasIssue
    Layout.fillWidth: true
    implicitHeight: 32

    Rectangle {
        id: systemStatusPill
        anchors.centerIn: parent
        implicitHeight: 32
        implicitWidth: footerStatusRow.implicitWidth + (Theme.spacingMd * 2)
        radius: Theme.radiusCircle
        color: Theme.cardBgSubtle
        border.color: Theme.cardBorder
        border.width: 1

        RowLayout {
            id: footerStatusRow
            anchors.centerIn: parent
            spacing: Theme.spacingMd

            RowLayout {
                id: shortcutStatusRow
                visible: DaemonClient.systemShortcutHasIssue
                spacing: 6

                StyledIcon {
                    source: "qrc:/qt/qml/QtScribe/assets/icons/keyboard.svg"
                    size: 14
                    color: Theme.textSecondary
                    Layout.alignment: Qt.AlignVCenter
                    opacity: 0.8
                }

                Rectangle {
                    implicitWidth: 6
                    implicitHeight: 6
                    radius: 3
                    Layout.alignment: Qt.AlignVCenter
                    color: DaemonClient.shortcutSupported ? Theme.colorWarning : Theme.colorDanger
                }

                StyledText {
                    text: DaemonClient.shortcutStatusMessage.length > 0 ? DaemonClient.shortcutStatusMessage : qsTr(
                                                                              "Global shortcut unavailable")
                    variant: "caption"
                    colorRole: "secondary"
                    Layout.alignment: Qt.AlignVCenter
                }

                StyledButton {
                    id: setupGuideBtn
                    text: qsTr("Setup Guide")
                    visible: !DaemonClient.shortcutSupported
                    variant: "flat"
                    size: "small"
                    onClicked: root.shortcutGuideRequested()
                }
            }

            StyledDivider {
                id: statusDivider
                visible: DaemonClient.systemShortcutHasIssue && DaemonClient.textInjectionHasIssue
                orientation: Qt.Vertical
                implicitHeight: 14
                Layout.alignment: Qt.AlignVCenter
            }

            RowLayout {
                id: textInjectionStatusRow
                visible: DaemonClient.textInjectionHasIssue
                spacing: 6

                StyledIcon {
                    source: "qrc:/qt/qml/QtScribe/assets/icons/bolt.svg"
                    size: 13
                    color: Theme.textSecondary
                    Layout.alignment: Qt.AlignVCenter
                    opacity: 0.8
                }

                Rectangle {
                    implicitWidth: 6
                    implicitHeight: 6
                    radius: 3
                    Layout.alignment: Qt.AlignVCenter
                    color: DaemonClient.textInjectionHasFatalError ? Theme.colorDanger : Theme.colorWarning
                }

                StyledText {
                    text: DaemonClient.textInjectionStatus
                    variant: "caption"
                    colorRole: "secondary"
                    Layout.alignment: Qt.AlignVCenter
                }
            }
        }
    }
}
