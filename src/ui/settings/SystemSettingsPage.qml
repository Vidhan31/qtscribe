pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import QtScribe
import "../controls"
import ".."

Item {
    id: root

    implicitWidth: 500
    implicitHeight: contentColumn.implicitHeight

    ListModel {
        id: themeOptionsModel
        ListElement {
            name: "System (Auto)"
            code: "system"
        }
        ListElement {
            name: "Dark Theme"
            code: "dark"
        }
        ListElement {
            name: "Light Theme"
            code: "light"
        }
    }

    ColumnLayout {
        id: contentColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: Theme.spacingLg

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingXs
            Layout.bottomMargin: Theme.spacingSm
            spacing: 4

            StyledText {
                text: qsTr("System & Audio")
                variant: "heading"
            }

            StyledText {
                text: qsTr("Configure appearance, recording mode, feedback sounds, direct typing, and global shortcuts")
                variant: "caption"
                colorRole: "secondary"
            }
        }

        StyledCard {
            title: qsTr("Appearance")
            description: qsTr("Choose your preferred theme or follow your desktop environment")

            PreferenceRow {
                title: qsTr("Theme")
                description: qsTr("System detected: %1 mode").arg(Theme.systemThemeName)
                layoutVertical: false

                StyledComboBox {
                    id: themeCombo
                    Layout.preferredWidth: 180
                    textRole: "name"
                    valueRole: "code"
                    model: themeOptionsModel
                    currentIndex: {
                        const idx = indexOfValue(Theme.themeMode);
                        return idx >= 0 ? idx : 0;
                    }

                    onActivated: index => {
                        Theme.themeMode = currentValue;
                    }
                }
            }
        }

        StyledCard {
            title: qsTr("Recording Mode")
            description: qsTr("Choose how global shortcuts and recording triggers activate speech transcription")

            StatusBanner {
                visible: !SystemHealthMonitor.pushToTalkSupported
                bannerType: "warning"
                title: qsTr("Push-to-Talk Unavailable on Current Desktop")
                message: qsTr(
                             "Push-to-Talk requires key press and release tracking via the XDG Global Shortcuts portal (org.freedesktop.portal.GlobalShortcuts). Your desktop environment does not support this portal, so dictation is restricted to Toggle mode.")
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.width >= 560 ? 2 : 1
                columnSpacing: Theme.spacingMd
                rowSpacing: Theme.spacingMd

                Rectangle {
                    id: toggleModeCard
                    Layout.fillWidth: true
                    Layout.preferredHeight: toggleContent.implicitHeight + (Theme.spacingMd * 2)
                    radius: Theme.radiusMd
                    color: isSelected ? Theme.selectedBg : (toggleMouse.containsMouse ? Theme.cardBgElevated :
                                                                                        Theme.cardBgSubtle)

                    border.color: isSelected ? Theme.accentColor : (toggleMouse.containsMouse ? Theme.cardBorderHover :
                                                                                                Theme.cardBorder)
                    border.width: isSelected ? 2 : 1

                    readonly property bool isSelected: DictationCoordinator.recordingMode
                                                       === DictationCoordinator.Toggle

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
                        id: toggleContent
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
                                color: toggleModeCard.isSelected ? Theme.sidebarItemSelected : Theme.controlBg

                                StyledIcon {
                                    anchors.centerIn: parent
                                    source: "qrc:/qt/qml/QtScribe/assets/icons/speech.svg"
                                    size: 16
                                    color: toggleModeCard.isSelected ? Theme.accentColor : Theme.textSecondary
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1

                                StyledText {
                                    text: qsTr("Toggle Mode")
                                    variant: "body"
                                    customWeight: Font.DemiBold
                                }

                                StyledText {
                                    text: qsTr("Press to start / stop")
                                    variant: "caption"
                                    colorRole: "secondary"
                                }
                            }

                            StateBadge {
                                text: qsTr("Active")
                                statusType: "accent"
                                visible: toggleModeCard.isSelected
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
                                text: qsTr("• Press shortcut or button once to start recording")
                                variant: "caption"
                                colorRole: "secondary"
                            }

                            StyledText {
                                text: qsTr("• Press again to stop recording and transcribe")
                                variant: "caption"
                                colorRole: "secondary"
                            }

                            StyledText {
                                text: qsTr("• Compatible with all desktop environments")
                                variant: "caption"
                                colorRole: "secondary"
                            }
                        }
                    }

                    MouseArea {
                        id: toggleMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            DictationCoordinator.recordingMode = DictationCoordinator.Toggle;
                        }
                    }
                }

                Rectangle {
                    id: pttModeCard
                    Layout.fillWidth: true
                    Layout.preferredHeight: pttContent.implicitHeight + (Theme.spacingMd * 2)
                    radius: Theme.radiusMd
                    opacity: SystemHealthMonitor.pushToTalkSupported ? 1.0 : 0.6
                    color: isSelected ? Theme.selectedBg : (pttMouse.containsMouse
                                                            && SystemHealthMonitor.pushToTalkSupported
                                                            ? Theme.cardBgElevated : Theme.cardBgSubtle)
                    border.color: isSelected ? Theme.accentColor : (pttMouse.containsMouse
                                                                    && SystemHealthMonitor.pushToTalkSupported
                                                                    ? Theme.cardBorderHover : Theme.cardBorder)
                    border.width: isSelected ? 2 : 1

                    readonly property bool isSelected: DictationCoordinator.recordingMode
                                                       === DictationCoordinator.PushToTalk

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
                        id: pttContent
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
                                color: pttModeCard.isSelected ? Theme.sidebarItemSelected : Theme.controlBg

                                StyledIcon {
                                    anchors.centerIn: parent
                                    source: "qrc:/qt/qml/QtScribe/assets/icons/mic.svg"
                                    size: 16
                                    color: pttModeCard.isSelected ? Theme.accentColor : Theme.textSecondary
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1

                                StyledText {
                                    text: qsTr("Push-to-Talk")
                                    variant: "body"
                                    customWeight: Font.DemiBold
                                }

                                StyledText {
                                    text: qsTr("Hold shortcut to speak")
                                    variant: "caption"
                                    colorRole: "secondary"
                                }
                            }

                            StateBadge {
                                text: qsTr("Active")
                                statusType: "accent"
                                visible: pttModeCard.isSelected
                                showDot: true
                            }

                            StateBadge {
                                text: qsTr("Unavailable")
                                statusType: "warning"
                                visible: !SystemHealthMonitor.pushToTalkSupported
                                showDot: false
                            }
                        }

                        StyledDivider {
                            dividerColor: Theme.cardBorder
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            StyledText {
                                text: qsTr("• Hold shortcut down while speaking")
                                variant: "caption"
                                colorRole: "secondary"
                            }

                            StyledText {
                                text: qsTr("• Release shortcut to immediately transcribe")
                                variant: "caption"
                                colorRole: "secondary"
                            }

                            StyledText {
                                text: qsTr("• Requires XDG Global Shortcuts portal support")
                                variant: "caption"
                                colorRole: "secondary"
                            }
                        }
                    }

                    MouseArea {
                        id: pttMouse
                        anchors.fill: parent
                        hoverEnabled: SystemHealthMonitor.pushToTalkSupported
                        cursorShape: SystemHealthMonitor.pushToTalkSupported ? Qt.PointingHandCursor : Qt.ArrowCursor
                        enabled: SystemHealthMonitor.pushToTalkSupported
                        onClicked: {
                            if (SystemHealthMonitor.pushToTalkSupported) {
                                DictationCoordinator.recordingMode = DictationCoordinator.PushToTalk;
                            }
                        }
                    }
                }
            }
        }

        StyledCard {
            title: qsTr("Sound Effects")
            description: qsTr("Play sounds when dictation starts and stops")

            PreferenceSwitch {
                id: soundSwitch
                title: qsTr("Feedback Sounds")
                description: qsTr("Play sound effects on recording start and completion")
                checked: AudioFeedbackPlayer.soundEnabled
                onToggled: {
                    AudioFeedbackPlayer.soundEnabled = soundSwitch.checked;
                }
            }

            StyledDivider {}

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm
                opacity: AudioFeedbackPlayer.soundEnabled ? 1.0 : 0.5
                enabled: AudioFeedbackPlayer.soundEnabled

                StyledText {
                    text: qsTr("Preview:")
                    variant: "caption"
                    colorRole: "secondary"
                }

                Item {
                    Layout.fillWidth: true
                }

                StyledButton {
                    id: playStartBtn
                    text: qsTr("Play Start Sound")
                    size: "small"
                    onClicked: AudioFeedbackPlayer.playStartSound()
                }

                StyledButton {
                    id: playStopBtn
                    text: qsTr("Play Stop Sound")
                    size: "small"
                    onClicked: AudioFeedbackPlayer.playStopSound()
                }
            }
        }

        StyledCard {
            title: qsTr("Desktop Notifications")
            description: qsTr("System notifications for background dictation, errors, and model downloads")

            PreferenceSwitch {
                id: notificationSwitch
                title: qsTr("Desktop Notifications")
                description: qsTr("Show system notifications for errors, safety limits, and download events")
                checked: LinuxNotificationService.enabled
                onToggled: {
                    LinuxNotificationService.enabled = notificationSwitch.checked;
                }
            }
        }

        StyledCard {
            title: qsTr("Microphone Test")
            description: qsTr("Check microphone input level and audio capture")

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingMd

                    StyledText {
                        text: qsTr("Input Level:")
                        variant: "body"
                        customWeight: Font.Medium
                    }

                    T.ProgressBar {
                        Layout.fillWidth: true
                        implicitHeight: 8
                        from: 0.0
                        to: 1.0
                        value: Math.min(1.0, Math.max(0.0, AudioRecorder.audioLevel))

                        background: Rectangle {
                            radius: Theme.radiusCircle
                            color: Theme.controlBg
                            border.color: Theme.controlBorder
                            border.width: 1
                        }

                        contentItem: Item {
                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: parent.width * Math.min(1.0, Math.max(0.0, AudioRecorder.audioLevel))
                                radius: Theme.radiusCircle

                                color: {
                                    if (AudioRecorder.audioLevel > 0.85)
                                    return Theme.colorDanger;
                                    if (AudioRecorder.audioLevel > 0.6)
                                    return Theme.colorWarning;
                                    return Theme.colorSuccess;
                                }

                                Behavior on width {
                                    NumberAnimation {
                                        duration: 60
                                        easing.type: Easing.OutQuad
                                    }
                                }
                            }
                        }
                    }

                    StyledText {
                        text: qsTr("%1%").arg(Math.round(AudioRecorder.audioLevel * 100))
                        variant: "caption"
                        customWeight: Font.Bold
                        Layout.preferredWidth: 36
                        horizontalAlignment: Text.AlignRight
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    StyledText {
                        text: AudioRecorder.recording ? qsTr("Recording active — speak into microphone") : qsTr(
                                                            "Microphone idle")
                        variant: "caption"
                        customColor: AudioRecorder.recording ? Theme.colorSuccess : Theme.textSecondary
                        Layout.fillWidth: true
                    }

                    StyledButton {
                        id: testMicBtn
                        text: AudioRecorder.recording ? qsTr("Stop Test") : qsTr("Test Microphone")
                        enabled: !DictationCoordinator.isBusy || AudioRecorder.recording
                        size: "small"
                        variant: AudioRecorder.recording ? "danger" : "secondary"
                        onClicked: {
                            if (AudioRecorder.recording) {
                                AudioRecorder.cancelRecording();
                            } else {
                                AudioRecorder.startRecording();
                            }
                        }
                    }
                }
            }
        }

        StyledCard {
            title: qsTr("Direct Typing")
            description: qsTr("Types text directly into the active application")

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        StyledText {
                            text: qsTr("Service Status")
                            variant: "body"
                            customWeight: Font.Medium
                        }

                        StyledText {
                            text: DaemonDiagnosticModel.hasFatalError ? (DaemonDiagnosticModel.fatalErrorMessage.length
                                                                         > 0 ? DaemonDiagnosticModel.fatalErrorMessage :
                                                                               qsTr("Service failed to start")) : (
                                                                            DaemonDiagnosticModel.connected ? qsTr(
                                                                                                                  "Connected to background typing service") :
                                                                                                              (DaemonDiagnosticModel.statusMessage.length
                                                                                                               > 0 ? DaemonDiagnosticModel.statusMessage :
                                                                                                                     qsTr("Not running — using clipboard fallback")))
                            variant: "caption"
                            customColor: DaemonDiagnosticModel.connected ? Theme.colorSuccess : (
                                                                               DaemonDiagnosticModel.hasFatalError
                                                                               ? Theme.colorDanger :
                                                                                 Theme.textSecondary)
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                StyledDivider {}

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    StyledButton {
                        id: testTypingBtn
                        text: qsTr("Test Typing")
                        enabled: !DaemonDiagnosticModel.hasFatalError
                        size: "small"
                        onClicked: {
                            DaemonDiagnosticModel.testTyping();
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    StyledButton {
                        id: restartServiceBtn
                        text: DaemonDiagnosticModel.hasFatalError ? qsTr("Restart Service") : (
                                                                        DaemonDiagnosticModel.connected ? qsTr(
                                                                                                              "Restart Service") :
                                                                                                          qsTr("Connect Service"))
                        variant: "primary"
                        size: "small"
                        onClicked: {
                            DaemonDiagnosticModel.restartService();
                        }
                    }

                    StyledButton {
                        id: stopServiceBtn
                        text: qsTr("Stop Service")
                        enabled: DaemonDiagnosticModel.connected
                        variant: "danger"
                        size: "small"
                        onClicked: {
                            DaemonDiagnosticModel.stopDaemon();
                        }
                    }
                }

                StyledDivider {}

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingMd

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            StyledText {
                                text: qsTr("Pre-Injection Delay")
                                variant: "body"
                                customWeight: Font.Medium
                            }

                            StyledText {
                                text: qsTr("Wait before injecting text into the active application")
                                variant: "caption"
                                colorRole: "secondary"
                            }
                        }

                        StyledText {
                            text: DaemonDiagnosticModel.injectionDelay === 0 ? qsTr("0 ms (Immediate)") : qsTr(
                                                                                   "%1 ms").arg(
                                                                                   DaemonDiagnosticModel.injectionDelay)

                            variant: "body"
                            customWeight: Font.Bold
                            colorRole: "accent"
                            Layout.preferredWidth: 120
                            horizontalAlignment: Text.AlignRight
                        }

                        StyledButton {
                            id: resetDelayBtn
                            text: qsTr("Reset")
                            variant: "flat"
                            size: "small"
                            enabled: DaemonDiagnosticModel.injectionDelay !== 200
                            onClicked: {
                                DaemonDiagnosticModel.injectionDelay = 200;
                            }
                        }
                    }

                    StyledSlider {
                        id: delaySlider
                        Layout.fillWidth: true
                        from: 0
                        to: 2000
                        stepSize: 50
                        value: DaemonDiagnosticModel.injectionDelay
                        onMoved: {
                            DaemonDiagnosticModel.injectionDelay = Math.round(delaySlider.value);
                        }
                    }
                }

                StyledDivider {}

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    StyledIcon {
                        source: "qrc:/qt/qml/QtScribe/assets/icons/keyboard.svg"
                        color: Theme.accentColor
                        size: 16
                        Layout.alignment: Qt.AlignVCenter
                    }

                    StyledText {
                        text: qsTr(
                                  "Injects text using <b>Ctrl+Shift+V</b> keystrokes. Ensure target applications accept Ctrl+Shift+V for pasting.")
                        textFormat: Text.StyledText
                        variant: "caption"
                        colorRole: "secondary"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }
        }

        StyledCard {
            title: qsTr("Clipboard Privacy & Behavior")
            description: qsTr("Control clipboard privacy and text injection behavior on Wayland")

            PreferenceSwitch {
                id: clipboardHistorySwitch
                title: qsTr("Exclude from Clipboard History")
                description: qsTr(
                                 "Prevents clipboard managers (KDE Klipper, GNOME GPaste) from storing your dictated text")
                checked: DaemonDiagnosticModel.preventClipboardHistory
                onToggled: {
                    DaemonDiagnosticModel.preventClipboardHistory = clipboardHistorySwitch.checked;
                }
            }

            StyledDivider {
                visible: DaemonDiagnosticModel.isDevBuild || !DaemonDiagnosticModel.isKde
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm
                visible: DaemonDiagnosticModel.isDevBuild || !DaemonDiagnosticModel.isKde

                StyledText {
                    text: qsTr("Wayland Clipboard Guidance")
                    variant: "body"
                    customWeight: Font.Medium
                }

                StyledText {
                    text: qsTr(
                              "On desktop environments without integrated clipboard history (like GNOME or wlroots compositors), dictation injects text by simulating Ctrl+Shift+V clipboard paste. QtScribe automatically restores copied plain text, but non-text clipboard items (such as images, screenshots, or files) cannot be restored and will be lost unless you paste and save them first.\n\nWe strongly recommend installing or enabling a clipboard manager (such as Klipper on KDE Plasma, Clipboard Indicator or GPaste on GNOME, or cliphist on wlroots/Hyprland) to preserve your media clipboard history. Please also ensure your active applications accept Ctrl+Shift+V for pasting.")
                    variant: "caption"
                    colorRole: "secondary"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        StyledCard {
            title: qsTr("Global Shortcut")
            description: qsTr("System-wide keyboard shortcut to start and stop dictation")

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        StyledText {
                            text: qsTr("Portal Status")
                            variant: "body"
                            customWeight: Font.Medium
                        }

                        StyledText {
                            text: GlobalShortcutManager.statusMessage.length > 0 ? GlobalShortcutManager.statusMessage :
                                                                                   qsTr("Registered via Desktop Portal (Ctrl+Shift+Space)")
                            variant: "caption"
                            customColor: GlobalShortcutManager.available ? Theme.colorSuccess : (
                                                                               GlobalShortcutManager.supported
                                                                               ? Theme.colorWarning :
                                                                                 Theme.textSecondary)
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                StyledDivider {}

                StyledCard {
                    Layout.fillWidth: true
                    variant: "subtle"
                    padding: Theme.spacingMd

                    ColumnLayout {
                        id: customShortcutColumn
                        Layout.fillWidth: true
                        spacing: Theme.spacingSm

                        StyledText {
                            text: qsTr("Desktop Environment Custom Shortcut (CLI)")
                            variant: "caption"
                            customWeight: Font.DemiBold
                        }

                        StyledText {
                            text: qsTr(
                                      "If your desktop does not support portal shortcuts (e.g. GNOME, Sway), bind this command to <b>Ctrl+Shift+Space</b> in your desktop's keyboard settings:")
                            textFormat: Text.StyledText
                            variant: "caption"
                            colorRole: "secondary"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.spacingSm

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 32
                                color: Theme.inputBg
                                border.color: Theme.inputBorder
                                border.width: 1
                                radius: Theme.radiusSm

                                StyledText {
                                    anchors.centerIn: parent
                                    text: "qtscribe --toggle"
                                    fontFamily: "mono"
                                    variant: "caption"
                                    customWeight: Font.Bold
                                    colorRole: "accent"
                                }
                            }

                            StyledButton {
                                id: copyCliBtn
                                text: qsTr("Copy")
                                iconSource: "qrc:/qt/qml/QtScribe/assets/icons/copy.svg"
                                size: "small"
                                onClicked: {
                                    DictationPadModel.copyTextToClipboard("qtscribe --toggle");
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
