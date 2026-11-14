#pragma once

#include "BaseCloudSttClient.h"

class GeminiSttClient : public BaseCloudSttClient {
    Q_OBJECT

    Q_PROPERTY(QString selectedModel READ selectedModel WRITE setSelectedModel NOTIFY selectedModelChanged FINAL)
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged FINAL)
    Q_PROPERTY(QString customVocabulary READ customVocabulary WRITE setCustomVocabulary NOTIFY customVocabularyChanged FINAL)

public:
    explicit GeminiSttClient(QObject* parent = nullptr);
    ~GeminiSttClient() override;

    static GeminiSttClient* instance();

    bool handlesSmartFormatting() const override;

    QString selectedModel() const;
    void setSelectedModel(const QString& model);

    QString mode() const;
    void setMode(const QString& mode);

    QString customVocabulary() const;
    void setCustomVocabulary(const QString& vocab);

    void setSettingsManager(SettingsManager* settings);
    void loadSettings() override;

signals:
    void selectedModelChanged();
    void modeChanged();
    void customVocabularyChanged();

protected:
    void sendTranscribeRequest() override;
    bool parseTranscript(const CloudApiResponse& res, QString& outText) override;

private:
    QString m_selectedModel;
    QString m_mode;
    QString m_customVocabulary;

    static constexpr QStringView kDefaultModel = u"gemini-3.5-transcribe";
    static constexpr QStringView kDefaultMode = u"smart";
    static constexpr QStringView kApiEndpoint = u"https://generativelanguage.googleapis.com/v1beta/interactions";
};
