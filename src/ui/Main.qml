pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import QtScribe
import "controls"
import "settings"

T.ApplicationWindow {
    id: root
    width: 960
    height: 660
    minimumWidth: 760
    minimumHeight: 520
    visible: false
    title: qsTr("QtScribe")
    color: Theme.windowBg

    property string currentSectionId: "dictate"

    function sectionToViewIndex(section: string): int {
        switch (section) {
        case "dictate":
            return 0;
        case "history":
            return 1;
        case "cloud":
        case "online":
            return 2;
        case "offline":
            return 3;
        case "system":
            return 4;
        case "about":
            return 5;
        case "license":
            return 6;
        default:
            return 0;
        }
    }

    function normalizeSectionId(target: string): string {
        switch (target) {
        case "apiKey":
        case "enhancement":
            return "cloud";
        case "dictation":
            return DictationCoordinator.activeBackend === DictationCoordinator.Cloud ? "cloud" : "offline";
        default:
            return target;
        }
    }

    function navigateToSection(target: string) {
        const normalized = normalizeSectionId(target);
        if (normalized.length > 0) {
            root.currentSectionId = normalized;
        }
    }

    property bool isQuitting: false

    function quitApplication() {
        root.isQuitting = true;
        Qt.quit();
    }

    onClosing: close => {
        if (!root.isQuitting) {
            close.accepted = false;
            root.hide();
        }
    }

    Connections {
        target: DictationCoordinator
        function onRequestShowWindow() {
            root.show();
            root.raise();
            root.requestActivate();
        }
        function onRequestNavigate(section: string) {
            root.show();
            root.raise();
            root.requestActivate();
            root.navigateToSection(section);
        }
        function onRequestQuitApp() {
            root.quitApplication();
        }
        function onActiveBackendChanged() {
            if (DictationCoordinator.activeBackend === DictationCoordinator.Cloud) {
                if (root.currentSectionId === "offline") {
                    root.currentSectionId = "cloud";
                }
            } else {
                if (root.currentSectionId === "cloud" || root.currentSectionId === "online") {
                    root.currentSectionId = "offline";
                }
            }
        }
    }

    NavigationModel {
        id: navModel
        isCloud: DictationCoordinator.activeBackend === DictationCoordinator.Cloud
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: root.width < 840 ? 180 : Theme.sidebarWidth
            Layout.fillHeight: true
            color: Theme.sidebarBg

            Behavior on Layout.preferredWidth {
                NumberAnimation {
                    duration: Theme.animFast
                    easing.type: Easing.OutCubic
                }
            }

            StyledDivider {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                orientation: Qt.Vertical
                dividerColor: Theme.sidebarBorder
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingSm
                spacing: Theme.spacingSm

                ListView {
                    id: navListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.topMargin: Theme.spacingXs
                    clip: true
                    spacing: 2
                    model: navModel

                    delegate: Item {
                        id: navDelegate
                        required property string section
                        required property string title
                        required property string iconSource
                        required property string sectionId
                        required property bool isFirstInSection
                        required property int index

                        readonly property bool isSelected: root.currentSectionId === navDelegate.sectionId

                        width: navListView.width
                        implicitHeight: (navDelegate.isFirstInSection ? 32 : 0) + 34

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 2

                            StyledText {
                                text: navDelegate.section
                                variant: "small"
                                customWeight: Font.Bold
                                colorRole: "tertiary"
                                visible: navDelegate.isFirstInSection
                                Layout.leftMargin: Theme.spacingSm
                                Layout.topMargin: navDelegate.index === 0 ? 4 : Theme.spacingMd
                                Layout.bottomMargin: 4
                            }

                            Rectangle {
                                id: itemPill
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                radius: Theme.radiusSm
                                color: {
                                    if (navDelegate.isSelected) {
                                        return Theme.sidebarItemSelected;
                                    }
                                    if (navMouse.containsMouse) {
                                        return Theme.sidebarItemHover;
                                    }
                                    return "transparent";
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: Theme.spacingSm + 2
                                    anchors.rightMargin: Theme.spacingSm
                                    spacing: Theme.spacingSm

                                    StyledIcon {
                                        source: navDelegate.iconSource
                                        size: 16
                                        color: navDelegate.isSelected ? Theme.accentColor : (navMouse.containsMouse
                                                                                             ? Theme.textPrimary :
                                                                                               Theme.textSecondary)
                                        Layout.alignment: Qt.AlignVCenter
                                    }

                                    StyledText {
                                        text: navDelegate.title
                                        variant: "body"
                                        customWeight: navDelegate.isSelected ? Font.DemiBold : Font.Normal
                                        customColor: navDelegate.isSelected ? Theme.accentColor : (
                                                                                  navMouse.containsMouse
                                                                                  ? Theme.textPrimary :
                                                                                    Theme.textSecondary)
                                        verticalAlignment: Text.AlignVCenter
                                        Layout.alignment: Qt.AlignVCenter
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                }

                                MouseArea {
                                    id: navMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        root.navigateToSection(navDelegate.sectionId);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            StackLayout {
                id: viewStack
                anchors.fill: parent
                anchors.margins: Theme.spacingLg
                currentIndex: root.sectionToViewIndex(root.currentSectionId)

                onCurrentIndexChanged: {
                    viewTransitionAnim.restart();
                }

                SpeechPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    onNavigateRequested: target => {
                        root.navigateToSection(target);
                    }
                }

                HistoryView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                ScrollView {
                    id: cloudScrollView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    CloudSettingsPage {
                        width: cloudScrollView.availableWidth
                    }
                }

                ScrollView {
                    id: offlineScrollView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    OfflineSettingsPage {
                        width: offlineScrollView.availableWidth
                    }
                }

                ScrollView {
                    id: systemScrollView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    SystemSettingsPage {
                        width: systemScrollView.availableWidth
                    }
                }

                CreditsView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                LicenseView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }

            OpacityAnimator {
                id: viewTransitionAnim
                target: viewStack
                from: 0.4
                to: 1.0
                duration: Theme.animNormal
                easing.type: Easing.OutCubic
            }
        }
    }
}
