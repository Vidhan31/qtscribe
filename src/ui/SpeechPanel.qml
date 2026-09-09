pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import QtScribe
import "controls"

Item {
    id: root

    signal navigateRequested(string target)

    implicitWidth: 620
    implicitHeight: 540

    property bool showCopySuccess: false

    Timer {
        id: copyFeedbackTimer
        interval: 2000
        repeat: false
        onTriggered: {
            root.showCopySuccess = false;
        }
    }

    ShortcutGuideDialog {
        id: shortcutGuideDialog
    }

    ClipboardWarningDialog {
        id: clipboardWarningDialog
        onAccepted: {
            DictationCoordinator.startRecording();
        }
    }

    T.ScrollView {
        id: speechScrollView
        anchors.fill: parent
        clip: true

        ColumnLayout {
            width: speechScrollView.availableWidth
            spacing: Theme.spacingMd

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spacingXs
                Layout.bottomMargin: Theme.spacingSm
                spacing: 4

                StyledText {
                    text: qsTr("Dictate")
                    variant: "heading"
                }

                StyledText {
                    text: qsTr("Real-time voice dictation and transcription pad")
                    variant: "caption"
                    colorRole: "secondary"
                }
            }

            SystemDiagnosticsColumn {
                Layout.fillWidth: true
                onNavigateRequested: target => root.navigateRequested(target)
                onClipboardWarningRequested: clipboardWarningDialog.open()
            }

            RecordingControl {
                Layout.fillWidth: true
                onClipboardWarningRequested: clipboardWarningDialog.open()
                onShortcutGuideRequested: shortcutGuideDialog.open()
            }

            StyledCard {
                Layout.fillWidth: true
                customRadius: Theme.radiusMd
                padding: Theme.spacingMd

                RowLayout {
                    Layout.fillWidth: true

                    StyledText {
                        text: qsTr("Dictation Pad")
                        variant: "body"
                        customWeight: Font.DemiBold
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    StyledText {
                        text: qsTr("%1 words • %2 characters").arg(DictationPadModel.wordCount).arg(
                                  DictationPadModel.charCount)
                        variant: "caption"
                        colorRole: "secondary"
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    implicitHeight: 96
                    clip: true

                    T.TextArea {
                        id: testingGroundText
                        text: DictationPadModel.text
                        placeholderText: qsTr("Text will appear here when dictating…")
                        placeholderTextColor: Theme.textPlaceholder
                        font.pixelSize: Theme.fontSizeBody
                        wrapMode: Text.WordWrap
                        selectByMouse: true
                        color: Theme.textPrimary
                        leftPadding: Theme.spacingSm + 2
                        rightPadding: Theme.spacingSm + 2
                        topPadding: Theme.spacingSm
                        bottomPadding: Theme.spacingSm

                        background: Rectangle {
                            color: Theme.inputBg
                            border.color: testingGroundText.activeFocus ? Theme.focusRingColor : Theme.inputBorder
                            border.width: testingGroundText.activeFocus ? Theme.focusRingWidth : 1
                            radius: Theme.radiusSm
                        }

                        onTextChanged: {
                            if (activeFocus) {
                                DictationPadModel.text = text;
                            } else {
                                cursorPosition = text.length;
                            }
                        }
                    }
                }

                Connections {
                    target: DictationPadModel
                    function onTextChanged() {
                        testingGroundText.cursorPosition = testingGroundText.text.length;
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    StyledButton {
                        id: copyPadBtn
                        text: root.showCopySuccess ? qsTr("Copied!") : qsTr("Copy")
                        iconSource: root.showCopySuccess ? "qrc:/qt/qml/QtScribe/assets/icons/check.svg" :
                                                           "qrc:/qt/qml/QtScribe/assets/icons/copy.svg"
                        enabled: DictationPadModel.charCount > 0
                        size: "small"
                        variant: root.showCopySuccess ? "primary" : "secondary"
                        onClicked: {
                            DictationPadModel.copyToClipboard();
                            root.showCopySuccess = true;
                            copyFeedbackTimer.restart();
                        }
                    }

                    StyledButton {
                        id: clearPadBtn
                        text: qsTr("Clear")
                        iconSource: "qrc:/qt/qml/QtScribe/assets/icons/trash.svg"
                        enabled: DictationPadModel.charCount > 0
                        size: "small"
                        onClicked: {
                            DictationPadModel.clear();
                        }
                    }
                }
            }
        }
    }
}
