#pragma once

#include "AbstractCloudSttClient.h"

#include <QByteArray>
#include <QPointer>
#include <QString>

#include <chrono>

class ApiKeyStore;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;
class SettingsManager;
struct CloudApiResponse;

class BaseCloudSttClient : public AbstractCloudSttClient {
    Q_OBJECT

    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged FINAL)

public:
    explicit BaseCloudSttClient(QString providerLogName, QString providerDisplayName,
                                std::chrono::milliseconds transferTimeout = std::chrono::seconds(15),
                                QObject* parent = nullptr);
    ~BaseCloudSttClient() override;

    void setKeyStore(ApiKeyStore* store);
    ApiKeyStore* keyStore() const;

    void setNetworkAccessManager(QNetworkAccessManager* nam);
    [[nodiscard]] QNetworkAccessManager* networkAccessManager() const noexcept;

    void setSettingsManager(SettingsManager* settings);
    [[nodiscard]] SettingsManager* settingsManager() const noexcept;

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

    void activate() override;
    void deactivate() override;

    Q_INVOKABLE void transcribe(const QByteArray& wavData) override;
    Q_INVOKABLE void retryLast() override;
    Q_INVOKABLE void cancel() override;

    virtual void loadSettings() = 0;

signals:
    void apiKeyChanged();

protected:
    void onApiKeySetChanged();
    virtual void sendTranscribeRequest() = 0;
    virtual bool parseTranscript(const CloudApiResponse& res, QString& outText) = 0;
    virtual ErrorCategory classifyError(const CloudApiResponse& res, QString& outMessage);
    void handleApiResponse(const CloudApiResponse& res);

    bool shouldRetry(const CloudApiResponse& res) const;
    static int calculateRetryDelayMs(const CloudApiResponse& res);
    static ErrorCategory classifyCommonError(const CloudApiResponse& res, QString& outMessage,
                                             const QString& providerDisplayName);

    void setBusy(bool busy);
    void setLastError(const QString& error, ErrorCategory category = ErrorCategory::GeneralError);
    void setErrorCategory(ErrorCategory category);
    void setRetrySecondsRemaining(int seconds);

    ApiKeyStore* m_keyStore = nullptr;
    SettingsManager* m_settingsManager = nullptr;
    bool m_ownsKeyStore = false;

    QNetworkAccessManager* m_nam = nullptr;
    QPointer<QNetworkReply> m_currentReply;
    QTimer* m_retryCountdownTimer = nullptr;
    QTimer* m_retryTimer = nullptr;

    QByteArray m_lastWavData;
    QString m_lastError;
    ErrorCategory m_errorCategory = ErrorCategory::None;
    int m_retrySecondsRemaining = 0;
    int m_retryCount = 0;
    bool m_busy = false;
    bool m_cancelled = false;
    QString m_providerLogName;
    QString m_providerDisplayName;

    static constexpr int kMaxRetries = 1;
    static constexpr int kDefaultDelayMs = 1000;
    static constexpr int kMaxTransientRetryAfterSec = 3;
};
