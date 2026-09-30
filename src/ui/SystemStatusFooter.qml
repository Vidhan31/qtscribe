pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtScribe
import "controls"

Item {
    id: root

    signal shortcutGuideRequested

    visible: SystemHealthMonitor.systemShortcutHasIssue || SystemHealthMonitor.directTypingHasIssue
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
                visible: SystemHealthMonitor.systemShortcutHasIssue
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
                    color: SystemHealthMonitor.systemShortcutSupported ? Theme.colorWarning : Theme.colorDanger
                }

                StyledText {
                    text: SystemHealthMonitor.systemShortcutStatus.length > 0
                          ? SystemHealthMonitor.systemShortcutStatus : qsTr("Global shortcut unavailable")
                    variant: "caption"
                    colorRole: "secondary"
                    Layout.alignment: Qt.AlignVCenter
                }

                StyledButton {
                    id: setupGuideBtn
                    text: qsTr("Setup Guide")
                    visible: !SystemHealthMonitor.systemShortcutSupported
                    variant: "flat"
                    size: "small"
                    onClicked: root.shortcutGuideRequested()
                }
            }

            StyledDivider {
                id: statusDivider
                visible: SystemHealthMonitor.systemShortcutHasIssue && SystemHealthMonitor.directTypingHasIssue
                orientation: Qt.Vertical
                implicitHeight: 14
                Layout.alignment: Qt.AlignVCenter
            }

            RowLayout {
                id: directTypingStatusRow
                visible: SystemHealthMonitor.directTypingHasIssue
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
                    color: SystemHealthMonitor.directTypingFatalError ? Theme.colorDanger : Theme.colorWarning
                }

                StyledText {
                    text: SystemHealthMonitor.directTypingStatus
                    variant: "caption"
                    colorRole: "secondary"
                    Layout.alignment: Qt.AlignVCenter
                }
            }
        }
    }
}
