pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import QtScribe
import "controls"

StyledCard {
    id: root

    signal clipboardWarningRequested
    signal shortcutGuideRequested

    customRadius: Theme.radiusMd
    customBorderColor: DaemonClient.recording ? Theme.colorDanger : Theme.cardBorder
    customBorderWidth: DaemonClient.recording ? 2 : 1
    padding: Theme.spacingLg

    Behavior on customBorderColor {
        ColorAnimation {
            duration: Theme.animNormal
        }
    }

    Item {
        Layout.fillWidth: true
        implicitHeight: 96

        Item {
            anchors.centerIn: parent
            width: 96
            height: 96

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: Theme.colorDanger
                opacity: DaemonClient.recording ? 0.25 : 0.0
                visible: DaemonClient.recording

                Behavior on opacity {
                    NumberAnimation {
                        duration: Theme.animNormal
                    }
                }

                SequentialAnimation on scale {
                    running: DaemonClient.recording
                    loops: Animation.Infinite
                    ScaleAnimator {
                        to: 1.18
                        duration: 650
                    }
                    ScaleAnimator {
                        to: 1.0
                        duration: 650
                    }
                }
            }

            T.Button {
                id: recordBtn
                anchors.fill: parent
                activeFocusOnTab: true
                Accessible.role: Accessible.Button
                Accessible.name: DaemonClient.recording ? qsTr("Stop Dictation") : (DaemonClient.recordingMode
                                                                                    === DaemonClient.PushToTalk ? qsTr(
                                                                                                                      "Push to Talk") :
                                                                                                                  qsTr("Start Dictation"))
                Accessible.description: DaemonClient.recordingMode === DaemonClient.PushToTalk ? qsTr(
                                                                                                     "Push to talk speech recognition") :
                                                                                                 qsTr("Toggle speech recognition recording")
                enabled: DaemonClient.canRecord

                background: Rectangle {
                    radius: recordBtn.width / 2
                    color: {
                        if (!recordBtn.enabled)
                        return Theme.controlBg;
                        if (DaemonClient.recording)
                        return recordBtn.down ? Theme.buttonDangerBgPressed : (recordBtn.hovered
                                                                               ? Theme.buttonDangerBgHover :
                                                                                 Theme.buttonDangerBg);
                        if (DaemonClient.isBusy)
                        return Theme.colorWarning;
                        return recordBtn.down ? Theme.buttonPrimaryBgPressed : (recordBtn.hovered
                                                                                ? Theme.buttonPrimaryBgHover :
                                                                                  Theme.buttonPrimaryBg);
                    }
                    border.color: recordBtn.visualFocus ? Theme.focusRingColor : Theme.cardBorder
                    border.width: recordBtn.visualFocus ? Theme.focusRingWidth : 1

                    Behavior on color {
                        ColorAnimation {
                            duration: Theme.animFast
                        }
                    }
                }

                contentItem: ColumnLayout {
                    spacing: 2
                    anchors.centerIn: parent

                    Item {
                        Layout.alignment: Qt.AlignHCenter
                        implicitWidth: 28
                        implicitHeight: 28

                        Image {
                            anchors.centerIn: parent
                            width: 24
                            height: 24
                            source: "qrc:/qt/qml/QtScribe/assets/icons/mic.svg"
                            sourceSize.width: 24
                            sourceSize.height: 24
                            visible: !DaemonClient.recording && !DaemonClient.isBusy
                            smooth: true
                        }

                        Image {
                            anchors.centerIn: parent
                            width: 22
                            height: 22
                            source: "qrc:/qt/qml/QtScribe/assets/icons/stop.svg"
                            sourceSize.width: 22
                            sourceSize.height: 22
                            visible: DaemonClient.recording
                            smooth: true
                        }

                        Image {
                            anchors.centerIn: parent
                            width: 24
                            height: 24
                            source: "qrc:/qt/qml/QtScribe/assets/icons/spinner.svg"
                            sourceSize.width: 24
                            sourceSize.height: 24
                            visible: DaemonClient.isBusy
                            smooth: true

                            RotationAnimation on rotation {
                                running: DaemonClient.isBusy
                                loops: Animation.Infinite
                                from: 0
                                to: 360
                                duration: 1000
                            }
                        }
                    }

                    StyledText {
                        text: DaemonClient.recording ? qsTr("Stop") : (DaemonClient.isBusy ? qsTr("Processing…") : (
                                                                                                 DaemonClient.recordingMode
                                                                                                 === DaemonClient.PushToTalk
                                                                                                 ? qsTr("Talk") : qsTr(
                                                                                                       "Record")))
                        variant: "caption"
                        customWeight: Font.DemiBold
                        colorRole: "onAccent"
                        Layout.alignment: Qt.AlignHCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                onClicked: {
                    if (!DaemonClient.recording && DaemonDiagnosticModel.clipboardWarningRequired) {
                        root.clipboardWarningRequested();
                    } else {
                        DaemonClient.toggleRecording();
                    }
                }
            }
        }
    }

    Item {
        Layout.fillWidth: true
        implicitHeight: 32
        visible: DaemonClient.recording

        RowLayout {
            anchors.centerIn: parent
            spacing: 5

            Repeater {
                model: 7
                delegate: Item {
                    id: waveformBar
                    required property int index
                    Layout.preferredWidth: 4
                    Layout.preferredHeight: 28

                    readonly property real multiplier: {
                        switch (index) {
                            case 0:
                            case 6:
                            return 0.5;
                            case 1:
                            case 5:
                            return 0.8;
                            case 2:
                            case 4:
                            return 1.1;
                            default:
                            return 1.4;
                        }
                    }

                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 4
                        height: Math.max(4, 28 * Math.min(DaemonClient.audioLevel * waveformBar.multiplier, 1.0))

                        radius: Theme.radiusXs / 2
                        color: Theme.colorSuccess

                        Behavior on height {
                            NumberAnimation {
                                duration: 50
                            }
                        }
                    }
                }
            }
        }
    }

    StyledText {
        id: audioStatusText
        text: DaemonClient.statusMessage
        variant: "caption"
        colorRole: "secondary"
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        visible: audioStatusText.text.length > 0
    }

    StyledText {
        id: modeHintText
        text: DaemonClient.recordingMode === DaemonClient.PushToTalk ? qsTr(
                                                                           "Push-to-Talk: Hold Ctrl+Shift+Space to talk") :
                                                                       qsTr("Toggle Mode: Press Ctrl+Shift+Space to start / stop")
        variant: "caption"
        colorRole: "secondary"
        horizontalAlignment: Text.AlignHCenter
        Layout.fillWidth: true
        visible: !DaemonClient.recording && !DaemonClient.isBusy
    }

    SystemStatusFooter {
        onShortcutGuideRequested: root.shortcutGuideRequested()
    }
}
