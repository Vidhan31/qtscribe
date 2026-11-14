#include "MetaMusaSttClient.h"

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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSettings>

#include <memory>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

#ifndef QTSCRIBE_VERSION
#define QTSCRIBE_VERSION "0.0.0-dev"
#endif

namespace {
MetaMusaSttClient* s_metaMusaInstance = nullptr;

QString musaUserAgent() {
    const QString appVer = QCoreApplication::applicationVersion().isEmpty() ? u"" QTSCRIBE_VERSION ""_s
                                                                            : QCoreApplication::applicationVersion();
    return u"QtScribe/%1 (Linux; Qt %2)"_s.arg(appVer, QString::fromUtf8(QT_VERSION_STR));
}
} // namespace

MetaMusaSttClient::MetaMusaSttClient(QObject* parent)
    : BaseCloudSttClient(u"Meta Musa STT"_s, u"Meta Musa"_s, 15s, parent) {
    if (!s_metaMusaInstance) {
        s_metaMusaInstance = this;
    }

    auto* defaultStore = new ApiKeyStore(this);
    defaultStore->setStorageKeys(u"QtScribe"_s, u"meta_musa_api_key"_s, SettingsKeys::kMetaMusaSettingsKey.toString());
    setKeyStore(defaultStore);
    m_ownsKeyStore = true;

    if (auto* inst = SettingsManager::instance()) {
        setSettingsManager(inst);
    } else {
        loadSettings();
    }
}

MetaMusaSttClient::~MetaMusaSttClient() {
    if (s_metaMusaInstance == this) {
        s_metaMusaInstance = nullptr;
    }
}

MetaMusaSttClient* MetaMusaSttClient::instance() {
    return s_metaMusaInstance;
}

void MetaMusaSttClient::setSettingsManager(SettingsManager* settings) {
    if (m_settingsManager == settings) {
        return;
    }
    if (m_settingsManager) {
        disconnect(m_settingsManager, nullptr, this, nullptr);
    }
    m_settingsManager = settings;
    if (m_settingsManager) {
        connect(m_settingsManager, &SettingsManager::metaMusaSttConfigChanged, this,
                [this](const SettingsManager::MetaMusaSttConfig& config) {
                    if (m_selectedModel != config.model) {
                        m_selectedModel = config.model;
                        emit selectedModelChanged();
                    }
                    if (m_keywords != config.keywords) {
                        m_keywords = config.keywords;
                        emit keywordsChanged();
                    }
                });
    }
    loadSettings();
}

void MetaMusaSttClient::loadSettings() {
    QString model = kDefaultModel.toString();
    QString kw;

    if (m_settingsManager) {
        const auto& config = m_settingsManager->metaMusaSttConfig();
        model = config.model;
        kw = config.keywords;
    } else {
        const QSettings settings;
        model = settings.value(SettingsKeys::kMetaMusaModel.toString(), kDefaultModel.toString()).toString();
        if (model.trimmed().isEmpty()) {
            model = kDefaultModel.toString();
        }
        kw = settings.value(SettingsKeys::kMetaMusaKeywords.toString(), QString()).toString();
    }

    if (m_selectedModel != model) {
        m_selectedModel = model;
        emit selectedModelChanged();
    }

    if (m_keywords != kw) {
        m_keywords = kw;
        emit keywordsChanged();
    }

    if (m_keyStore) {
        m_keyStore->loadApiKey();
    }
}

bool MetaMusaSttClient::handlesSmartFormatting() const {
    return true;
}

QString MetaMusaSttClient::selectedModel() const {
    return m_selectedModel;
}

void MetaMusaSttClient::setSelectedModel(const QString& model) {
    QString trimmed = model.trimmed();
    if (trimmed.isEmpty()) {
        trimmed = kDefaultModel.toString();
    }
    if (m_selectedModel != trimmed) {
        m_selectedModel = trimmed;
        if (m_settingsManager) {
            m_settingsManager->setMetaMusaModel(m_selectedModel);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kMetaMusaModel.toString(), m_selectedModel);
            settings.sync();
        }
        emit selectedModelChanged();
    }
}

QString MetaMusaSttClient::keywords() const {
    return m_keywords;
}

void MetaMusaSttClient::setKeywords(const QString& keywords) {
    if (m_keywords != keywords) {
        m_keywords = keywords;
        if (m_settingsManager) {
            m_settingsManager->setMetaMusaKeywords(m_keywords);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kMetaMusaKeywords.toString(), m_keywords);
            settings.sync();
        }
        emit keywordsChanged();
    }
}

void MetaMusaSttClient::sendTranscribeRequest() {
    if (!isApiKeySet() || m_lastWavData.isEmpty() || m_cancelled) {
        setBusy(false);
        return;
    }

    setBusy(true);
    setLastError({});

    const QString modelToUse =
        m_selectedModel.trimmed().isEmpty() ? kDefaultModel.toString() : m_selectedModel.trimmed();

    qCDebug(lcNetwork) << "Preparing Meta Musa STT multipart request -> Model:" << modelToUse
                       << "Keywords set:" << (!m_keywords.trimmed().isEmpty()) << "Audio size:" << m_lastWavData.size()
                       << "bytes"
                       << "(Retry attempt:" << m_retryCount << ")";

    QJsonObject requestObj;
    requestObj.insert(u"model"_s, modelToUse);
    requestObj.insert(u"audioEncoding"_s, u"WAV"_s);
    requestObj.insert(u"mode"_s, u"PUSH_TO_TALK"_s);

    if (!m_keywords.trimmed().isEmpty()) {
        QJsonArray keywordsArr;
        const auto lines = m_keywords.split(QRegularExpression(u"[,;\n\r]+"_s), Qt::SkipEmptyParts);
        for (const auto& token : lines) {
            const QString item = token.trimmed();
            if (!item.isEmpty()) {
                keywordsArr.append(item);
            }
        }
        if (!keywordsArr.isEmpty()) {
            requestObj.insert(u"keywords"_s, keywordsArr);
        }
    }

    auto* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart requestPart;
    requestPart.setHeader(QNetworkRequest::ContentDispositionHeader, u"form-data; name=\"request\""_s);
    requestPart.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, u"application/json"_s);
    requestPart.setBody(QJsonDocument(requestObj).toJson(QJsonDocument::Compact));
    multiPart->append(requestPart);

    QHttpPart audioPart;
    audioPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        u"form-data; name=\"audio\"; filename=\"audio.wav\""_s);
    audioPart.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, u"audio/wav"_s);
    audioPart.setBody(m_lastWavData);
    multiPart->append(audioPart);

    QNetworkRequest request(QUrl(kApiEndpoint.toString()));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::SameOriginRedirectPolicy);
    NetworkTlsHelper::applySecureTls(request);
    request.setTransferTimeout(15s);

    QHttpHeaders headers;
    headers.append(QHttpHeaders::WellKnownHeader::UserAgent, musaUserAgent());
    headers.append(QHttpHeaders::WellKnownHeader::Authorization, "Bearer " + apiKey().toUtf8());
    headers.append(QHttpHeaders::WellKnownHeader::Accept, u"application/json"_s);
    request.setHeaders(headers);

    auto timer = std::make_shared<QElapsedTimer>();
    timer->start();

    m_currentReply = m_nam->post(request, multiPart);
    multiPart->setParent(m_currentReply);

    const QPointer<MetaMusaSttClient> self = this;
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

bool MetaMusaSttClient::parseTranscript(const CloudApiResponse& res, QString& outText) {
    outText = res.json.value(u"transcript"_s).toString().trimmed();
    return !outText.isEmpty();
}
