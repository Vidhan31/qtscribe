pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtScribe
import "controls"

Item {
    id: root

    implicitWidth: 620
    implicitHeight: 540

    component ComponentItem: ColumnLayout {
        id: compItem
        property string title: ""
        property string url: ""
        property string urlLabel: "GitHub"
        property string description: ""
        property string badgeText: ""

        Layout.fillWidth: true
        spacing: Theme.spacingXs

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSm

            StyledText {
                text: compItem.title
                variant: "body"
                customWeight: Font.Medium
            }

            StateBadge {
                text: compItem.badgeText
                statusType: "neutral"
                visible: compItem.badgeText.length > 0
                pixelSize: Theme.fontSizeSmall
            }

            Item {
                Layout.fillWidth: true
            }

            StyledText {
                text: '<a href="' + compItem.url + '" style="color: ' + Theme.accentColor.toString()
                      + '; text-decoration: none;">' + compItem.urlLabel + ' ↗</a>'
                textFormat: Text.StyledText
                variant: "caption"
                customWeight: Font.Medium
                onLinkActivated: link => Qt.openUrlExternally(link)

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    acceptedButtons: Qt.NoButton
                }
            }
        }

        StyledText {
            text: compItem.description
            variant: "caption"
            colorRole: "secondary"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    ScrollView {
        id: creditsScrollView
        anchors.fill: parent
        clip: true

        ColumnLayout {
            width: creditsScrollView.availableWidth
            spacing: Theme.spacingMd

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spacingXs
                Layout.bottomMargin: Theme.spacingSm
                spacing: 4

                StyledText {
                    text: qsTr("About")
                    variant: "heading"
                }

                StyledText {
                    text: qsTr("Application information, components, and open-source libraries")
                    variant: "caption"
                    colorRole: "secondary"
                }
            }

            StyledCard {
                Layout.fillWidth: true

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingMd

                    Image {
                        source: "qrc:/qt/qml/QtScribe/assets/speech-to-text-64.png"
                        sourceSize: Qt.size(44, 44)
                        Layout.preferredWidth: 44
                        Layout.preferredHeight: 44
                        Layout.alignment: Qt.AlignVCenter
                        smooth: true
                        mipmap: true
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingXs

                        RowLayout {
                            spacing: Theme.spacingSm

                            StyledText {
                                text: qsTr("QtScribe")
                                variant: "heading"
                            }

                            StateBadge {
                                text: Qt.application.version ? "v" + Qt.application.version : "v1.0.0"
                                statusType: "accent"
                            }

                            StateBadge {
                                text: qsTr("GPL-3.0-or-later")
                                statusType: "neutral"
                            }
                        }

                        StyledText {
                            text: qsTr("Fast and modern speech-to-text dictation client for Wayland.")
                            variant: "body"
                            colorRole: "secondary"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        StyledText {
                            text: '<a href="https://github.com/Vidhan31/qtscribe" style="color: '
                                  + Theme.accentColor.toString()
                                  + '; text-decoration: none;">github.com/Vidhan31/qtscribe ↗</a>'
                            textFormat: Text.StyledText
                            variant: "caption"
                            customWeight: Font.Medium
                            onLinkActivated: link => Qt.openUrlExternally(link)

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                acceptedButtons: Qt.NoButton
                            }
                        }
                    }
                }
            }

            StyledCard {
                Layout.fillWidth: true

                StyledText {
                    text: qsTr("Speech & Machine Learning")
                    variant: "subheading"
                    Layout.bottomMargin: Theme.spacingXs
                }

                ComponentItem {
                    title: qsTr("whisper.cpp")
                    badgeText: qsTr("MIT")
                    url: "https://github.com/ggerganov/whisper.cpp"
                    urlLabel: "ggerganov/whisper.cpp"
                    description: qsTr(
                                     "High-performance C/C++ inference engine for OpenAI's Whisper model, providing private offline transcription with optional Vulkan GPU acceleration.")
                }

                StyledDivider {}

                ComponentItem {
                    title: qsTr("Groq Cloud API")
                    badgeText: qsTr("Cloud AI")
                    url: "https://github.com/groq"
                    urlLabel: "github.com/groq"
                    description: qsTr(
                                     "Ultra-fast cloud LPU inference service powering real-time Whisper speech-to-text recognition and LLaMA text enhancement.")
                }
            }

            StyledCard {
                Layout.fillWidth: true

                StyledText {
                    text: qsTr("Core Framework & Platform")
                    variant: "subheading"
                    Layout.bottomMargin: Theme.spacingXs
                }

                ComponentItem {
                    title: qsTr("Qt 6 Framework")
                    badgeText: qsTr("LGPL-3.0 / GPL-3.0")
                    url: "https://github.com/qt/qtbase"
                    urlLabel: "qt/qtbase"
                    description: qsTr(
                                     "Cross-platform GUI framework, declarative QML / Qt Quick presentation layer, Multimedia audio recording, Network, and D-Bus IPC.")
                }

                StyledDivider {}

                ComponentItem {
                    title: qsTr("QtKeychain")
                    badgeText: qsTr("BSD-3-Clause")
                    url: "https://github.com/frankosterfeld/qtkeychain"
                    urlLabel: "frankosterfeld/qtkeychain"
                    description: qsTr(
                                     "Platform-independent Qt library providing secure credential storage using Linux Secret Service (freedesktop.org) and KWallet.")
                }

                StyledDivider {}

                ComponentItem {
                    title: qsTr("Vulkan Headers & SDK")
                    badgeText: qsTr("Apache-2.0")
                    url: "https://github.com/KhronosGroup/Vulkan-Headers"
                    urlLabel: "KhronosGroup/Vulkan-Headers"
                    description: qsTr(
                                     "Khronos cross-platform low-overhead GPU compute and graphics headers enabling Vulkan tensor acceleration for whisper.cpp.")
                }
            }

            StyledCard {
                Layout.fillWidth: true

                StyledText {
                    text: qsTr("Linux & Desktop Integration")
                    variant: "subheading"
                    Layout.bottomMargin: Theme.spacingXs
                }

                ComponentItem {
                    title: qsTr("wl-clipboard")
                    badgeText: qsTr("GPL-3.0-or-later")
                    url: "https://github.com/bugaevc/wl-clipboard"
                    urlLabel: "bugaevc/wl-clipboard"
                    description: qsTr(
                                     "Wayland clipboard copy/paste utilities and data-control protocol client enabling clipboard synchronization.")
                }

                StyledDivider {}

                ComponentItem {
                    title: qsTr("libevdev")
                    badgeText: qsTr("MIT")
                    url: "https://gitlab.freedesktop.org/libevdev/libevdev"
                    urlLabel: "freedesktop/libevdev"
                    description: qsTr(
                                     "Linux kernel event device library used by keyinjectord for virtual uinput device management and simulated keypress injection.")
                }

                StyledDivider {}

                ComponentItem {
                    title: qsTr("libcap")
                    badgeText: qsTr("BSD-3-Clause / GPL-2.0")
                    url: "https://git.kernel.org/pub/scm/libs/libcap/libcap.git"
                    urlLabel: "kernel.org/libcap"
                    description: qsTr(
                                     "Linux POSIX capability library enabling keyinjectord to drop root privileges while retaining CAP_SETUID and CAP_SETGID for sandboxed operation.")
                }
            }

            StyledCard {
                Layout.fillWidth: true

                StyledText {
                    text: qsTr("Artwork & Icons")
                    variant: "subheading"
                    Layout.bottomMargin: Theme.spacingXs
                }

                ComponentItem {
                    title: qsTr("Font Awesome Free")
                    badgeText: qsTr("CC BY 4.0 / SIL OFL")
                    url: "https://github.com/FortAwesome/Font-Awesome"
                    urlLabel: "FortAwesome/Font-Awesome"
                    description: qsTr("Vector icon assets for UI controls and action buttons.")
                }

                StyledDivider {}

                ComponentItem {
                    title: qsTr("Speech to Text Icon")
                    badgeText: qsTr("Flaticon License")
                    url: "https://www.flaticon.com/free-icons/speech-to-text"
                    urlLabel: "flaticon.com"
                    description: qsTr("Application branding icon created by Fajrul Fitrianto on Flaticon.")
                }
            }
        }
    }
}
