#include "GeminiSttClient.h"

#include "LoggingCategories.h"

#include "common/SettingsKeys.h"
#include "ApiKeyStore.h"
#include "CloudResponseParser.h"
#include "NetworkTlsHelper.h"
#include "SettingsManager.h"

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

#include <memory>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

#ifndef QTSCRIBE_VERSION
#define QTSCRIBE_VERSION "0.0.0-dev"
#endif

namespace {
GeminiSttClient* s_geminiInstance = nullptr;

QString geminiUserAgent() {
    const QString appVer = QCoreApplication::applicationVersion().isEmpty() ? u"" QTSCRIBE_VERSION ""_s
                                                                            : QCoreApplication::applicationVersion();
    return u"QtScribe/%1 (Linux; Qt %2)"_s.arg(appVer, QString::fromUtf8(QT_VERSION_STR));
}
} // namespace

GeminiSttClient::GeminiSttClient(QObject* parent)
    : BaseCloudSttClient(u"Gemini STT"_s, u"Gemini"_s, 20s, parent) {
    if (!s_geminiInstance) {
        s_geminiInstance = this;
    }

    auto* defaultStore = new ApiKeyStore(this);
    defaultStore->setStorageKeys(u"QtScribe"_s, u"gemini_api_key"_s, SettingsKeys::kGeminiSettingsKey.toString());
    setKeyStore(defaultStore);
    m_ownsKeyStore = true;

    if (auto* inst = SettingsManager::instance()) {
        setSettingsManager(inst);
    } else {
        loadSettings();
    }
}

GeminiSttClient::~GeminiSttClient() {
    if (s_geminiInstance == this) {
        s_geminiInstance = nullptr;
    }
}

GeminiSttClient* GeminiSttClient::instance() {
    return s_geminiInstance;
}

void GeminiSttClient::setSettingsManager(SettingsManager* settings) {
    if (m_settingsManager == settings) {
        return;
    }
    if (m_settingsManager) {
        disconnect(m_settingsManager, nullptr, this, nullptr);
    }
    m_settingsManager = settings;
    if (m_settingsManager) {
        connect(m_settingsManager, &SettingsManager::geminiSttConfigChanged, this,
                [this](const SettingsManager::GeminiSttConfig& config) {
                    if (m_selectedModel != config.model) {
                        m_selectedModel = config.model;
                        emit selectedModelChanged();
                    }
                    if (m_mode != config.mode) {
                        m_mode = config.mode;
                        emit modeChanged();
                    }
                    if (m_customVocabulary != config.customVocabulary) {
                        m_customVocabulary = config.customVocabulary;
                        emit customVocabularyChanged();
                    }
                });
    }
    loadSettings();
}

void GeminiSttClient::loadSettings() {
    QString model = kDefaultModel.toString();
    QString modeVal = kDefaultMode.toString();
    QString vocab;

    if (m_settingsManager) {
        const auto& config = m_settingsManager->geminiSttConfig();
        model = config.model;
        modeVal = config.mode;
        vocab = config.customVocabulary;
    } else {
        const QSettings settings;
        model = settings.value(SettingsKeys::kGeminiModel.toString(), kDefaultModel.toString()).toString();
        if (model.trimmed().isEmpty()) {
            model = kDefaultModel.toString();
        }
        modeVal = settings.value(SettingsKeys::kGeminiMode.toString(), kDefaultMode.toString()).toString();
        if (modeVal != u"smart"_s && modeVal != u"verbatim"_s) {
            modeVal = kDefaultMode.toString();
        }
        vocab = settings.value(SettingsKeys::kGeminiCustomVocabulary.toString(), QString()).toString();
    }

    if (m_selectedModel != model) {
        m_selectedModel = model;
        emit selectedModelChanged();
    }

    if (m_mode != modeVal) {
        m_mode = modeVal;
        emit modeChanged();
    }

    if (m_customVocabulary != vocab) {
        m_customVocabulary = vocab;
        emit customVocabularyChanged();
    }

    if (m_keyStore) {
        m_keyStore->loadApiKey();
    }
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
        if (m_settingsManager) {
            m_settingsManager->setGeminiModel(m_selectedModel);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kGeminiModel.toString(), m_selectedModel);
            settings.sync();
        }
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
        if (m_settingsManager) {
            m_settingsManager->setGeminiMode(m_mode);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kGeminiMode.toString(), m_mode);
            settings.sync();
        }
        emit modeChanged();
    }
}

QString GeminiSttClient::customVocabulary() const {
    return m_customVocabulary;
}

void GeminiSttClient::setCustomVocabulary(const QString& vocab) {
    if (m_customVocabulary != vocab) {
        m_customVocabulary = vocab;
        if (m_settingsManager) {
            m_settingsManager->setGeminiCustomVocabulary(m_customVocabulary);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kGeminiCustomVocabulary.toString(), m_customVocabulary);
            settings.sync();
        }
        emit customVocabularyChanged();
    }
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

    const QPointer<GeminiSttClient> self = this;
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

bool GeminiSttClient::parseTranscript(const CloudApiResponse& res, QString& outText) {
    if (res.json.contains(u"output_text"_s)) {
        outText = res.json.value(u"output_text"_s).toString().trimmed();
    }

    if (outText.isEmpty() && res.json.contains(u"steps"_s)) {
        const QJsonArray steps = res.json.value(u"steps"_s).toArray();
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
        outText = extractedParts.join(u"\n"_s).trimmed();
    }

    return !outText.isEmpty();
}
