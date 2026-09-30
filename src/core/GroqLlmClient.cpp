#include "GroqLlmClient.h"

#include "LoggingCategories.h"

#include "ApiKeyStore.h"
#include "CloudResponseParser.h"
#include "NetworkTlsHelper.h"
#include "PresetProvider.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHttpHeaders>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QTimer>

#include <algorithm>
#include <memory>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

#ifndef QTSCRIBE_VERSION
#define QTSCRIBE_VERSION "0.0.0-dev"
#endif

static QString groqLlmUserAgent() {
    const QString appVer = QCoreApplication::applicationVersion().isEmpty() ? u"" QTSCRIBE_VERSION ""_s
                                                                            : QCoreApplication::applicationVersion();
    return u"QtScribe/%1 (Linux; Qt %2)"_s.arg(appVer, QString::fromUtf8(QT_VERSION_STR));
}

GroqLlmClient::GroqLlmClient(QObject* parent)
    : AbstractLlmClient(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_retryTimer(new QTimer(this)) {
    NetworkTlsHelper::ensureSystemProxyConfigured();
    m_nam->setTransferTimeout(15s);
    QSettings settings;
    m_enabled = settings.value(u"Groq/LlmEnabled"_s, false).toBool();
    m_selectedModel = settings.value(u"Groq/LlmModel"_s, kDefaultModel.toString()).toString();
    if (m_selectedModel.isEmpty() || m_selectedModel == u"llama-3.1-8b-instant"_s) {
        m_selectedModel = kDefaultModel.toString();
    }
    m_activePreset = settings.value(u"Groq/LlmPreset"_s, u"grammar"_s).toString();
    if (m_activePreset.isEmpty()) {
        m_activePreset = u"grammar"_s;
    }
    m_customPrompt = settings.value(u"Groq/LlmCustomPrompt"_s, QString()).toString();
    m_temperature = settings.value(u"Groq/LlmTemperature"_s, 0.1).toDouble();

    m_retryTimer->setSingleShot(true);
    connect(m_retryTimer, &QTimer::timeout, this, [this]() {
        if (!m_requestRunner.isCancelled() && !m_pendingRawText.isEmpty()) {
            sendProcessRequest();
        }
    });
}

void GroqLlmClient::setKeyStore(ApiKeyStore* keyStore) {
    m_keyStore = keyStore;
}

ApiKeyStore* GroqLlmClient::keyStore() const {
    return m_keyStore;
}

bool GroqLlmClient::isBusy() const {
    return m_requestRunner.isBusy();
}

bool GroqLlmClient::isCancelled() const {
    return m_requestRunner.isCancelled();
}

void GroqLlmClient::setBusy(bool busy) {
    if (m_requestRunner.isBusy() != busy) {
        m_requestRunner.setBusy(busy);
        emit busyChanged();
    }
}

bool GroqLlmClient::enabled() const {
    return m_enabled;
}

void GroqLlmClient::setEnabled(bool enabled) {
    if (m_enabled != enabled) {
        m_enabled = enabled;
        QSettings settings;
        settings.setValue(u"Groq/LlmEnabled"_s, m_enabled);
        qCDebug(lcLLM) << "LLM post-processing enabled state changed to:" << m_enabled;
        emit enabledChanged();
    }
}

QString GroqLlmClient::lastError() const {
    return m_lastError;
}

QString GroqLlmClient::selectedModel() const {
    return m_selectedModel;
}

void GroqLlmClient::setSelectedModel(const QString& model) {
    QString trimmed = model.trimmed();
    if (trimmed.isEmpty()) {
        trimmed = kDefaultModel.toString();
    }
    if (m_selectedModel != trimmed) {
        m_selectedModel = trimmed;
        QSettings settings;
        settings.setValue(u"Groq/LlmModel"_s, m_selectedModel);
        qCDebug(lcLLM) << "LLM model set to:" << m_selectedModel;
        emit selectedModelChanged();
    }
}

QString GroqLlmClient::activePreset() const {
    return m_activePreset;
}

void GroqLlmClient::setActivePreset(const QString& preset) {
    QString trimmed = preset.trimmed();
    if (trimmed.isEmpty()) {
        trimmed = u"grammar"_s;
    }
    if (m_activePreset != trimmed) {
        m_activePreset = trimmed;
        QSettings settings;
        settings.setValue(u"Groq/LlmPreset"_s, m_activePreset);
        qCDebug(lcLLM) << "Active LLM preset changed to:" << m_activePreset;
        emit activePresetChanged();
    }
}

QString GroqLlmClient::customPrompt() const {
    return m_customPrompt;
}

void GroqLlmClient::setCustomPrompt(const QString& prompt) {
    const QString trimmed = prompt.trimmed();
    if (m_customPrompt != trimmed) {
        m_customPrompt = trimmed;
        QSettings settings;
        settings.setValue(u"Groq/LlmCustomPrompt"_s, m_customPrompt);
        qCDebug(lcLLM) << "Custom LLM prompt updated";
        emit customPromptChanged();
    }
}

double GroqLlmClient::temperature() const {
    return m_temperature;
}

void GroqLlmClient::setTemperature(double temp) {
    temp = std::clamp(temp, 0.0, 1.0);
    if (!qFuzzyCompare(m_temperature, temp)) {
        m_temperature = temp;
        QSettings settings;
        settings.setValue(u"Groq/LlmTemperature"_s, m_temperature);
        emit temperatureChanged();
    }
}

QString GroqLlmClient::formattedTemperature() const {
    return QString::number(m_temperature, 'f', 2);
}

QString GroqLlmClient::systemPromptForPreset(const QString& preset) const {
    return PresetProvider::systemPromptForPreset(preset, m_customPrompt);
}

QString GroqLlmClient::currentSystemPrompt() const {
    return PresetProvider::systemPromptForPreset(m_activePreset, m_customPrompt);
}

void GroqLlmClient::cancel() {
    const bool wasBusy = m_requestRunner.isBusy();
    if (m_retryTimer) {
        m_retryTimer->stop();
    }
    m_requestRunner.cancel();
    m_pendingRawText.clear();
    if (wasBusy) {
        emit busyChanged();
    }
}

void GroqLlmClient::processText(const QString& rawText) {
    if (isBusy()) {
        qCDebug(lcLLM) << "processText ignored: request already in progress";
        setLastError(u"An LLM post-processing request is already in progress"_s);
        emit errorOccurred(m_lastError, rawText);
        return;
    }

    if (!m_keyStore || !m_keyStore->apiKeySet()) {
        qWarning() << "GroqLlmClient: API key not set, falling back to raw transcription";
        setLastError(u"Groq API key is not set"_s);
        emit errorOccurred(m_lastError, rawText);
        return;
    }

    if (rawText.trimmed().isEmpty()) {
        qCDebug(lcLLM) << "GroqLlmClient: Empty input text, emitting ready directly";
        emit enhancementReady(rawText);
        return;
    }

    if (m_retryTimer) {
        m_retryTimer->stop();
    }
    m_requestRunner.prepareNewRequest();
    m_pendingRawText = rawText;

    sendProcessRequest();
}

void GroqLlmClient::sendProcessRequest() {
    if (!m_keyStore || !m_keyStore->apiKeySet() || m_pendingRawText.trimmed().isEmpty() ||
        m_requestRunner.isCancelled()) {
        setBusy(false);
        return;
    }

    setBusy(true);
    setLastError({});

    QString modelToUse = m_selectedModel.trimmed().isEmpty() ? kDefaultModel.toString() : m_selectedModel.trimmed();
    QString systemPrompt = currentSystemPrompt();

    qCDebug(lcLLM) << "Dispatching Groq LLM completion request -> Model:" << modelToUse << "Preset:" << m_activePreset
                   << "Input length:" << m_pendingRawText.size() << "chars"
                   << "(Retry attempt:" << m_requestRunner.retryCount() << ")";

    QJsonObject rootObj;
    rootObj[u"model"_s] = modelToUse;

    QJsonArray messages;
    QJsonObject sysMsg;
    sysMsg[u"role"_s] = u"system"_s;
    sysMsg[u"content"_s] = systemPrompt;
    messages.append(sysMsg);

    QJsonObject userMsg;
    userMsg[u"role"_s] = u"user"_s;
    userMsg[u"content"_s] = m_pendingRawText;
    messages.append(userMsg);

    rootObj[u"messages"_s] = messages;
    rootObj[u"temperature"_s] = m_temperature;
    rootObj[u"max_completion_tokens"_s] = 2048;
    rootObj[u"reasoning_format"_s] = u"hidden"_s;
    rootObj[u"stream"_s] = false;

    QNetworkRequest request(QUrl(u"https://api.groq.com/openai/v1/chat/completions"_s));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);
    NetworkTlsHelper::applySecureTls(request);
    request.setTransferTimeout(15s);

    QHttpHeaders headers;
    headers.append(QHttpHeaders::WellKnownHeader::UserAgent, groqLlmUserAgent());
    headers.append(QHttpHeaders::WellKnownHeader::ContentType, u"application/json"_s);
    headers.append(QHttpHeaders::WellKnownHeader::Authorization, "Bearer " + m_keyStore->apiKey().toUtf8());
    request.setHeaders(headers);

    auto timer = std::make_shared<QElapsedTimer>();
    timer->start();

    auto* reply = m_nam->post(request, QJsonDocument(rootObj).toJson(QJsonDocument::Compact));
    m_requestRunner.setCurrentReply(reply);

    QPointer<GroqLlmClient> self = this;
    connect(reply, &QNetworkReply::finished, this, [self, reply, timer]() {
        if (!self) {
            return;
        }
        reply->deleteLater();
        const qint64 elapsedMs = timer->elapsed();
        const CloudApiResponse response = CloudResponseParser::parseReply(reply, elapsedMs);
        self->handleProcessResponse(response);
    });
}

void GroqLlmClient::handleProcessResponse(const CloudApiResponse& res) {
    m_requestRunner.setCurrentReply(nullptr);

    qCDebug(lcLLM) << "Groq LLM HTTP response received -> Status:" << res.httpStatus << "Elapsed time:" << res.latencyMs
                   << "ms";

    if (m_requestRunner.isCancelled() || res.networkError == QNetworkReply::OperationCanceledError) {
        qCDebug(lcLLM) << "GroqLlmClient: Request cancelled/aborted, ignoring response";
        if (m_retryTimer) {
            m_retryTimer->stop();
        }
        setBusy(false);
        return;
    }

    if (!res.isSuccess) {
        if (m_requestRunner.shouldRetry(res) && !m_pendingRawText.isEmpty()) {
            m_requestRunner.incrementRetryCount();
            const int delayMs = m_requestRunner.calculateRetryDelayMs(res);
            qCDebug(lcLLM) << "GroqLlmClient: Transient error encountered (Status:" << res.httpStatus
                           << "Error:" << res.networkError << "). Scheduling retry in" << delayMs << "ms (Attempt"
                           << m_requestRunner.retryCount() << "/" << m_requestRunner.policy().maxRetries << ")";
            if (m_retryTimer) {
                m_retryTimer->start(delayMs);
            }
            return;
        }

        const QString rawFallback = m_pendingRawText;
        if (m_retryTimer) {
            m_retryTimer->stop();
        }
        m_requestRunner.reset();
        m_pendingRawText.clear();
        setBusy(false);

        const QString errorText = res.errorMessage;
        qWarning() << "GroqLlmClient error:" << errorText
                   << "Raw body:" << NetworkTlsHelper::redactedBodyPreview(res.rawBody);
        setLastError(errorText);
        emit errorOccurred(errorText, rawFallback);
        return;
    }

    const QString rawFallback = m_pendingRawText;
    if (m_retryTimer) {
        m_retryTimer->stop();
    }
    m_requestRunner.reset();
    m_pendingRawText.clear();
    setBusy(false);

    const QJsonArray choices = res.json.value(u"choices"_s).toArray();
    if (choices.isEmpty()) {
        qWarning() << "GroqLlmClient: No choices returned in LLM completion";
        const QString err = u"Empty completion choices from Groq LLM API"_s;
        setLastError(err);
        emit errorOccurred(err, rawFallback);
        return;
    }

    const QJsonObject choice0 = choices.first().toObject();
    const QJsonObject message = choice0.value(u"message"_s).toObject();
    QString enhancedText = message.value(u"content"_s).toString().trimmed();

    if (enhancedText.isEmpty()) {
        qWarning() << "GroqLlmClient: LLM returned empty text content, using fallback";
        enhancedText = rawFallback;
    }

    qCDebug(lcLLM) << "LLM enhancement succeeded -> Length:" << enhancedText.size() << "chars";

    setLastError({});
    emit enhancementReady(enhancedText);
}

void GroqLlmClient::setLastError(const QString& error) {
    if (m_lastError != error) {
        m_lastError = error;
        emit lastErrorChanged();
    }
}
