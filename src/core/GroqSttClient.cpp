#include "GroqSttClient.h"

#include "LoggingCategories.h"

#include "common/SettingsKeys.h"
#include "ApiKeyStore.h"
#include "CloudResponseParser.h"
#include "NetworkTlsHelper.h"
#include "SettingsManager.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHttpHeaders>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>

#include <memory>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

#ifndef QTSCRIBE_VERSION
#define QTSCRIBE_VERSION "0.0.0-dev"
#endif

namespace {
GroqSttClient* s_groqInstance = nullptr;

QString groqUserAgent() {
    const QString appVer = QCoreApplication::applicationVersion().isEmpty() ? u"" QTSCRIBE_VERSION ""_s
                                                                            : QCoreApplication::applicationVersion();
    return u"QtScribe/%1 (Linux; Qt %2)"_s.arg(appVer, QString::fromUtf8(QT_VERSION_STR));
}
} // namespace

GroqSttClient::GroqSttClient(QObject* parent)
    : BaseCloudSttClient(u"Groq STT"_s, u"Groq"_s, 15s, parent) {
    if (!s_groqInstance) {
        s_groqInstance = this;
    }

    auto* defaultStore = new ApiKeyStore(this);
    defaultStore->setStorageKeys(u"QtScribe"_s, u"groq_api_key"_s, SettingsKeys::kGroqSettingsKey.toString());
    setKeyStore(defaultStore);
    m_ownsKeyStore = true;

    if (auto* inst = SettingsManager::instance()) {
        setSettingsManager(inst);
    } else {
        loadSettings();
    }
}

GroqSttClient::~GroqSttClient() {
    if (s_groqInstance == this) {
        s_groqInstance = nullptr;
    }
}

GroqSttClient* GroqSttClient::instance() {
    return s_groqInstance;
}

void GroqSttClient::setSettingsManager(SettingsManager* settings) {
    if (m_settingsManager == settings) {
        return;
    }
    if (m_settingsManager) {
        disconnect(m_settingsManager, nullptr, this, nullptr);
    }
    m_settingsManager = settings;
    if (m_settingsManager) {
        connect(m_settingsManager, &SettingsManager::groqSttConfigChanged, this,
                [this](const SettingsManager::GroqSttConfig& config) {
                    if (m_selectedModel != config.model) {
                        m_selectedModel = config.model;
                        emit selectedModelChanged();
                    }
                    if (m_language != config.language) {
                        m_language = config.language;
                        emit languageChanged();
                    }
                    if (m_customPrompt != config.customPrompt) {
                        m_customPrompt = config.customPrompt;
                        emit customPromptChanged();
                    }
                });
    }
    loadSettings();
}

void GroqSttClient::loadSettings() {
    QString model = kDefaultModel.toString();
    QString lang;
    QString prompt;

    if (m_settingsManager) {
        const auto& config = m_settingsManager->groqSttConfig();
        model = config.model;
        lang = config.language;
        prompt = config.customPrompt;
    } else {
        const QSettings settings;
        model = settings.value(SettingsKeys::kGroqModel.toString(), kDefaultModel.toString()).toString();
        if (model.isEmpty() || (model != u"whisper-large-v3-turbo"_s && model != u"whisper-large-v3"_s)) {
            model = kDefaultModel.toString();
        }
        lang = settings.value(SettingsKeys::kGroqLanguage.toString(), QString()).toString();
        prompt = settings.value(SettingsKeys::kGroqCustomPrompt.toString(), QString()).toString();
    }

    if (m_selectedModel != model) {
        m_selectedModel = model;
        emit selectedModelChanged();
    }

    if (m_language != lang) {
        m_language = lang;
        emit languageChanged();
    }

    if (m_customPrompt != prompt) {
        m_customPrompt = prompt;
        emit customPromptChanged();
    }

    if (m_keyStore) {
        m_keyStore->loadApiKey();
    }
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
        if (m_settingsManager) {
            m_settingsManager->setGroqModel(m_selectedModel);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kGroqModel.toString(), m_selectedModel);
            settings.sync();
        }
        emit selectedModelChanged();
    }
}

QString GroqSttClient::language() const {
    return m_language;
}

void GroqSttClient::setLanguage(const QString& lang) {
    const QString trimmed = lang.trimmed();
    if (m_language != trimmed) {
        m_language = trimmed;
        if (m_settingsManager) {
            m_settingsManager->setGroqLanguage(m_language);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kGroqLanguage.toString(), m_language);
            settings.sync();
        }
        emit languageChanged();
    }
}

QString GroqSttClient::customPrompt() const {
    return m_customPrompt;
}

void GroqSttClient::setCustomPrompt(const QString& prompt) {
    if (m_customPrompt != prompt) {
        m_customPrompt = prompt;
        if (m_settingsManager) {
            m_settingsManager->setGroqCustomPrompt(m_customPrompt);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kGroqCustomPrompt.toString(), m_customPrompt);
            settings.sync();
        }
        emit customPromptChanged();
    }
}

void GroqSttClient::transcribe(const QByteArray& wavData, const QString& filename) {
    m_lastFilename = filename;
    transcribe(wavData);
}

void GroqSttClient::sendTranscribeRequest() {
    if (!isApiKeySet() || m_lastWavData.isEmpty() || m_cancelled) {
        setBusy(false);
        return;
    }

    setBusy(true);
    setLastError({});

    const QString modelToUse =
        m_selectedModel.trimmed().isEmpty() ? kDefaultModel.toString() : m_selectedModel.trimmed();
    const QString langToUse = m_language.trimmed();
    const QString promptToUse = m_customPrompt.trimmed();
    const QString filenameToUse = m_lastFilename.isEmpty() ? u"audio.wav"_s : m_lastFilename;

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

    const QPointer<GroqSttClient> self = this;
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

bool GroqSttClient::parseTranscript(const CloudApiResponse& res, QString& outText) {
    outText = res.json.value(u"text"_s).toString();
    return !outText.isEmpty();
}
