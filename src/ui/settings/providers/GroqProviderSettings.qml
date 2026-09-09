pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import QtScribe
import "../../controls"
import ".."

Item {
    id: root

    implicitWidth: 500
    implicitHeight: contentColumn.implicitHeight

    property int activeSubTab: 0

    ListModel {
        id: whisperModelsModel
        ListElement {
            name: "Whisper Turbo (Fastest)"
            code: "whisper-large-v3-turbo"
        }
        ListElement {
            name: "Whisper Large V3 (Accurate)"
            code: "whisper-large-v3"
        }
    }

    ListModel {
        id: languageOptionsModel
        ListElement {
            name: "Auto-detect (Default)"
            code: ""
        }
        ListElement {
            name: "English (en)"
            code: "en"
        }
        ListElement {
            name: "Spanish (es)"
            code: "es"
        }
        ListElement {
            name: "Italian (it)"
            code: "it"
        }
        ListElement {
            name: "German (de)"
            code: "de"
        }
        ListElement {
            name: "Portuguese (pt)"
            code: "pt"
        }
        ListElement {
            name: "French (fr)"
            code: "fr"
        }
        ListElement {
            name: "Japanese (ja)"
            code: "ja"
        }
        ListElement {
            name: "Polish (pl)"
            code: "pl"
        }
        ListElement {
            name: "Dutch (nl)"
            code: "nl"
        }
        ListElement {
            name: "Russian (ru)"
            code: "ru"
        }
        ListElement {
            name: "Korean (ko)"
            code: "ko"
        }
        ListElement {
            name: "Catalan (ca)"
            code: "ca"
        }
        ListElement {
            name: "Turkish (tr)"
            code: "tr"
        }
        ListElement {
            name: "Indonesian (id)"
            code: "id"
        }
        ListElement {
            name: "Vietnamese (vi)"
            code: "vi"
        }
    }

    ListModel {
        id: llmModelOptionsModel
        ListElement {
            name: "GPT-OSS 20B (Recommended)"
            code: "openai/gpt-oss-20b"
        }
        ListElement {
            name: "GPT-OSS 120B (High Depth)"
            code: "openai/gpt-oss-120b"
        }
        ListElement {
            name: "LLaMA 3.3 70B (Versatile)"
            code: "llama-3.3-70b-versatile"
        }
        ListElement {
            name: "Qwen 3.6 27B (Multilingual)"
            code: "qwen/qwen3.6-27b"
        }
        ListElement {
            name: "LLaMA 3.1 8B (Fast)"
            code: "llama-3.1-8b-instant"
        }
    }

    ListModel {
        id: presetOptionsModel
        ListElement {
            name: "Fix Grammar & Typos"
            code: "grammar"
        }
        ListElement {
            name: "Format as Bullet Points"
            code: "bullets"
        }
        ListElement {
            name: "Professional Tone"
            code: "professional"
        }
        ListElement {
            name: "Custom Instructions"
            code: "custom"
        }
    }

    ColumnLayout {
        id: contentColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: Theme.spacingLg

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 40
            radius: Theme.radiusMd
            color: Theme.cardBgSubtle
            border.color: Theme.cardBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 3
                spacing: 4

                Rectangle {
                    id: tab0Btn
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: Theme.radiusSm
                    color: root.activeSubTab === 0 ? Theme.cardBgElevated : (tab0Mouse.containsMouse ? Theme.hoverOverlay :
                                                                                                       "transparent")
                    border.color: root.activeSubTab === 0 ? Theme.cardBorder : "transparent"
                    border.width: root.activeSubTab === 0 ? 1 : 0

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: Theme.spacingSm

                        StyledIcon {
                            source: "qrc:/qt/qml/QtScribe/assets/icons/key.svg"
                            size: 14
                            color: root.activeSubTab === 0 ? Theme.accentColor : Theme.textSecondary
                        }

                        StyledText {
                            text: qsTr("Credentials & Speech Model")
                            variant: "caption"
                            customWeight: root.activeSubTab === 0 ? Font.DemiBold : Font.Normal
                            customColor: root.activeSubTab === 0 ? Theme.accentColor : Theme.textSecondary
                        }
                    }

                    MouseArea {
                        id: tab0Mouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.activeSubTab = 0
                    }
                }

                Rectangle {
                    id: tab1Btn
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: Theme.radiusSm
                    color: root.activeSubTab === 1 ? Theme.cardBgElevated : (tab1Mouse.containsMouse ? Theme.hoverOverlay :
                                                                                                       "transparent")
                    border.color: root.activeSubTab === 1 ? Theme.cardBorder : "transparent"
                    border.width: root.activeSubTab === 1 ? 1 : 0

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: Theme.spacingSm

                        StyledIcon {
                            source: "qrc:/qt/qml/QtScribe/assets/icons/sparkles.svg"
                            size: 14
                            color: root.activeSubTab === 1 ? Theme.accentColor : Theme.textSecondary
                        }

                        StyledText {
                            text: qsTr("Text Enhancement")
                            variant: "caption"
                            customWeight: root.activeSubTab === 1 ? Font.DemiBold : Font.Normal
                            customColor: root.activeSubTab === 1 ? Theme.accentColor : Theme.textSecondary
                        }
                    }

                    MouseArea {
                        id: tab1Mouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.activeSubTab = 1
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingLg
            visible: root.activeSubTab === 0

            ApiKeyConfigCard {
                title: qsTr("Groq API Key")
                description: qsTr(
                                 "Required for cloud speech recognition and LLM text enhancement (Free tier available)")
                rowDescription: qsTr("Enter your secret Groq API key starting with 'gsk_'")
                apiKey: GroqSttClient.apiKey
                placeholderText: qsTr("gsk_...")
                helpText: qsTr(
                              "Free API keys available with generous quotas at console.groq.com. Stored securely in system keychain (with configuration fallback).")
                linkButtonText: qsTr("Get Free API Key")
                linkUrl: "https://console.groq.com/keys"
                onSaveRequested: key => {
                    GroqSttClient.apiKey = key;
                }
            }

            StyledCard {
                title: qsTr("Speech Model & Language")
                description: qsTr("Select the cloud Whisper model and spoken language for recognition")

                PreferenceRow {
                    title: qsTr("Model")
                    description: qsTr("Turbo delivers maximum speed. Large V3 offers highest transcription accuracy.")
                    layoutVertical: true

                    StyledComboBox {
                        id: modelCombo
                        Layout.fillWidth: true
                        textRole: "name"
                        valueRole: "code"
                        model: whisperModelsModel
                        currentIndex: {
                            const idx = indexOfValue(GroqSttClient.selectedModel);
                            return idx >= 0 ? idx : 0;
                        }

                        onActivated: index => {
                            GroqSttClient.selectedModel = currentValue;
                        }
                    }
                }

                StyledDivider {}

                PreferenceRow {
                    title: qsTr("Language")
                    description: qsTr("Specifying language improves recognition speed and accuracy.")
                    layoutVertical: true

                    StyledComboBox {
                        id: langCombo
                        Layout.fillWidth: true
                        textRole: "name"
                        valueRole: "code"
                        model: languageOptionsModel
                        currentIndex: {
                            const idx = indexOfValue(GroqSttClient.language);
                            return idx >= 0 ? idx : 0;
                        }

                        onActivated: index => {
                            GroqSttClient.language = currentValue;
                        }
                    }
                }
            }

            CustomVocabularyCard {
                title: qsTr("Custom Vocabulary")
                description: qsTr("Guide the cloud transcriber with specialized acronyms, names, or terminology")
                fieldLabel: qsTr("Custom Prompt / Words")
                placeholderText: qsTr("e.g. QtScribe, Wayland, Qt6, CMake, JSON, PyTorch, Neovim, kubectl...")
                vocabulary: GroqSttClient.customPrompt
                maxCharacters: 800
                onVocabularySaved: text => {
                    GroqSttClient.customPrompt = text;
                }
                onVocabularyCleared: {
                    GroqSttClient.customPrompt = "";
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingLg
            visible: root.activeSubTab === 1

            StyledCard {
                title: qsTr("Text Enhancement (LLM)")
                description: qsTr(
                                 "Automatically polish, reformat, or correct transcribed text with cloud language models")

                PreferenceSwitch {
                    id: enableSwitch
                    title: qsTr("Enable Text Enhancement")
                    description: qsTr("Apply style formatting and corrections to transcribed text")
                    checked: GroqLlmClient.enabled
                    onToggled: {
                        GroqLlmClient.enabled = enableSwitch.checked;
                    }
                }

                StyledDivider {}

                PreferenceRow {
                    title: qsTr("Enhancement Model")
                    description: qsTr("Cloud language model used for post-processing text")
                    layoutVertical: true
                    opacity: GroqLlmClient.enabled ? 1.0 : 0.5
                    enabled: GroqLlmClient.enabled

                    StyledComboBox {
                        id: llmModelCombo
                        Layout.fillWidth: true
                        textRole: "name"
                        valueRole: "code"
                        model: llmModelOptionsModel
                        currentIndex: {
                            const idx = indexOfValue(GroqLlmClient.selectedModel);
                            return idx >= 0 ? idx : 0;
                        }

                        onActivated: index => {
                            GroqLlmClient.selectedModel = currentValue;
                        }
                    }
                }

                StyledDivider {}

                PreferenceRow {
                    title: qsTr("Style Preset")
                    description: qsTr("Choose how voice transcriptions are structured and polished")
                    layoutVertical: true
                    opacity: GroqLlmClient.enabled ? 1.0 : 0.5
                    enabled: GroqLlmClient.enabled

                    StyledComboBox {
                        id: presetCombo
                        Layout.fillWidth: true
                        textRole: "name"
                        valueRole: "code"
                        model: presetOptionsModel
                        currentIndex: {
                            const idx = indexOfValue(GroqLlmClient.activePreset);
                            return idx >= 0 ? idx : 0;
                        }

                        onActivated: index => {
                            GroqLlmClient.activePreset = currentValue;
                        }
                    }
                }
            }

            StyledCard {
                title: qsTr("Style Precision / Creativity")
                description: qsTr(
                                 "Lower values produce exact corrections; higher values produce more creative rewrites")
                opacity: GroqLlmClient.enabled ? 1.0 : 0.5
                enabled: GroqLlmClient.enabled

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
                                text: qsTr("Precision / Creativity")
                                variant: "body"
                                customWeight: Font.Medium
                            }

                            StyledText {
                                text: qsTr("Exact (0.0) — Creative (1.0)")
                                variant: "caption"
                                colorRole: "secondary"
                            }
                        }

                        StyledText {
                            text: GroqLlmClient.formattedTemperature
                            variant: "body"
                            customWeight: Font.Bold
                            colorRole: "accent"
                            Layout.preferredWidth: 40
                            horizontalAlignment: Text.AlignRight
                        }

                        StyledButton {
                            id: resetTempBtn
                            text: qsTr("Reset")
                            variant: "flat"
                            size: "small"
                            enabled: Math.abs(GroqLlmClient.temperature - 0.1) > 0.01
                            onClicked: {
                                GroqLlmClient.temperature = 0.1;
                            }
                        }
                    }

                    StyledSlider {
                        id: tempSlider
                        Layout.fillWidth: true
                        from: 0.0
                        to: 1.0
                        stepSize: 0.05
                        value: GroqLlmClient.temperature
                        onMoved: {
                            GroqLlmClient.temperature = tempSlider.value;
                        }
                    }
                }
            }

            StyledCard {
                title: GroqLlmClient.activePreset === "custom" ? qsTr("Custom Instructions") : qsTr(
                                                                     "Preset Instructions")
                description: GroqLlmClient.activePreset === "custom" ? qsTr(
                                                                           "Define specific formatting and transformation instructions") :
                                                                       qsTr("Standard instructions applied by the selected preset")
                opacity: GroqLlmClient.enabled ? 1.0 : 0.5
                enabled: GroqLlmClient.enabled

                PreferenceRow {
                    layoutVertical: true

                    RowLayout {
                        Layout.fillWidth: true

                        StyledText {
                            text: GroqLlmClient.activePreset === "custom" ? qsTr("Instructions") : qsTr(
                                                                                "Instructions (Read-only)")
                            variant: "body"
                            customWeight: Font.Medium
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        StyledText {
                            text: GroqLlmClient.activePreset === "custom" ? qsTr("%1 characters").arg(
                                                                                customPromptArea.text.length) : qsTr(
                                                                                "Preset Managed")
                            variant: "caption"
                            colorRole: "secondary"
                        }
                    }

                    T.ScrollView {
                        Layout.fillWidth: true
                        implicitHeight: 100
                        clip: true

                        T.TextArea {
                            id: customPromptArea
                            readOnly: GroqLlmClient.activePreset !== "custom"
                            text: GroqLlmClient.activePreset === "custom" ? GroqLlmClient.customPrompt :
                                                                            GroqLlmClient.systemPromptForPreset(
                                                                                GroqLlmClient.activePreset)
                            placeholderText: qsTr(
                                                 "e.g. Clean up spoken language, fix grammar, and format lists in markdown...")
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
                                color: customPromptArea.readOnly ? Theme.cardBgSubtle : Theme.inputBg
                                border.color: customPromptArea.activeFocus ? Theme.focusRingColor : Theme.inputBorder
                                border.width: customPromptArea.activeFocus ? Theme.focusRingWidth : 1
                                radius: Theme.radiusSm
                            }

                            onTextChanged: {
                                if (activeFocus && GroqLlmClient.activePreset === "custom") {
                                    saveLlmPromptTimer.restart();
                                }
                            }
                        }
                    }

                    Timer {
                        id: saveLlmPromptTimer
                        interval: 350
                        repeat: false
                        onTriggered: {
                            if (GroqLlmClient.activePreset === "custom") {
                                GroqLlmClient.customPrompt = customPromptArea.text;
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingSm
                        visible: GroqLlmClient.activePreset === "custom"

                        Item {
                            Layout.fillWidth: true
                        }

                        StyledButton {
                            id: clearPromptBtn
                            text: qsTr("Clear")
                            iconSource: "qrc:/qt/qml/QtScribe/assets/icons/trash.svg"
                            enabled: customPromptArea.text.length > 0
                            size: "small"
                            onClicked: {
                                customPromptArea.text = "";
                                GroqLlmClient.customPrompt = "";
                            }
                        }

                        StyledButton {
                            id: savePromptBtn
                            text: qsTr("Save")
                            iconSource: "qrc:/qt/qml/QtScribe/assets/icons/save.svg"
                            size: "small"
                            variant: "primary"
                            onClicked: {
                                saveLlmPromptTimer.stop();
                                GroqLlmClient.customPrompt = customPromptArea.text;
                            }
                        }
                    }
                }
            }
        }
    }
}
