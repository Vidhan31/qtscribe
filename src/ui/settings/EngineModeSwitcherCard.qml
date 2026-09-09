pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtScribe
import "../controls"

StyledCard {
    id: root

    title: qsTr("Transcription Engine")
    description: qsTr("Select between fast Cloud-based AI transcription and private On-Device inference")

    GridLayout {
        Layout.fillWidth: true
        columns: root.width >= 560 ? 2 : 1
        columnSpacing: Theme.spacingMd
        rowSpacing: Theme.spacingMd

        Rectangle {
            id: cloudOptionCard
            Layout.fillWidth: true
            Layout.preferredHeight: cloudContent.implicitHeight + (Theme.spacingMd * 2)
            radius: Theme.radiusMd
            color: isSelected ? Theme.selectedBg : (cloudMouse.containsMouse ? Theme.cardBgElevated :
                                                                               Theme.cardBgSubtle)

            border.color: isSelected ? Theme.accentColor : (cloudMouse.containsMouse ? Theme.cardBorderHover :
                                                                                       Theme.cardBorder)
            border.width: isSelected ? 2 : 1

            readonly property bool isSelected: DictationCoordinator.activeBackend === DictationCoordinator.Cloud

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

            ColumnLayout {
                id: cloudContent
                anchors.fill: parent
                anchors.margins: Theme.spacingMd
                spacing: Theme.spacingSm

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    Rectangle {
                        implicitWidth: 32
                        implicitHeight: 32
                        radius: Theme.radiusSm
                        color: cloudOptionCard.isSelected ? Theme.sidebarItemSelected : Theme.controlBg

                        StyledIcon {
                            anchors.centerIn: parent
                            source: "qrc:/qt/qml/QtScribe/assets/icons/sparkles.svg"
                            size: 16
                            color: cloudOptionCard.isSelected ? Theme.accentColor : Theme.textSecondary
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        StyledText {
                            text: qsTr("Cloud Transcription")
                            variant: "body"
                            customWeight: Font.DemiBold
                        }

                        StyledText {
                            text: qsTr("%1 Transcription").arg(CloudProviderModel.activeProviderName)
                            variant: "caption"
                            colorRole: "secondary"
                        }
                    }

                    StateBadge {
                        text: qsTr("Active")
                        statusType: "accent"
                        visible: cloudOptionCard.isSelected
                        showDot: true
                    }
                }

                StyledDivider {
                    dividerColor: Theme.cardBorder
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    StyledText {
                        text: qsTr("• Ultra-fast cloud speech recognition (<1s)")
                        variant: "caption"
                        colorRole: "secondary"
                    }

                    StyledText {
                        text: qsTr("• Text enhancement & smart formatting")
                        variant: "caption"
                        colorRole: "secondary"
                    }

                    StyledText {
                        text: qsTr("• Multiple provider options (Groq & Gemini)")
                        variant: "caption"
                        colorRole: "secondary"
                    }
                }
            }

            MouseArea {
                id: cloudMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (DictationCoordinator.activeBackend !== DictationCoordinator.Cloud) {
                        DictationCoordinator.activeBackend = DictationCoordinator.Cloud;
                    }
                }
            }
        }

        Rectangle {
            id: offlineOptionCard
            Layout.fillWidth: true
            Layout.preferredHeight: offlineContent.implicitHeight + (Theme.spacingMd * 2)
            radius: Theme.radiusMd
            color: isSelected ? Theme.selectedBg : (offlineMouse.containsMouse ? Theme.cardBgElevated :
                                                                                 Theme.cardBgSubtle)

            border.color: isSelected ? Theme.accentColor : (offlineMouse.containsMouse ? Theme.cardBorderHover :
                                                                                         Theme.cardBorder)
            border.width: isSelected ? 2 : 1

            readonly property bool isSelected: DictationCoordinator.activeBackend === DictationCoordinator.WhisperCpp

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

            ColumnLayout {
                id: offlineContent
                anchors.fill: parent
                anchors.margins: Theme.spacingMd
                spacing: Theme.spacingSm

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    Rectangle {
                        implicitWidth: 32
                        implicitHeight: 32
                        radius: Theme.radiusSm
                        color: offlineOptionCard.isSelected ? Theme.sidebarItemSelected : Theme.controlBg

                        StyledIcon {
                            anchors.centerIn: parent
                            source: "qrc:/qt/qml/QtScribe/assets/icons/bolt.svg"
                            size: 16
                            color: offlineOptionCard.isSelected ? Theme.accentColor : Theme.textSecondary
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        StyledText {
                            text: qsTr("Offline Transcription")
                            variant: "body"
                            customWeight: Font.DemiBold
                        }

                        StyledText {
                            text: qsTr("whisper.cpp Local Engine")
                            variant: "caption"
                            colorRole: "secondary"
                        }
                    }

                    StateBadge {
                        text: qsTr("Active")
                        statusType: "accent"
                        visible: offlineOptionCard.isSelected
                        showDot: true
                    }
                }

                StyledDivider {
                    dividerColor: Theme.cardBorder
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    StyledText {
                        text: qsTr("• 100% private on-device inference")
                        variant: "caption"
                        colorRole: "secondary"
                    }

                    StyledText {
                        text: qsTr("• Zero internet connection needed")
                        variant: "caption"
                        colorRole: "secondary"
                    }

                    StyledText {
                        text: qsTr("• Uses downloaded Whisper models")
                        variant: "caption"
                        colorRole: "secondary"
                    }
                }
            }

            MouseArea {
                id: offlineMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (DictationCoordinator.activeBackend !== DictationCoordinator.WhisperCpp) {
                        DictationCoordinator.activeBackend = DictationCoordinator.WhisperCpp;
                    }
                }
            }
        }
    }

    StyledDivider {
        Layout.fillWidth: true
        dividerColor: Theme.cardBorder
    }

    PreferenceSwitch {
        Layout.fillWidth: true
        title: qsTr("Automatic Offline Fallback")
        description: qsTr("Automatically switch to local whisper.cpp model when internet connectivity is lost")
        checked: DictationCoordinator.autoSwitchOffline
        onToggled: DictationCoordinator.autoSwitchOffline = checked
    }
}
