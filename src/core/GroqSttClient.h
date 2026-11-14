#pragma once

#include "BaseCloudSttClient.h"

class GroqSttClient : public BaseCloudSttClient {
    Q_OBJECT

    Q_PROPERTY(QString selectedModel READ selectedModel WRITE setSelectedModel NOTIFY selectedModelChanged FINAL)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged FINAL)
    Q_PROPERTY(QString customPrompt READ customPrompt WRITE setCustomPrompt NOTIFY customPromptChanged FINAL)

public:
    explicit GroqSttClient(QObject* parent = nullptr);
    ~GroqSttClient() override;

    static GroqSttClient* instance();

    QString selectedModel() const;
    void setSelectedModel(const QString& model);

    QString language() const;
    void setLanguage(const QString& lang);

    QString customPrompt() const;
    void setCustomPrompt(const QString& prompt);

    void setSettingsManager(SettingsManager* settings);
    void loadSettings() override;

    using BaseCloudSttClient::transcribe;
    Q_INVOKABLE void transcribe(const QByteArray& wavData, const QString& filename);

signals:
    void selectedModelChanged();
    void languageChanged();
    void customPromptChanged();

protected:
    void sendTranscribeRequest() override;
    bool parseTranscript(const CloudApiResponse& res, QString& outText) override;

private:
    QString m_selectedModel;
    QString m_language;
    QString m_customPrompt;
    QString m_lastFilename;

    static constexpr QStringView kDefaultModel = u"whisper-large-v3-turbo";
    static constexpr QStringView kApiEndpoint = u"https://api.groq.com/openai/v1/audio/transcriptions";
};
