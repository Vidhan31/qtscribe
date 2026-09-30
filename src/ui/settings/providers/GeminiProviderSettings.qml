pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtScribe
import "../../controls"
import ".."

Item {
    id: root

    implicitWidth: 500
    implicitHeight: contentColumn.implicitHeight

    ListModel {
        id: geminiModelsModel
        ListElement {
            name: "Gemini 3.5 Transcribe (Default)"
            code: "gemini-3.5-transcribe"
        }
    }

    ListModel {
        id: transcriptionModesModel
        ListElement {
            name: "Smart Transcription (Auto-formatting & cleanup)"
            code: "smart"
        }
        ListElement {
            name: "Verbatim Mode (Exact word-for-word)"
            code: "verbatim"
        }
    }

    ColumnLayout {
        id: contentColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: Theme.spacingLg

        ApiKeyConfigCard {
            title: qsTr("Gemini API Key")
            description: qsTr("Required for Gemini speech recognition (Free tier available at Google AI Studio)")
            rowDescription: qsTr("Enter your Google Gemini API key")
            apiKey: GeminiSttClient.apiKey
            placeholderText: qsTr("AIzaSy...")
            helpText: qsTr(
                          "API keys available from Google AI Studio at aistudio.google.com. Stored securely in system keychain (with configuration fallback).")
            linkButtonText: qsTr("Get API Key")
            linkUrl: "https://aistudio.google.com/app/apikey"
            onSaveRequested: key => {
                GeminiSttClient.apiKey = key;
            }
        }

        StyledCard {
            title: qsTr("Speech Model & Mode")
            description: qsTr("Configure the Gemini model and transcription mode")

            PreferenceRow {
                title: qsTr("Model")
                description: qsTr("Gemini 3.5 Transcribe is optimized for speech-to-text with high accuracy.")
                layoutVertical: true

                StyledComboBox {
                    id: modelCombo
                    Layout.fillWidth: true
                    textRole: "name"
                    valueRole: "code"
                    model: geminiModelsModel
                    currentIndex: {
                        const idx = indexOfValue(GeminiSttClient.selectedModel);
                        return idx >= 0 ? idx : 0;
                    }

                    onActivated: index => {
                        GeminiSttClient.selectedModel = currentValue;
                    }
                }
            }

            StyledDivider {}

            PreferenceRow {
                title: qsTr("Transcription Mode")
                description: qsTr(
                                 "Smart mode removes filler words and structures output. Verbatim provides word-for-word accuracy.")
                layoutVertical: true

                StyledComboBox {
                    id: modeCombo
                    Layout.fillWidth: true
                    textRole: "name"
                    valueRole: "code"
                    model: transcriptionModesModel
                    currentIndex: {
                        const idx = indexOfValue(GeminiSttClient.mode);
                        return idx >= 0 ? idx : 0;
                    }

                    onActivated: index => {
                        GeminiSttClient.mode = currentValue;
                    }
                }
            }
        }

        CustomVocabularyCard {
            title: qsTr("Custom Vocabulary")
            description: qsTr("Provide specialized terminology to bias recognition")
            fieldLabel: qsTr("Custom Vocabulary")
            placeholderText: qsTr(
                                 "Comma-separated or newline-separated terms, e.g. QtScribe, Wayland, Qt6, CMake, Kubernetes, Gemini...")
            vocabulary: GeminiSttClient.customVocabulary
            maxCharacters: 0
            onVocabularySaved: text => {
                GeminiSttClient.customVocabulary = text;
            }
            onVocabularyCleared: {
                GeminiSttClient.customVocabulary = "";
            }
        }
    }
}
