#pragma once

#include "AbstractCloudSttClient.h"

#include <QByteArray>
#include <QPointer>
#include <QString>

class ApiKeyStore;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;
class SettingsManager;
struct CloudApiResponse;

class GeminiSttClient : public AbstractCloudSttClient {
    Q_OBJECT

    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged FINAL)
    Q_PROPERTY(QString selectedModel READ selectedModel WRITE setSelectedModel NOTIFY selectedModelChanged FINAL)
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged FINAL)
    Q_PROPERTY(QString customVocabulary READ customVocabulary WRITE setCustomVocabulary NOTIFY customVocabularyChanged FINAL)

public:
    explicit GeminiSttClient(QObject* parent = nullptr);
    ~GeminiSttClient() override;

    static GeminiSttClient* instance();

    void setKeyStore(ApiKeyStore* store);
    ApiKeyStore* keyStore() const;
    void setNetworkAccessManager(QNetworkAccessManager* nam);

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
    bool handlesSmartFormatting() const override;

    QString selectedModel() const;
    void setSelectedModel(const QString& model);

    QString mode() const;
    void setMode(const QString& mode);

    QString customVocabulary() const;
    void setCustomVocabulary(const QString& vocab);

    void loadSettings();

    void activate() override;
    void deactivate() override;

    Q_INVOKABLE void transcribe(const QByteArray& wavData) override;
    Q_INVOKABLE void retryLast() override;
    Q_INVOKABLE void cancel() override;

signals:
    void apiKeyChanged();
    void selectedModelChanged();
    void modeChanged();
    void customVocabularyChanged();

private:
    void onApiKeySetChanged();

    void sendTranscribeRequest();
    void handleApiResponse(const CloudApiResponse& res);
    static ErrorCategory classifyError(const CloudApiResponse& res, QString& outMessage);
    bool shouldRetry(const CloudApiResponse& res) const;
    static int calculateRetryDelayMs(const CloudApiResponse& res);

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

    QString m_selectedModel;
    QString m_mode;
    QString m_customVocabulary;
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
    static constexpr QStringView kDefaultModel = u"gemini-3.5-transcribe";
    static constexpr QStringView kDefaultMode = u"smart";
    static constexpr QStringView kApiEndpoint = u"https://generativelanguage.googleapis.com/v1beta/interactions";
};
