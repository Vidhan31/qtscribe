pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import QtScribe
import "../controls"

StyledCard {
    id: root

    property string fieldLabel: qsTr("Custom Vocabulary")
    property string placeholderText: ""
    property string vocabulary: ""
    property int maxCharacters: 0
    property int debounceInterval: 400

    signal vocabularySaved(string text)
    signal vocabularyCleared

    PreferenceRow {
        layoutVertical: true

        RowLayout {
            Layout.fillWidth: true

            StyledText {
                text: root.fieldLabel
                variant: "body"
                customWeight: Font.Medium
            }

            Item {
                Layout.fillWidth: true
            }

            StyledText {
                text: root.maxCharacters > 0 ? qsTr("%1 / %2 characters").arg(vocabArea.text.length).arg(root.maxCharacters) :
                                               qsTr("%1 characters").arg(vocabArea.text.length)
                variant: "caption"
                customColor: root.maxCharacters > 0 && vocabArea.text.length > (root.maxCharacters - 100)
                             ? Theme.colorWarning : Theme.textSecondary
            }
        }

        T.ScrollView {
            Layout.fillWidth: true
            implicitHeight: 85
            clip: true

            T.TextArea {
                id: vocabArea
                text: root.vocabulary
                placeholderText: root.placeholderText
                placeholderTextColor: Theme.textPlaceholder
                selectByMouse: true
                wrapMode: TextArea.Wrap
                font.pixelSize: Theme.fontSizeBody
                color: Theme.textPrimary
                leftPadding: Theme.spacingSm + 2
                rightPadding: Theme.spacingSm + 2
                topPadding: Theme.spacingSm
                bottomPadding: Theme.spacingSm

                background: Rectangle {
                    color: Theme.inputBg
                    border.color: vocabArea.activeFocus ? Theme.focusRingColor : Theme.inputBorder
                    border.width: vocabArea.activeFocus ? Theme.focusRingWidth : 1
                    radius: Theme.radiusSm
                }

                onTextChanged: {
                    if (activeFocus) {
                        saveVocabTimer.restart();
                    }
                }
            }
        }

        Timer {
            id: saveVocabTimer
            interval: root.debounceInterval
            repeat: false
            onTriggered: {
                root.vocabularySaved(vocabArea.text);
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSm

            Item {
                Layout.fillWidth: true
            }

            StyledButton {
                id: clearVocabBtn
                text: qsTr("Clear")
                iconSource: "qrc:/qt/qml/QtScribe/assets/icons/trash.svg"
                enabled: vocabArea.text.length > 0
                size: "small"
                onClicked: {
                    vocabArea.text = "";
                    root.vocabularyCleared();
                    root.vocabularySaved("");
                }
            }

            StyledButton {
                id: saveVocabBtn
                text: qsTr("Save")
                iconSource: "qrc:/qt/qml/QtScribe/assets/icons/save.svg"
                size: "small"
                variant: "primary"
                onClicked: {
                    saveVocabTimer.stop();
                    root.vocabularySaved(vocabArea.text);
                }
            }
        }
    }
}
