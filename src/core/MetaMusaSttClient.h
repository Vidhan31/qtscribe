#pragma once

#include "BaseCloudSttClient.h"

class MetaMusaSttClient : public BaseCloudSttClient {
    Q_OBJECT

    Q_PROPERTY(QString selectedModel READ selectedModel WRITE setSelectedModel NOTIFY selectedModelChanged FINAL)
    Q_PROPERTY(QString keywords READ keywords WRITE setKeywords NOTIFY keywordsChanged FINAL)

public:
    explicit MetaMusaSttClient(QObject* parent = nullptr);
    ~MetaMusaSttClient() override;

    static MetaMusaSttClient* instance();

    bool handlesSmartFormatting() const override;

    QString selectedModel() const;
    void setSelectedModel(const QString& model);

    QString keywords() const;
    void setKeywords(const QString& keywords);

    void setSettingsManager(SettingsManager* settings);
    void loadSettings() override;

signals:
    void selectedModelChanged();
    void keywordsChanged();

protected:
    void sendTranscribeRequest() override;
    bool parseTranscript(const CloudApiResponse& res, QString& outText) override;

private:
    QString m_selectedModel;
    QString m_keywords;

    static constexpr QStringView kDefaultModel = u"muse-voice-transcribe-1.0";
    static constexpr QStringView kApiEndpoint = u"https://api.meta.ai/v1/asr/transcribe";
};
