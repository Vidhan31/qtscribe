#include "GroqSttClient.h"

#include "LoggingCategories.h"

#include "ApiKeyStore.h"
#include "CloudResponseParser.h"
#include "NetworkTlsHelper.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHttpHeaders>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QTimer>

#include <memory>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

#ifndef QTSCRIBE_VERSION
#define QTSCRIBE_VERSION "0.0.0-dev"
#endif

static QString groqUserAgent() {
    const QString appVer = QCoreApplication::applicationVersion().isEmpty() ? u"" QTSCRIBE_VERSION ""_s
                                                                            : QCoreApplication::applicationVersion();
    return u"QtScribe/%1 (Linux; Qt %2)"_s.arg(appVer, QString::fromUtf8(QT_VERSION_STR));
}

GroqSttClient::GroqSttClient(QObject* parent)
    : AbstractSttClient(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_retryCountdownTimer(new QTimer(this))
    , m_retryTimer(new QTimer(this)) {
    NetworkTlsHelper::ensureSystemProxyConfigured();

    m_nam->setTransferTimeout(15s);
    connect(m_nam, &QNetworkAccessManager::sslErrors, this,
            [](QNetworkReply* /*reply*/, const QList<QSslError>& errors) {
                for (const auto& err : errors) {
                    qWarning() << "GroqSttClient SSL Error:" << err.errorString();
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

    auto* defaultStore = new ApiKeyStore(this);
    defaultStore->setStorageKeys(u"QtScribe"_s, u"groq_api_key"_s, u"Cloud/Providers/Groq/ApiKey"_s);
    setKeyStore(defaultStore);
    m_ownsKeyStore = true;

    QSettings settings;
    m_selectedModel = settings.value(u"Groq/Model"_s, kDefaultModel.toString()).toString();
    if (m_selectedModel.isEmpty() ||
        (m_selectedModel != u"whisper-large-v3-turbo"_s && m_selectedModel != u"whisper-large-v3"_s)) {
        m_selectedModel = kDefaultModel.toString();
    }
    m_language = settings.value(u"Groq/Language"_s, QString()).toString();
    m_customPrompt = settings.value(u"Groq/CustomPrompt"_s, QString()).toString();
}

void GroqSttClient::setKeyStore(ApiKeyStore* store) {
    if (m_keyStore == store) {
        return;
    }

    const bool wasApiKeySet = isApiKeySet();
    const bool wasKeyInvalid = isApiKeyInvalid();

    if (m_keyStore) {
        disconnect(m_keyStore, &ApiKeyStore::apiKeyChanged, this, &GroqSttClient::apiKeyChanged);
        disconnect(m_keyStore, &ApiKeyStore::apiKeySetChanged, this, &GroqSttClient::onApiKeySetChanged);
        if (m_ownsKeyStore && m_keyStore->parent() == this) {
            m_keyStore->deleteLater();
        }
    }

    m_keyStore = store;
    m_ownsKeyStore = false;

    if (m_keyStore) {
        connect(m_keyStore, &ApiKeyStore::apiKeyChanged, this, &GroqSttClient::apiKeyChanged);
        connect(m_keyStore, &ApiKeyStore::apiKeySetChanged, this, &GroqSttClient::onApiKeySetChanged);
    }

    if (isApiKeySet() != wasApiKeySet) {
        emit apiKeySetChanged();
        emit readyChanged();
    }
    if (isApiKeyInvalid() != wasKeyInvalid) {
        emit isApiKeyInvalidChanged();
    }
}

ApiKeyStore* GroqSttClient::keyStore() const {
    return m_keyStore;
}

QString GroqSttClient::apiKey() const {
    return m_keyStore ? m_keyStore->apiKey() : QString();
}

void GroqSttClient::setApiKey(const QString& key) {
    if (m_keyStore) {
        m_keyStore->setApiKey(key);
    }
}

bool GroqSttClient::isApiKeySet() const {
    return m_keyStore && m_keyStore->apiKeySet();
}

bool GroqSttClient::isApiKeyInvalid() const {
    return isApiKeySet() && m_errorCategory == ErrorCategory::InvalidApiKey;
}

bool GroqSttClient::isRateLimited() const {
    return m_errorCategory == ErrorCategory::RateLimited && m_retrySecondsRemaining > 0;
}

int GroqSttClient::retrySecondsRemaining() const {
    return m_retrySecondsRemaining;
}

AbstractSttClient::ErrorCategory GroqSttClient::errorCategory() const {
    return m_errorCategory;
}

QString GroqSttClient::lastError() const {
    return m_lastError;
}

bool GroqSttClient::isReady() const {
    return isApiKeySet();
}

bool GroqSttClient::isBusy() const {
    return m_busy;
}

bool GroqSttClient::isCancelled() const {
    return m_cancelled;
}

QString GroqSttClient::selectedModel() const {
    return m_selectedModel;
}

void GroqSttClient::setSelectedModel(const QString& model) {
    QString trimmed = model.trimmed();
    if (trimmed.isEmpty() || (trimmed != u"whisper-large-v3-turbo"_s && trimmed != u"whisper-large-v3"_s)) {
        trimmed = kDefaultModel.toString();
    }
    if (m_selectedModel != trimmed) {
        m_selectedModel = trimmed;
        QSettings settings;
        settings.setValue(u"Groq/Model"_s, m_selectedModel);
        emit selectedModelChanged();
    }
}

QString GroqSttClient::language() const {
    return m_language;
}

void GroqSttClient::setLanguage(const QString& lang) {
    QString trimmed = lang.trimmed();
    if (m_language != trimmed) {
        m_language = trimmed;
        QSettings settings;
        settings.setValue(u"Groq/Language"_s, m_language);
        emit languageChanged();
    }
}

QString GroqSttClient::customPrompt() const {
    return m_customPrompt;
}

void GroqSttClient::setCustomPrompt(const QString& prompt) {
    if (m_customPrompt != prompt) {
        m_customPrompt = prompt;
        QSettings settings;
        settings.setValue(u"Groq/CustomPrompt"_s, m_customPrompt);
        emit customPromptChanged();
    }
}

void GroqSttClient::activate() {
    if (m_keyStore) {
        m_keyStore->ensureApiKeyLoaded();
    }
    emit readyChanged();
}

void GroqSttClient::deactivate() {
    cancel();
}

void GroqSttClient::onApiKeySetChanged() {
    const bool wasKeyInvalid = isApiKeyInvalid();
    emit apiKeySetChanged();
    if (isApiKeyInvalid() != wasKeyInvalid) {
        emit isApiKeyInvalidChanged();
    }
    emit readyChanged();
}

void GroqSttClient::setBusy(bool busy) {
    if (m_busy != busy) {
        m_busy = busy;
        emit busyChanged();
    }
}

void GroqSttClient::setLastError(const QString& error, ErrorCategory category) {
    if (m_lastError != error) {
        m_lastError = error;
        emit lastErrorChanged();
    }
    setErrorCategory(category);
}

void GroqSttClient::setErrorCategory(ErrorCategory category) {
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

void GroqSttClient::setRetrySecondsRemaining(int seconds) {
    if (m_retrySecondsRemaining != seconds) {
        const bool wasRateLimited = isRateLimited();
        m_retrySecondsRemaining = seconds;
        emit retrySecondsRemainingChanged();
        if (isRateLimited() != wasRateLimited) {
            emit isRateLimitedChanged();
        }
    }
}

void GroqSttClient::cancel() {
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

void GroqSttClient::retryLast() {
    if (!m_lastWavData.isEmpty() && !isBusy()) {
        if (m_retryTimer) {
            m_retryTimer->stop();
        }
        m_cancelled = false;
        m_retryCount = 0;
        sendTranscribeRequest();
    }
}

void GroqSttClient::transcribe(const QByteArray& wavData) {
    transcribe(wavData, u"audio.wav"_s);
}

void GroqSttClient::transcribe(const QByteArray& wavData, const QString& filename) {
    if (isBusy()) {
        qCDebug(lcNetwork) << "Groq STT: transcribe ignored, request already in progress";
        setLastError(u"A transcription request is already in progress"_s, ErrorCategory::GeneralError);
        return;
    }

    if (!isApiKeySet()) {
        qWarning() << "Groq STT: Attempted transcribe without an API key";
        setLastError(u"Groq API key is not set"_s, ErrorCategory::InvalidApiKey);
        emit errorOccurred(m_lastError);
        return;
    }

    if (wavData.isEmpty()) {
        qWarning() << "Groq STT: Attempted transcribe with empty audio data";
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
    m_lastFilename = filename;

    sendTranscribeRequest();
}

void GroqSttClient::sendTranscribeRequest() {
    if (!isApiKeySet() || m_lastWavData.isEmpty() || m_cancelled) {
        setBusy(false);
        return;
    }

    setBusy(true);
    setLastError({});

    QString modelToUse = m_selectedModel.trimmed().isEmpty() ? kDefaultModel.toString() : m_selectedModel.trimmed();
    QString langToUse = m_language.trimmed();
    QString promptToUse = m_customPrompt.trimmed();
    QString filenameToUse = m_lastFilename.isEmpty() ? u"audio.wav"_s : m_lastFilename;

    qCDebug(lcNetwork) << "Preparing Groq STT multipart request -> Model:" << modelToUse
                       << "Language:" << (langToUse.isEmpty() ? u"Auto-detect"_s : langToUse)
                       << "Prompt set:" << (!promptToUse.isEmpty()) << "Filename:" << filenameToUse
                       << "Audio payload size:" << m_lastWavData.size() << "bytes"
                       << "(Retry attempt:" << m_retryCount << ")";

    auto* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart modelPart;
    modelPart.setHeader(QNetworkRequest::ContentDispositionHeader, u"form-data; name=\"model\""_s);
    modelPart.setBody(modelToUse.toUtf8());
    multiPart->append(modelPart);

    QHttpPart formatPart;
    formatPart.setHeader(QNetworkRequest::ContentDispositionHeader, u"form-data; name=\"response_format\""_s);
    formatPart.setBody("json");
    multiPart->append(formatPart);

    if (!langToUse.isEmpty()) {
        QHttpPart langPart;
        langPart.setHeader(QNetworkRequest::ContentDispositionHeader, u"form-data; name=\"language\""_s);
        langPart.setBody(langToUse.toUtf8());
        multiPart->append(langPart);
    }

    if (!promptToUse.isEmpty()) {
        QHttpPart promptPart;
        promptPart.setHeader(QNetworkRequest::ContentDispositionHeader, u"form-data; name=\"prompt\""_s);
        promptPart.setBody(promptToUse.toUtf8());
        multiPart->append(promptPart);
    }

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       u"form-data; name=\"file\"; filename=\"%1\""_s.arg(filenameToUse));
    filePart.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, u"audio/wav"_s);
    filePart.setBody(m_lastWavData);
    multiPart->append(filePart);

    QNetworkRequest request(QUrl(kApiEndpoint.toString()));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);
    NetworkTlsHelper::applySecureTls(request);
    request.setTransferTimeout(15s);

    QHttpHeaders headers;
    headers.append(QHttpHeaders::WellKnownHeader::UserAgent, groqUserAgent());
    headers.append(QHttpHeaders::WellKnownHeader::Authorization, "Bearer " + apiKey().toUtf8());
    request.setHeaders(headers);

    auto timer = std::make_shared<QElapsedTimer>();
    timer->start();

    m_currentReply = m_nam->post(request, multiPart);
    multiPart->setParent(m_currentReply);

    QPointer<GroqSttClient> self = this;
    connect(m_currentReply, &QNetworkReply::finished, this, [self, timer]() {
        if (!self) {
            return;
        }
        auto* reply = self->m_currentReply.data();
        if (!reply) {
            return;
        }
        reply->deleteLater();
        const qint64 elapsedMs = timer->elapsed();
        const CloudApiResponse response = CloudResponseParser::parseReply(reply, elapsedMs);
        self->handleApiResponse(response);
    });
}

bool GroqSttClient::shouldRetry(const CloudApiResponse& res) const {
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

int GroqSttClient::calculateRetryDelayMs(const CloudApiResponse& res) const {
    if (res.isRateLimited && res.retryAfterSeconds > 0) {
        return res.retryAfterSeconds * 1000;
    }
    return kDefaultDelayMs;
}

AbstractSttClient::ErrorCategory GroqSttClient::classifyError(const CloudApiResponse& res, QString& outMessage) const {
    ErrorCategory cat = ErrorCategory::GeneralError;
    outMessage = res.errorMessage;

    if (res.httpStatus == 401 || res.httpStatus == 403 ||
        outMessage.contains(u"API_KEY_INVALID"_s, Qt::CaseInsensitive) ||
        outMessage.contains(u"Invalid API Key"_s, Qt::CaseInsensitive) ||
        outMessage.contains(u"API key not valid"_s, Qt::CaseInsensitive)) {
        cat = ErrorCategory::InvalidApiKey;
        outMessage = u"Invalid API Key. Please check your Groq API key in Settings."_s;
    } else if (res.isRateLimited) {
        cat = ErrorCategory::RateLimited;
    } else if (res.networkError == QNetworkReply::HostNotFoundError ||
               res.networkError == QNetworkReply::ConnectionRefusedError ||
               res.networkError == QNetworkReply::TimeoutError ||
               res.networkError == QNetworkReply::NetworkSessionFailedError) {
        cat = ErrorCategory::NetworkOffline;
        outMessage = u"No internet connection. Please check your network and try again."_s;
    }

    return cat;
}

void GroqSttClient::handleApiResponse(const CloudApiResponse& res) {
    m_currentReply = nullptr;

    qCDebug(lcNetwork) << "Groq STT HTTP response received -> Status:" << res.httpStatus
                       << "Elapsed time:" << res.latencyMs << "ms";

    if (m_cancelled || res.networkError == QNetworkReply::OperationCanceledError) {
        qCDebug(lcNetwork) << "Groq STT: Request cancelled/aborted, ignoring response";
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
            qCDebug(lcNetwork) << "Groq STT: Transient error encountered (Status:" << res.httpStatus
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
        ErrorCategory cat = classifyError(res, errorText);

        if (cat == ErrorCategory::RateLimited && res.retryAfterSeconds > 0) {
            setRetrySecondsRemaining(res.retryAfterSeconds);
            m_retryCountdownTimer->start();
        }

        qWarning() << "Groq STT error:" << errorText << "Category:" << static_cast<int>(cat)
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

    const QString text = res.json.value(u"text"_s).toString();
    if (text.isEmpty()) {
        qWarning() << "Groq STT: Empty transcription text in response";
        const QString err = u"Groq API returned empty transcription"_s;
        setLastError(err, ErrorCategory::GeneralError);
        emit errorOccurred(err);
        return;
    }

    qCDebug(lcNetwork) << "Groq STT: Transcription successfully received -> Length:" << text.size() << "chars";

    setLastError({}, ErrorCategory::None);
    emit transcriptionReady(text);
}
