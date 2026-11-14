#pragma once

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

// Presentation-layer settings facade.
//
// The GUI never executes speech, audio, or download work. It only edits the
// shared configuration (QSettings plus the system keychain) and asks the daemon
// to reload. This keeps every hardware and service resource owned by qtscribed.
class AppSettings : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString groqApiKey READ groqApiKey WRITE setGroqApiKey NOTIFY groqApiKeyChanged FINAL)
    Q_PROPERTY(QString groqSttModel READ groqSttModel WRITE setGroqSttModel NOTIFY groqSttModelChanged FINAL)
    Q_PROPERTY(QString groqLanguage READ groqLanguage WRITE setGroqLanguage NOTIFY groqLanguageChanged FINAL)
    Q_PROPERTY(QString groqCustomPrompt READ groqCustomPrompt WRITE setGroqCustomPrompt NOTIFY groqCustomPromptChanged FINAL)

    Q_PROPERTY(QString geminiApiKey READ geminiApiKey WRITE setGeminiApiKey NOTIFY geminiApiKeyChanged FINAL)
    Q_PROPERTY(QString geminiModel READ geminiModel WRITE setGeminiModel NOTIFY geminiModelChanged FINAL)
    Q_PROPERTY(QString geminiMode READ geminiMode WRITE setGeminiMode NOTIFY geminiModeChanged FINAL)
    Q_PROPERTY(QString geminiCustomVocabulary READ geminiCustomVocabulary WRITE setGeminiCustomVocabulary NOTIFY geminiCustomVocabularyChanged FINAL)

    Q_PROPERTY(QString metaMusaApiKey READ metaMusaApiKey WRITE setMetaMusaApiKey NOTIFY metaMusaApiKeyChanged FINAL)
    Q_PROPERTY(QString metaMusaModel READ metaMusaModel WRITE setMetaMusaModel NOTIFY metaMusaModelChanged FINAL)
    Q_PROPERTY(QString metaMusaKeywords READ metaMusaKeywords WRITE setMetaMusaKeywords NOTIFY metaMusaKeywordsChanged FINAL)

    Q_PROPERTY(bool llmEnabled READ llmEnabled WRITE setLlmEnabled NOTIFY llmEnabledChanged FINAL)
    Q_PROPERTY(QString llmModel READ llmModel WRITE setLlmModel NOTIFY llmModelChanged FINAL)
    Q_PROPERTY(QString llmPreset READ llmPreset WRITE setLlmPreset NOTIFY llmPresetChanged FINAL)
    Q_PROPERTY(QString llmCustomPrompt READ llmCustomPrompt WRITE setLlmCustomPrompt NOTIFY llmCustomPromptChanged FINAL)
    Q_PROPERTY(double llmTemperature READ llmTemperature WRITE setLlmTemperature NOTIFY llmTemperatureChanged FINAL)
    Q_PROPERTY(QString formattedLlmTemperature READ formattedLlmTemperature NOTIFY llmTemperatureChanged FINAL)

    Q_PROPERTY(bool soundEnabled READ soundEnabled WRITE setSoundEnabled NOTIFY soundEnabledChanged FINAL)
    Q_PROPERTY(bool notificationsEnabled READ notificationsEnabled WRITE setNotificationsEnabled NOTIFY notificationsEnabledChanged FINAL)

public:
    explicit AppSettings(QObject* parent = nullptr);
    ~AppSettings() override = default;

    [[nodiscard]] QString groqApiKey() const;
    void setGroqApiKey(const QString& key);
    [[nodiscard]] QString groqSttModel() const;
    void setGroqSttModel(const QString& model);
    [[nodiscard]] QString groqLanguage() const;
    void setGroqLanguage(const QString& language);
    [[nodiscard]] QString groqCustomPrompt() const;
    void setGroqCustomPrompt(const QString& prompt);

    [[nodiscard]] QString geminiApiKey() const;
    void setGeminiApiKey(const QString& key);
    [[nodiscard]] QString geminiModel() const;
    void setGeminiModel(const QString& model);
    [[nodiscard]] QString geminiMode() const;
    void setGeminiMode(const QString& mode);
    [[nodiscard]] QString geminiCustomVocabulary() const;
    void setGeminiCustomVocabulary(const QString& vocabulary);

    [[nodiscard]] QString metaMusaApiKey() const;
    void setMetaMusaApiKey(const QString& key);
    [[nodiscard]] QString metaMusaModel() const;
    void setMetaMusaModel(const QString& model);
    [[nodiscard]] QString metaMusaKeywords() const;
    void setMetaMusaKeywords(const QString& keywords);

    [[nodiscard]] bool llmEnabled() const;
    void setLlmEnabled(bool enabled);
    [[nodiscard]] QString llmModel() const;
    void setLlmModel(const QString& model);
    [[nodiscard]] QString llmPreset() const;
    void setLlmPreset(const QString& preset);
    [[nodiscard]] QString llmCustomPrompt() const;
    void setLlmCustomPrompt(const QString& prompt);
    [[nodiscard]] double llmTemperature() const;
    void setLlmTemperature(double temperature);
    [[nodiscard]] QString formattedLlmTemperature() const;

    [[nodiscard]] bool soundEnabled() const;
    void setSoundEnabled(bool enabled);
    [[nodiscard]] bool notificationsEnabled() const;
    void setNotificationsEnabled(bool enabled);

    Q_INVOKABLE [[nodiscard]] QString systemPromptForPreset(const QString& preset) const;

signals:
    void groqApiKeyChanged();
    void groqSttModelChanged();
    void groqLanguageChanged();
    void groqCustomPromptChanged();
    void geminiApiKeyChanged();
    void geminiModelChanged();
    void geminiModeChanged();
    void geminiCustomVocabularyChanged();
    void metaMusaApiKeyChanged();
    void metaMusaModelChanged();
    void metaMusaKeywordsChanged();
    void llmEnabledChanged();
    void llmModelChanged();
    void llmPresetChanged();
    void llmCustomPromptChanged();
    void llmTemperatureChanged();
    void soundEnabledChanged();
    void notificationsEnabledChanged();

private:
    // NOLINTNEXTLINE(performance-enum-size) small private enum, no QML exposure
    enum class ApiKeyKind { Groq, Gemini, MetaMusa };

    void loadPersistedValues();
    void loadApiKey(ApiKeyKind kind);
    void persistApiKey(ApiKeyKind kind, const QString& value);
    void applyLoadedApiKey(ApiKeyKind kind, const QString& key);
    void emitApiKeyChanged(ApiKeyKind kind);
    static void persistValue(const QString& key, const QVariant& value);
    static void reloadDaemon();

    [[nodiscard]] QString& apiKeyRef(ApiKeyKind kind);

    QString m_groqApiKey;
    QString m_geminiApiKey;
    QString m_metaMusaApiKey;

    QString m_groqSttModel;
    QString m_groqLanguage;
    QString m_groqCustomPrompt;
    QString m_geminiModel;
    QString m_geminiMode;
    QString m_geminiCustomVocabulary;
    QString m_metaMusaModel;
    QString m_metaMusaKeywords;
    QString m_llmModel;
    QString m_llmPreset;
    QString m_llmCustomPrompt;
    double m_llmTemperature = 0.1;
    bool m_llmEnabled = false;
    bool m_soundEnabled = true;
    bool m_notificationsEnabled = true;
};
