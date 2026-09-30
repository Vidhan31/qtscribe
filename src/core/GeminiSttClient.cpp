#include "GeminiSttClient.h"

#include "LoggingCategories.h"

#include "ApiKeyStore.h"
#include "CloudResponseParser.h"
#include "NetworkTlsHelper.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHttpHeaders>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>

#include <memory>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

#ifndef QTSCRIBE_VERSION
#define QTSCRIBE_VERSION "0.0.0-dev"
#endif

static QString geminiUserAgent() {
    const QString appVer = QCoreApplication::applicationVersion().isEmpty() ? u"" QTSCRIBE_VERSION ""_s
                                                                            : QCoreApplication::applicationVersion();
    return u"QtScribe/%1 (Linux; Qt %2)"_s.arg(appVer, QString::fromUtf8(QT_VERSION_STR));
}

GeminiSttClient::GeminiSttClient(QObject* parent)
    : AbstractSttClient(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_retryCountdownTimer(new QTimer(this))
    , m_retryTimer(new QTimer(this)) {
    NetworkTlsHelper::ensureSystemProxyConfigured();

    m_nam->setTransferTimeout(20s);
    connect(m_nam, &QNetworkAccessManager::sslErrors, this,
            [](QNetworkReply* /*reply*/, const QList<QSslError>& errors) {
                for (const auto& err : errors) {
                    qWarning() << "GeminiSttClient SSL Error:" << err.errorString();
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
    defaultStore->setStorageKeys(u"QtScribe"_s, u"gemini_api_key"_s, u"Cloud/Providers/Gemini/ApiKey"_s);
    setKeyStore(defaultStore);
    m_ownsKeyStore = true;

    QSettings settings;
    m_selectedModel = settings.value(u"Gemini/Model"_s, kDefaultModel.toString()).toString();
    if (m_selectedModel.trimmed().isEmpty()) {
        m_selectedModel = kDefaultModel.toString();
    }
    m_mode = settings.value(u"Gemini/Mode"_s, kDefaultMode.toString()).toString();
    if (m_mode != u"smart"_s && m_mode != u"verbatim"_s) {
        m_mode = kDefaultMode.toString();
    }
    m_customVocabulary = settings.value(u"Gemini/CustomVocabulary"_s, QString()).toString();
}

void GeminiSttClient::setKeyStore(ApiKeyStore* store) {
    if (m_keyStore == store) {
        return;
    }

    const bool wasApiKeySet = isApiKeySet();
    const bool wasKeyInvalid = isApiKeyInvalid();

    if (m_keyStore) {
        disconnect(m_keyStore, &ApiKeyStore::apiKeyChanged, this, &GeminiSttClient::apiKeyChanged);
        disconnect(m_keyStore, &ApiKeyStore::apiKeySetChanged, this, &GeminiSttClient::onApiKeySetChanged);
        if (m_ownsKeyStore && m_keyStore->parent() == this) {
            m_keyStore->deleteLater();
        }
    }

    m_keyStore = store;
    m_ownsKeyStore = false;

    if (m_keyStore) {
        connect(m_keyStore, &ApiKeyStore::apiKeyChanged, this, &GeminiSttClient::apiKeyChanged);
        connect(m_keyStore, &ApiKeyStore::apiKeySetChanged, this, &GeminiSttClient::onApiKeySetChanged);
    }

    if (isApiKeySet() != wasApiKeySet) {
        emit apiKeySetChanged();
        emit readyChanged();
    }
    if (isApiKeyInvalid() != wasKeyInvalid) {
        emit isApiKeyInvalidChanged();
    }
}

ApiKeyStore* GeminiSttClient::keyStore() const {
    return m_keyStore;
}

QString GeminiSttClient::apiKey() const {
    return m_keyStore ? m_keyStore->apiKey() : QString();
}

void GeminiSttClient::setApiKey(const QString& key) {
    if (m_keyStore) {
        m_keyStore->setApiKey(key);
    }
}

bool GeminiSttClient::isApiKeySet() const {
    return m_keyStore && m_keyStore->apiKeySet();
}

bool GeminiSttClient::isApiKeyInvalid() const {
    return isApiKeySet() && m_errorCategory == ErrorCategory::InvalidApiKey;
}

bool GeminiSttClient::isRateLimited() const {
    return m_errorCategory == ErrorCategory::RateLimited && m_retrySecondsRemaining > 0;
}

int GeminiSttClient::retrySecondsRemaining() const {
    return m_retrySecondsRemaining;
}

AbstractSttClient::ErrorCategory GeminiSttClient::errorCategory() const {
    return m_errorCategory;
}

QString GeminiSttClient::lastError() const {
    return m_lastError;
}

bool GeminiSttClient::isReady() const {
    return isApiKeySet();
}

bool GeminiSttClient::isBusy() const {
    return m_busy;
}

bool GeminiSttClient::isCancelled() const {
    return m_cancelled;
}

bool GeminiSttClient::handlesSmartFormatting() const {
    return m_mode == u"smart"_s;
}

QString GeminiSttClient::selectedModel() const {
    return m_selectedModel;
}

void GeminiSttClient::setSelectedModel(const QString& model) {
    QString trimmed = model.trimmed();
    if (trimmed.isEmpty()) {
        trimmed = kDefaultModel.toString();
    }
    if (m_selectedModel != trimmed) {
        m_selectedModel = trimmed;
        QSettings settings;
        settings.setValue(u"Gemini/Model"_s, m_selectedModel);
        emit selectedModelChanged();
    }
}

QString GeminiSttClient::mode() const {
    return m_mode;
}

void GeminiSttClient::setMode(const QString& mode) {
    QString trimmed = mode.trimmed().toLower();
    if (trimmed != u"smart"_s && trimmed != u"verbatim"_s) {
        trimmed = kDefaultMode.toString();
    }
    if (m_mode != trimmed) {
        m_mode = trimmed;
        QSettings settings;
        settings.setValue(u"Gemini/Mode"_s, m_mode);
        emit modeChanged();
    }
}

QString GeminiSttClient::customVocabulary() const {
    return m_customVocabulary;
}

void GeminiSttClient::setCustomVocabulary(const QString& vocab) {
    if (m_customVocabulary != vocab) {
        m_customVocabulary = vocab;
        QSettings settings;
        settings.setValue(u"Gemini/CustomVocabulary"_s, m_customVocabulary);
        emit customVocabularyChanged();
    }
}

void GeminiSttClient::activate() {
    if (m_keyStore) {
        m_keyStore->ensureApiKeyLoaded();
    }
    emit readyChanged();
}

void GeminiSttClient::deactivate() {
    cancel();
}

void GeminiSttClient::onApiKeySetChanged() {
    const bool wasKeyInvalid = isApiKeyInvalid();
    emit apiKeySetChanged();
    if (isApiKeyInvalid() != wasKeyInvalid) {
        emit isApiKeyInvalidChanged();
    }
    emit readyChanged();
}

void GeminiSttClient::setBusy(bool busy) {
    if (m_busy != busy) {
        m_busy = busy;
        emit busyChanged();
    }
}

void GeminiSttClient::setLastError(const QString& error, ErrorCategory category) {
    if (m_lastError != error) {
        m_lastError = error;
        emit lastErrorChanged();
    }
    setErrorCategory(category);
}

void GeminiSttClient::setErrorCategory(ErrorCategory category) {
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

void GeminiSttClient::setRetrySecondsRemaining(int seconds) {
    if (m_retrySecondsRemaining != seconds) {
        const bool wasRateLimited = isRateLimited();
        m_retrySecondsRemaining = seconds;
        emit retrySecondsRemainingChanged();
        if (isRateLimited() != wasRateLimited) {
            emit isRateLimitedChanged();
        }
    }
}

void GeminiSttClient::cancel() {
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

void GeminiSttClient::retryLast() {
    if (!m_lastWavData.isEmpty() && !isBusy()) {
        if (m_retryTimer) {
            m_retryTimer->stop();
        }
        m_cancelled = false;
        m_retryCount = 0;
        sendTranscribeRequest();
    }
}

void GeminiSttClient::transcribe(const QByteArray& wavData) {
    if (isBusy()) {
        qCDebug(lcNetwork) << "Gemini STT: transcribe ignored, request already in progress";
        setLastError(u"A transcription request is already in progress"_s, ErrorCategory::GeneralError);
        return;
    }

    if (!isApiKeySet()) {
        qWarning() << "Gemini STT: Attempted transcribe without an API key";
        setLastError(u"Gemini API key is not set"_s, ErrorCategory::InvalidApiKey);
        emit errorOccurred(m_lastError);
        return;
    }

    if (wavData.isEmpty()) {
        qWarning() << "Gemini STT: Attempted transcribe with empty audio data";
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

void GeminiSttClient::sendTranscribeRequest() {
    if (!isApiKeySet() || m_lastWavData.isEmpty() || m_cancelled) {
        setBusy(false);
        return;
    }

    setBusy(true);
    setLastError({});

    const QString modelToUse =
        m_selectedModel.trimmed().isEmpty() ? kDefaultModel.toString() : m_selectedModel.trimmed();
    const QString modeToUse = (m_mode == u"verbatim"_s) ? u"verbatim"_s : u"smart"_s;

    qCDebug(lcNetwork) << "Preparing Gemini STT request -> Model:" << modelToUse << "Mode:" << modeToUse
                       << "Audio size:" << m_lastWavData.size() << "bytes"
                       << "(Retry attempt:" << m_retryCount << ")";

    QJsonObject rootObj;
    rootObj.insert(u"model"_s, modelToUse);

    QJsonObject audioItem;
    audioItem.insert(u"type"_s, u"audio"_s);
    audioItem.insert(u"data"_s, QString::fromUtf8(m_lastWavData.toBase64()));
    audioItem.insert(u"mime_type"_s, u"audio/wav"_s);

    QJsonArray inputArr;
    inputArr.append(audioItem);
    rootObj.insert(u"input"_s, inputArr);

    QJsonObject transcriptionConfig;
    if (modeToUse == u"smart"_s) {
        transcriptionConfig.insert(u"mode"_s, u"smart"_s);
    } else {
        QJsonObject verbatimMode;
        verbatimMode.insert(u"type"_s, u"verbatim"_s);
        transcriptionConfig.insert(u"mode"_s, verbatimMode);
    }

    if (!m_customVocabulary.trimmed().isEmpty()) {
        QJsonArray vocabArr;
        const auto lines = m_customVocabulary.split(QRegularExpression(u"[,;\n\r]+"_s), Qt::SkipEmptyParts);
        for (const auto& token : lines) {
            const QString item = token.trimmed();
            if (!item.isEmpty()) {
                vocabArr.append(item);
            }
        }
        if (!vocabArr.isEmpty()) {
            transcriptionConfig.insert(u"custom_vocabulary"_s, vocabArr);
        }
    }

    QJsonObject generationConfig;
    generationConfig.insert(u"transcription_config"_s, transcriptionConfig);
    rootObj.insert(u"generation_config"_s, generationConfig);

    const QByteArray payload = QJsonDocument(rootObj).toJson(QJsonDocument::Compact);

    QNetworkRequest request(QUrl(kApiEndpoint.toString()));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);
    NetworkTlsHelper::applySecureTls(request);
    request.setTransferTimeout(20s);

    QHttpHeaders headers;
    headers.append(QHttpHeaders::WellKnownHeader::UserAgent, geminiUserAgent());
    headers.append(QHttpHeaders::WellKnownHeader::ContentType, u"application/json"_s);
    headers.append(QByteArrayView("x-goog-api-key"), apiKey().toUtf8());
    request.setHeaders(headers);

    auto timer = std::make_shared<QElapsedTimer>();
    timer->start();

    m_currentReply = m_nam->post(request, payload);

    QPointer<GeminiSttClient> self = this;
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

bool GeminiSttClient::shouldRetry(const CloudApiResponse& res) const {
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

int GeminiSttClient::calculateRetryDelayMs(const CloudApiResponse& res) const {
    if (res.isRateLimited && res.retryAfterSeconds > 0) {
        return res.retryAfterSeconds * 1000;
    }
    return kDefaultDelayMs;
}

AbstractSttClient::ErrorCategory GeminiSttClient::classifyError(const CloudApiResponse& res,
                                                                QString& outMessage) const {
    ErrorCategory cat = ErrorCategory::GeneralError;
    outMessage = res.errorMessage;

    if (res.httpStatus == 401 || res.httpStatus == 403 ||
        outMessage.contains(u"API_KEY_INVALID"_s, Qt::CaseInsensitive) ||
        outMessage.contains(u"Invalid API Key"_s, Qt::CaseInsensitive) ||
        outMessage.contains(u"API key not valid"_s, Qt::CaseInsensitive)) {
        cat = ErrorCategory::InvalidApiKey;
        outMessage = u"Invalid API Key. Please check your Gemini API key in Settings."_s;
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

void GeminiSttClient::handleApiResponse(const CloudApiResponse& res) {
    m_currentReply = nullptr;

    qCDebug(lcNetwork) << "Gemini STT HTTP response received -> Status:" << res.httpStatus
                       << "Elapsed time:" << res.latencyMs << "ms";

    if (m_cancelled || res.networkError == QNetworkReply::OperationCanceledError) {
        qCDebug(lcNetwork) << "Gemini STT: Request cancelled/aborted, ignoring response";
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
            qCDebug(lcNetwork) << "Gemini STT: Transient error encountered (Status:" << res.httpStatus
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

        qWarning() << "Gemini STT error:" << errorText << "Category:" << static_cast<int>(cat)
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
    const QJsonObject& json = res.json;
    if (json.contains(u"output_text"_s)) {
        text = json.value(u"output_text"_s).toString().trimmed();
    }

    if (text.isEmpty() && json.contains(u"steps"_s)) {
        const QJsonArray steps = json.value(u"steps"_s).toArray();
        QStringList extractedParts;
        for (const auto& stepVal : steps) {
            const QJsonObject stepObj = stepVal.toObject();
            if (stepObj.value(u"type"_s).toString() == u"model_output"_s && stepObj.contains(u"content"_s)) {
                const QJsonArray contentArr = stepObj.value(u"content"_s).toArray();
                for (const auto& contentVal : contentArr) {
                    const QJsonObject contentObj = contentVal.toObject();
                    if (contentObj.value(u"type"_s).toString() == u"text"_s) {
                        extractedParts.append(contentObj.value(u"text"_s).toString());
                    }
                }
            }
        }
        text = extractedParts.join(u"\n"_s).trimmed();
    }

    if (text.isEmpty()) {
        qWarning() << "Gemini STT: Empty transcription text in response";
        const QString err = u"Gemini API returned empty transcription"_s;
        setLastError(err, ErrorCategory::GeneralError);
        emit errorOccurred(err);
        return;
    }

    qCDebug(lcNetwork) << "Gemini STT: Transcription successfully received -> Length:" << text.size() << "chars";

    setLastError({}, ErrorCategory::None);
    emit transcriptionReady(text);
}
