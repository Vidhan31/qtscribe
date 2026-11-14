#include "BaseCloudSttClient.h"

#include "LoggingCategories.h"

#include "ApiKeyStore.h"
#include "CloudResponseParser.h"
#include "NetworkTlsHelper.h"
#include "SettingsManager.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslError>
#include <QTimer>

#include <utility>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

BaseCloudSttClient::BaseCloudSttClient(QString providerLogName, QString providerDisplayName,
                                       std::chrono::milliseconds transferTimeout, QObject* parent)
    : AbstractCloudSttClient(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_retryCountdownTimer(new QTimer(this))
    , m_retryTimer(new QTimer(this))
    , m_providerLogName(std::move(providerLogName))
    , m_providerDisplayName(std::move(providerDisplayName)) {
    NetworkTlsHelper::ensureSystemProxyConfigured();

    m_nam->setTransferTimeout(transferTimeout);
    connect(m_nam, &QNetworkAccessManager::sslErrors, this,
            [this](QNetworkReply* /*reply*/, const QList<QSslError>& errors) {
                for (const auto& err : errors) {
                    qWarning() << m_providerLogName << "SSL Error:" << err.errorString();
                }
            });

    m_retryCountdownTimer->setInterval(1s);
    connect(m_retryCountdownTimer, &QTimer::timeout, this, [this]() {
        if (m_retrySecondsRemaining > 0) {
            setRetrySecondsRemaining(m_retrySecondsRemaining - 1);
        } else {
            m_retryCountdownTimer->stop();
        }
    });

    m_retryTimer->setSingleShot(true);
    connect(m_retryTimer, &QTimer::timeout, this, [this]() {
        if (!m_cancelled && !m_lastWavData.isEmpty()) {
            sendTranscribeRequest();
        }
    });
}

BaseCloudSttClient::~BaseCloudSttClient() = default;

void BaseCloudSttClient::setKeyStore(ApiKeyStore* store) {
    if (m_keyStore == store) {
        return;
    }

    const bool wasApiKeySet = isApiKeySet();
    const bool wasKeyInvalid = isApiKeyInvalid();

    if (m_keyStore) {
        disconnect(m_keyStore, &ApiKeyStore::apiKeyChanged, this, &BaseCloudSttClient::apiKeyChanged);
        disconnect(m_keyStore, &ApiKeyStore::apiKeySetChanged, this, &BaseCloudSttClient::onApiKeySetChanged);
        if (m_ownsKeyStore && m_keyStore->parent() == this) {
            m_keyStore->deleteLater();
        }
    }

    m_keyStore = store;
    m_ownsKeyStore = false;

    if (m_keyStore) {
        connect(m_keyStore, &ApiKeyStore::apiKeyChanged, this, &BaseCloudSttClient::apiKeyChanged);
        connect(m_keyStore, &ApiKeyStore::apiKeySetChanged, this, &BaseCloudSttClient::onApiKeySetChanged);
    }

    if (isApiKeySet() != wasApiKeySet) {
        emit apiKeySetChanged();
        emit readyChanged();
    }
    if (isApiKeyInvalid() != wasKeyInvalid) {
        emit isApiKeyInvalidChanged();
    }
}

ApiKeyStore* BaseCloudSttClient::keyStore() const {
    return m_keyStore;
}

void BaseCloudSttClient::setNetworkAccessManager(QNetworkAccessManager* nam) {
    if (m_nam == nam) {
        return;
    }
    if (m_nam && m_nam->parent() == this) {
        m_nam->deleteLater();
    }
    m_nam = nam;
}

QNetworkAccessManager* BaseCloudSttClient::networkAccessManager() const noexcept {
    return m_nam;
}

void BaseCloudSttClient::setSettingsManager(SettingsManager* settings) {
    if (m_settingsManager == settings) {
        return;
    }
    if (m_settingsManager) {
        disconnect(m_settingsManager, nullptr, this, nullptr);
    }
    m_settingsManager = settings;
    loadSettings();
}

SettingsManager* BaseCloudSttClient::settingsManager() const noexcept {
    return m_settingsManager;
}

QString BaseCloudSttClient::apiKey() const {
    return m_keyStore ? m_keyStore->apiKey() : QString();
}

void BaseCloudSttClient::setApiKey(const QString& key) {
    if (m_keyStore) {
        m_keyStore->setApiKey(key);
    }
}

bool BaseCloudSttClient::isApiKeySet() const {
    return m_keyStore && m_keyStore->apiKeySet();
}

bool BaseCloudSttClient::isApiKeyInvalid() const {
    return isApiKeySet() && m_errorCategory == ErrorCategory::InvalidApiKey;
}

bool BaseCloudSttClient::isRateLimited() const {
    return m_errorCategory == ErrorCategory::RateLimited && m_retrySecondsRemaining > 0;
}

int BaseCloudSttClient::retrySecondsRemaining() const {
    return m_retrySecondsRemaining;
}

AbstractCloudSttClient::ErrorCategory BaseCloudSttClient::errorCategory() const {
    return m_errorCategory;
}

QString BaseCloudSttClient::lastError() const {
    return m_lastError;
}

bool BaseCloudSttClient::isReady() const {
    return isApiKeySet();
}

bool BaseCloudSttClient::isBusy() const {
    return m_busy;
}

bool BaseCloudSttClient::isCancelled() const {
    return m_cancelled;
}

void BaseCloudSttClient::activate() {
    if (m_keyStore) {
        m_keyStore->ensureApiKeyLoaded();
    }
    emit readyChanged();
}

void BaseCloudSttClient::deactivate() {
    cancel();
}

void BaseCloudSttClient::onApiKeySetChanged() {
    const bool wasKeyInvalid = isApiKeyInvalid();
    emit apiKeySetChanged();
    if (isApiKeyInvalid() != wasKeyInvalid) {
        emit isApiKeyInvalidChanged();
    }
    emit readyChanged();
}

void BaseCloudSttClient::setBusy(bool busy) {
    if (m_busy != busy) {
        m_busy = busy;
        emit busyChanged();
    }
}

void BaseCloudSttClient::setLastError(const QString& error, ErrorCategory category) {
    if (m_lastError != error) {
        m_lastError = error;
        emit lastErrorChanged();
    }
    setErrorCategory(category);
}

void BaseCloudSttClient::setErrorCategory(ErrorCategory category) {
    if (m_errorCategory != category) {
        const bool wasKeyInvalid = isApiKeyInvalid();
        const bool wasRateLimited = isRateLimited();
        m_errorCategory = category;
        emit errorCategoryChanged();
        if (isApiKeyInvalid() != wasKeyInvalid) {
            emit isApiKeyInvalidChanged();
        }
        if (isRateLimited() != wasRateLimited) {
            emit isRateLimitedChanged();
        }
    }
}

void BaseCloudSttClient::setRetrySecondsRemaining(int seconds) {
    if (m_retrySecondsRemaining != seconds) {
        const bool wasRateLimited = isRateLimited();
        m_retrySecondsRemaining = seconds;
        emit retrySecondsRemainingChanged();
        if (isRateLimited() != wasRateLimited) {
            emit isRateLimitedChanged();
        }
    }
}

void BaseCloudSttClient::cancel() {
    const bool wasBusy = m_busy;
    if (m_retryTimer) {
        m_retryTimer->stop();
    }
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply = nullptr;
    }
    m_cancelled = true;
    m_retryCountdownTimer->stop();
    setRetrySecondsRemaining(0);
    m_lastWavData.clear();
    m_retryCount = 0;
    setBusy(false);
    if (wasBusy) {
        emit busyChanged();
    }
}

void BaseCloudSttClient::retryLast() {
    if (!m_lastWavData.isEmpty() && !isBusy()) {
        if (m_retryTimer) {
            m_retryTimer->stop();
        }
        m_cancelled = false;
        m_retryCount = 0;
        sendTranscribeRequest();
    }
}

void BaseCloudSttClient::transcribe(const QByteArray& wavData) {
    if (isBusy()) {
        qCDebug(lcNetwork) << m_providerLogName << "transcribe ignored, request already in progress";
        setLastError(u"A transcription request is already in progress"_s, ErrorCategory::GeneralError);
        return;
    }

    if (!isApiKeySet()) {
        qWarning() << m_providerLogName << "Attempted transcribe without an API key";
        const QString err = u"%1 API key is not set"_s.arg(m_providerDisplayName);
        setLastError(err, ErrorCategory::InvalidApiKey);
        emit errorOccurred(m_lastError);
        return;
    }

    if (wavData.isEmpty()) {
        qWarning() << m_providerLogName << "Attempted transcribe with empty audio data";
        setLastError(u"No audio data to transcribe"_s, ErrorCategory::GeneralError);
        emit errorOccurred(m_lastError);
        return;
    }

    if (m_retryTimer) {
        m_retryTimer->stop();
    }
    m_cancelled = false;
    m_retryCount = 0;
    m_lastWavData = wavData;

    sendTranscribeRequest();
}

bool BaseCloudSttClient::shouldRetry(const CloudApiResponse& res) const {
    if (m_cancelled || m_retryCount >= kMaxRetries || res.isSuccess) {
        return false;
    }
    if (res.isRateLimited && res.retryAfterSeconds > 0 && res.retryAfterSeconds <= kMaxTransientRetryAfterSec) {
        return true;
    }
    if (res.httpStatus >= 500 && res.httpStatus <= 599) {
        return true;
    }
    switch (res.networkError) {
        case QNetworkReply::RemoteHostClosedError:
        case QNetworkReply::TemporaryNetworkFailureError:
        case QNetworkReply::TimeoutError:
        case QNetworkReply::NetworkSessionFailedError:
            return true;
        default:
            break;
    }
    return false;
}

int BaseCloudSttClient::calculateRetryDelayMs(const CloudApiResponse& res) {
    if (res.isRateLimited && res.retryAfterSeconds > 0) {
        return res.retryAfterSeconds * 1000;
    }
    return kDefaultDelayMs;
}

AbstractCloudSttClient::ErrorCategory BaseCloudSttClient::classifyCommonError(const CloudApiResponse& res,
                                                                              QString& outMessage,
                                                                              const QString& providerDisplayName) {
    ErrorCategory cat = ErrorCategory::GeneralError;
    outMessage = res.errorMessage;

    if (res.httpStatus == 401 || res.httpStatus == 403 ||
        outMessage.contains(u"API_KEY_INVALID"_s, Qt::CaseInsensitive) ||
        outMessage.contains(u"Invalid API Key"_s, Qt::CaseInsensitive) ||
        outMessage.contains(u"API key not valid"_s, Qt::CaseInsensitive)) {
        cat = ErrorCategory::InvalidApiKey;
        outMessage = tr("Invalid API Key. Please check your %1 API key in Settings.").arg(providerDisplayName);
    } else if (res.httpStatus == 402 || outMessage.contains(u"billing"_s, Qt::CaseInsensitive) ||
               outMessage.contains(u"insufficient_quota"_s, Qt::CaseInsensitive) ||
               outMessage.contains(u"payment_required"_s, Qt::CaseInsensitive)) {
        cat = ErrorCategory::BillingError;
        outMessage =
            tr("Billing error. Please verify your %1 account balance or payment details.").arg(providerDisplayName);
    } else if (res.isRateLimited) {
        cat = ErrorCategory::RateLimited;
    } else if (res.networkError == QNetworkReply::HostNotFoundError ||
               res.networkError == QNetworkReply::ConnectionRefusedError ||
               res.networkError == QNetworkReply::TimeoutError ||
               res.networkError == QNetworkReply::NetworkSessionFailedError) {
        cat = ErrorCategory::NetworkOffline;
        outMessage = tr("No internet connection. Please check your network and try again.");
    }

    return cat;
}

AbstractCloudSttClient::ErrorCategory BaseCloudSttClient::classifyError(const CloudApiResponse& res,
                                                                        QString& outMessage) {
    return classifyCommonError(res, outMessage, m_providerDisplayName);
}

void BaseCloudSttClient::handleApiResponse(const CloudApiResponse& res) {
    m_currentReply = nullptr;

    qCDebug(lcNetwork) << m_providerLogName << "HTTP response received -> Status:" << res.httpStatus
                       << "Elapsed time:" << res.latencyMs << "ms";

    if (m_cancelled || res.networkError == QNetworkReply::OperationCanceledError) {
        qCDebug(lcNetwork) << m_providerLogName << ": Request cancelled/aborted, ignoring response";
        if (m_retryTimer) {
            m_retryTimer->stop();
        }
        setBusy(false);
        return;
    }

    if (!res.isSuccess) {
        if (shouldRetry(res) && !m_lastWavData.isEmpty()) {
            ++m_retryCount;
            const int delayMs = calculateRetryDelayMs(res);
            qCDebug(lcNetwork) << m_providerLogName << ": Transient error encountered (Status:" << res.httpStatus
                               << "Error:" << res.networkError << "). Scheduling retry in" << delayMs << "ms (Attempt"
                               << m_retryCount << "/" << kMaxRetries << ")";
            if (m_retryTimer) {
                m_retryTimer->start(delayMs);
            }
            return;
        }

        if (m_retryTimer) {
            m_retryTimer->stop();
        }
        setBusy(false);

        QString errorText;
        const ErrorCategory cat = classifyError(res, errorText);

        if (cat == ErrorCategory::RateLimited && res.retryAfterSeconds > 0) {
            setRetrySecondsRemaining(res.retryAfterSeconds);
            m_retryCountdownTimer->start();
        }

        qWarning() << m_providerLogName << "error:" << errorText << "Category:" << static_cast<int>(cat)
                   << "Raw body:" << NetworkTlsHelper::redactedBodyPreview(res.rawBody);
        setLastError(errorText, cat);
        emit errorOccurred(errorText);
        return;
    }

    if (m_retryTimer) {
        m_retryTimer->stop();
    }
    m_lastWavData.clear();
    m_retryCountdownTimer->stop();
    setRetrySecondsRemaining(0);
    setBusy(false);

    QString text;
    if (!parseTranscript(res, text) || text.isEmpty()) {
        qWarning() << m_providerLogName << ": Empty transcription text in response";
        const QString err = u"%1 API returned empty transcription"_s.arg(m_providerDisplayName);
        setLastError(err, ErrorCategory::GeneralError);
        emit errorOccurred(err);
        return;
    }

    qCDebug(lcNetwork) << m_providerLogName << ": Transcription successfully received -> Length:" << text.size()
                       << "chars";

    setLastError({}, ErrorCategory::None);
    emit transcriptionReady(text);
}
