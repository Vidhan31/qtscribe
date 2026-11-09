#pragma once

#include "common/SettingsKeys.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>

class SettingsManager : public QObject {
    Q_OBJECT

public:
    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum class TranscriptionBackend { Cloud = 0, WhisperCpp };
    Q_ENUM(TranscriptionBackend)

    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum class RecordingMode { Toggle = 0, PushToTalk };
    Q_ENUM(RecordingMode)

    struct SpeechConfig {
        TranscriptionBackend backend = TranscriptionBackend::Cloud;
        RecordingMode recordingMode = RecordingMode::Toggle;
        bool autoSwitchOffline = true;
        bool operator==(const SpeechConfig&) const = default;
    };

    struct GroqSttConfig {
        QString model = QStringLiteral("whisper-large-v3-turbo");
        QString language;
        QString customPrompt;
        bool operator==(const GroqSttConfig&) const = default;
    };

    struct GeminiSttConfig {
        QString model = QStringLiteral("gemini-3.5-transcribe");
        QString mode = QStringLiteral("smart");
        QString customVocabulary;
        bool operator==(const GeminiSttConfig&) const = default;
    };

    struct LlmConfig {
        bool enabled = false;
        QString model = QStringLiteral("openai/gpt-oss-20b");
        QString preset = QStringLiteral("grammar");
        QString customPrompt;
        double temperature = 0.1;
        bool operator==(const LlmConfig&) const = default;
    };

    struct TextInjectorConfig {
        bool preventClipboardHistory = true;
        int injectionDelayMs = 200;
        bool operator==(const TextInjectorConfig&) const = default;
    };

    struct AudioFeedbackConfig {
        bool soundEnabled = true;
        bool operator==(const AudioFeedbackConfig&) const = default;
    };

    struct NotificationConfig {
        bool notificationsEnabled = true;
        bool operator==(const NotificationConfig&) const = default;
    };

    struct CloudRouterConfig {
        QString activeProvider = QStringLiteral("groq");
        bool operator==(const CloudRouterConfig&) const = default;
    };

    struct WhisperConfig {
        QString selectedModel;
        bool operator==(const WhisperConfig&) const = default;
    };

    explicit SettingsManager(QObject* parent = nullptr);
    ~SettingsManager() override;

    static SettingsManager* instance();
    static void setInstance(SettingsManager* inst);

    void load();
    void save() const;

    [[nodiscard]] const SpeechConfig& speechConfig() const noexcept { return m_speechConfig; }
    [[nodiscard]] const GroqSttConfig& groqSttConfig() const noexcept { return m_groqSttConfig; }
    [[nodiscard]] const GeminiSttConfig& geminiSttConfig() const noexcept { return m_geminiSttConfig; }
    [[nodiscard]] const LlmConfig& llmConfig() const noexcept { return m_llmConfig; }
    [[nodiscard]] const TextInjectorConfig& textInjectorConfig() const noexcept { return m_textInjectorConfig; }
    [[nodiscard]] const AudioFeedbackConfig& audioFeedbackConfig() const noexcept { return m_audioFeedbackConfig; }
    [[nodiscard]] const NotificationConfig& notificationConfig() const noexcept { return m_notificationConfig; }
    [[nodiscard]] const CloudRouterConfig& cloudRouterConfig() const noexcept { return m_cloudRouterConfig; }
    [[nodiscard]] const WhisperConfig& whisperConfig() const noexcept { return m_whisperConfig; }

    void setSpeechConfig(const SpeechConfig& config);
    void setGroqSttConfig(const GroqSttConfig& config);
    void setGeminiSttConfig(const GeminiSttConfig& config);
    void setLlmConfig(const LlmConfig& config);
    void setTextInjectorConfig(const TextInjectorConfig& config);
    void setAudioFeedbackConfig(const AudioFeedbackConfig& config);
    void setNotificationConfig(const NotificationConfig& config);
    void setCloudRouterConfig(const CloudRouterConfig& config);
    void setWhisperConfig(const WhisperConfig& config);

    void setBackend(TranscriptionBackend backend);
    void setRecordingMode(RecordingMode mode);
    void setAutoSwitchOffline(bool enable);

    void setGroqModel(const QString& model);
    void setGroqLanguage(const QString& language);
    void setGroqCustomPrompt(const QString& prompt);

    void setGeminiModel(const QString& model);
    void setGeminiMode(const QString& mode);
    void setGeminiCustomVocabulary(const QString& vocabulary);

    void setLlmEnabled(bool enabled);
    void setLlmModel(const QString& model);
    void setLlmPreset(const QString& preset);
    void setLlmCustomPrompt(const QString& prompt);
    void setLlmTemperature(double temp);

    void setPreventClipboardHistory(bool prevent);
    void setInjectionDelay(int delayMs);

    void setSoundEnabled(bool enabled);
    void setNotificationsEnabled(bool enabled);
    void setActiveProvider(const QString& providerId);
    void setWhisperSelectedModel(const QString& modelId);

    void setSetting(const QString& key, const QVariant& value);
    [[nodiscard]] QVariant getSetting(const QString& key, const QVariant& defaultValue = {}) const;
    [[nodiscard]] QVariantMap allSettings() const;

signals:
    void speechConfigChanged(const SettingsManager::SpeechConfig& config);
    void groqSttConfigChanged(const SettingsManager::GroqSttConfig& config);
    void geminiSttConfigChanged(const SettingsManager::GeminiSttConfig& config);
    void llmConfigChanged(const SettingsManager::LlmConfig& config);
    void textInjectorConfigChanged(const SettingsManager::TextInjectorConfig& config);
    void audioFeedbackConfigChanged(const SettingsManager::AudioFeedbackConfig& config);
    void notificationConfigChanged(const SettingsManager::NotificationConfig& config);
    void cloudRouterConfigChanged(const SettingsManager::CloudRouterConfig& config);
    void whisperConfigChanged(const SettingsManager::WhisperConfig& config);
    void settingChanged(const QString& key, const QVariant& value);

private:
    static QString validateGroqModel(const QString& model);
    static QString validateGeminiModel(const QString& model);
    static QString validateGeminiMode(const QString& mode);
    static QString validateLlmModel(const QString& model);
    static QString validateLlmPreset(const QString& preset);
    static double validateLlmTemperature(double temp);
    static int validateInjectionDelay(int delayMs);

    SpeechConfig m_speechConfig;
    GroqSttConfig m_groqSttConfig;
    GeminiSttConfig m_geminiSttConfig;
    LlmConfig m_llmConfig;
    TextInjectorConfig m_textInjectorConfig;
    AudioFeedbackConfig m_audioFeedbackConfig;
    NotificationConfig m_notificationConfig;
    CloudRouterConfig m_cloudRouterConfig;
    WhisperConfig m_whisperConfig;

    static inline SettingsManager* s_instance = nullptr;
};
