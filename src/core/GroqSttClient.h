#pragma once

#include "AbstractSttClient.h"

#include <QByteArray>
#include <QPointer>
#include <QQmlEngine>
#include <QString>

class ApiKeyStore;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;
struct CloudApiResponse;

class GroqSttClient : public AbstractSttClient {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged FINAL)
    Q_PROPERTY(QString selectedModel READ selectedModel WRITE setSelectedModel NOTIFY selectedModelChanged FINAL)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged FINAL)
    Q_PROPERTY(QString customPrompt READ customPrompt WRITE setCustomPrompt NOTIFY customPromptChanged FINAL)

public:
    explicit GroqSttClient(QObject* parent = nullptr);
    ~GroqSttClient() override = default;

    void setKeyStore(ApiKeyStore* store);
    ApiKeyStore* keyStore() const;

    QString apiKey() const;
    void setApiKey(const QString& key);

    bool isApiKeySet() const override;
    bool isApiKeyInvalid() const override;
    bool isRateLimited() const override;
    int retrySecondsRemaining() const override;
    ErrorCategory errorCategory() const override;
    QString lastError() const override;

    bool isReady() const override;
    bool isBusy() const override;
    bool isCancelled() const;

    QString selectedModel() const;
    void setSelectedModel(const QString& model);

    QString language() const;
    void setLanguage(const QString& lang);

    QString customPrompt() const;
    void setCustomPrompt(const QString& prompt);

    void activate() override;
    void deactivate() override;

    Q_INVOKABLE void transcribe(const QByteArray& wavData) override;
    Q_INVOKABLE void transcribe(const QByteArray& wavData, const QString& filename);
    Q_INVOKABLE void retryLast() override;
    Q_INVOKABLE void cancel() override;

signals:
    void apiKeyChanged();
    void selectedModelChanged();
    void languageChanged();
    void customPromptChanged();

private slots:
    void onApiKeySetChanged();

private:
    void sendTranscribeRequest();
    void handleApiResponse(const CloudApiResponse& res);
    ErrorCategory classifyError(const CloudApiResponse& res, QString& outMessage) const;
    bool shouldRetry(const CloudApiResponse& res) const;
    int calculateRetryDelayMs(const CloudApiResponse& res) const;

    void setBusy(bool busy);
    void setLastError(const QString& error, ErrorCategory category = ErrorCategory::GeneralError);
    void setErrorCategory(ErrorCategory category);
    void setRetrySecondsRemaining(int seconds);

    ApiKeyStore* m_keyStore = nullptr;
    bool m_ownsKeyStore = false;

    QNetworkAccessManager* m_nam = nullptr;
    QPointer<QNetworkReply> m_currentReply;
    QTimer* m_retryCountdownTimer = nullptr;
    QTimer* m_retryTimer = nullptr;

    QString m_selectedModel;
    QString m_language;
    QString m_customPrompt;
    QString m_lastFilename;
    QByteArray m_lastWavData;
    QString m_lastError;
    ErrorCategory m_errorCategory = ErrorCategory::None;
    int m_retrySecondsRemaining = 0;
    int m_retryCount = 0;
    bool m_busy = false;
    bool m_cancelled = false;

    static constexpr int kMaxRetries = 1;
    static constexpr int kDefaultDelayMs = 1000;
    static constexpr int kMaxTransientRetryAfterSec = 3;
    inline static constexpr QStringView kDefaultModel = u"whisper-large-v3-turbo";
    inline static constexpr QStringView kApiEndpoint = u"https://api.groq.com/openai/v1/audio/transcriptions";
};
