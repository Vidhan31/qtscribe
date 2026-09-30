#pragma once

#include "AbstractLlmClient.h"
#include "HttpRequestRunner.h"

#include <QQmlEngine>
#include <QString>

class ApiKeyStore;
class QNetworkAccessManager;
class QTimer;

class GroqLlmClient : public AbstractLlmClient {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(QString selectedModel READ selectedModel WRITE setSelectedModel NOTIFY selectedModelChanged FINAL)
    Q_PROPERTY(QString activePreset READ activePreset WRITE setActivePreset NOTIFY activePresetChanged FINAL)
    Q_PROPERTY(QString customPrompt READ customPrompt WRITE setCustomPrompt NOTIFY customPromptChanged FINAL)
    Q_PROPERTY(double temperature READ temperature WRITE setTemperature NOTIFY temperatureChanged FINAL)
    Q_PROPERTY(QString formattedTemperature READ formattedTemperature NOTIFY temperatureChanged FINAL)

public:
    explicit GroqLlmClient(QObject* parent = nullptr);
    ~GroqLlmClient() override = default;

    void setKeyStore(ApiKeyStore* keyStore);
    ApiKeyStore* keyStore() const;

    bool isBusy() const override;
    bool isCancelled() const;

    bool enabled() const override;
    void setEnabled(bool enabled) override;

    QString lastError() const;

    QString selectedModel() const;
    void setSelectedModel(const QString& model);

    QString activePreset() const;
    void setActivePreset(const QString& preset);

    QString customPrompt() const;
    void setCustomPrompt(const QString& prompt);

    double temperature() const;
    void setTemperature(double temp);
    QString formattedTemperature() const;

    Q_INVOKABLE QString systemPromptForPreset(const QString& preset) const;
    Q_INVOKABLE void processText(const QString& rawText) override;
    Q_INVOKABLE void cancel() override;

signals:
    void lastErrorChanged();
    void selectedModelChanged();
    void activePresetChanged();
    void customPromptChanged();
    void temperatureChanged();

private:
    void setBusy(bool busy);
    void setLastError(const QString& error);
    void sendProcessRequest();
    void handleProcessResponse(const CloudApiResponse& res);
    QString currentSystemPrompt() const;

    ApiKeyStore* m_keyStore = nullptr;
    QNetworkAccessManager* m_nam = nullptr;
    QTimer* m_retryTimer = nullptr;
    HttpRequestRunner m_requestRunner;
    QString m_lastError;
    QString m_selectedModel;
    QString m_activePreset;
    QString m_customPrompt;
    QString m_pendingRawText;
    double m_temperature = 0.1;
    bool m_enabled = false;

    inline static constexpr QStringView kDefaultModel = u"openai/gpt-oss-20b";
};
