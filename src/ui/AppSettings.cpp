#include "AppSettings.h"

#include "common/PresetProvider.h"
#include "common/SettingsKeys.h"
#include "DaemonClient.h"
#include "SettingsClient.h"

#include <QSettings>
#include <QVariant>
#include <qt6keychain/keychain.h>

using namespace Qt::StringLiterals;

namespace {
constexpr auto kKeychainService = "QtScribe";

constexpr auto kGroqKeychainKey = "groq_api_key";
const auto kGroqSettingsKey = SettingsKeys::kGroqSettingsKey.toString();
const auto kGroqLegacySettingsKey = SettingsKeys::kGroqLegacySettingsKey.toString();

constexpr auto kGeminiKeychainKey = "gemini_api_key";
const auto kGeminiSettingsKey = SettingsKeys::kGeminiSettingsKey.toString();

constexpr auto kMetaMusaKeychainKey = "meta_musa_api_key";
const auto kMetaMusaSettingsKey = SettingsKeys::kMetaMusaSettingsKey.toString();

const auto kDefaultGroqModel = u"whisper-large-v3-turbo"_s;
const auto kDefaultGeminiModel = u"gemini-3.5-transcribe"_s;
const auto kDefaultGeminiMode = u"smart"_s;
const auto kDefaultMetaMusaModel = u"muse-voice-transcribe-1.0"_s;
const auto kDefaultLlmModel = u"openai/gpt-oss-20b"_s;
const auto kDefaultLlmPreset = u"grammar"_s;
} // namespace

AppSettings::AppSettings(QObject* parent)
    : QObject(parent) {
    loadPersistedValues();
    loadApiKey(ApiKeyKind::Groq);
    loadApiKey(ApiKeyKind::Gemini);
    loadApiKey(ApiKeyKind::MetaMusa);

    if (auto* sc = SettingsClient::instance()) {
        connect(sc, &SettingsClient::settingChanged, this, [this](const QString& key, const QVariant& value) {
            if (key == SettingsKeys::kGroqModel) {
                const QString val = value.toString();
                if (m_groqSttModel != val) {
                    m_groqSttModel = val;
                    emit groqSttModelChanged();
                }
            } else if (key == SettingsKeys::kGroqLanguage) {
                const QString val = value.toString();
                if (m_groqLanguage != val) {
                    m_groqLanguage = val;
                    emit groqLanguageChanged();
                }
            } else if (key == SettingsKeys::kGroqCustomPrompt) {
                const QString val = value.toString();
                if (m_groqCustomPrompt != val) {
                    m_groqCustomPrompt = val;
                    emit groqCustomPromptChanged();
                }
            } else if (key == SettingsKeys::kGeminiModel) {
                const QString val = value.toString();
                if (m_geminiModel != val) {
                    m_geminiModel = val;
                    emit geminiModelChanged();
                }
            } else if (key == SettingsKeys::kGeminiMode) {
                const QString val = value.toString();
                if (m_geminiMode != val) {
                    m_geminiMode = val;
                    emit geminiModeChanged();
                }
            } else if (key == SettingsKeys::kGeminiCustomVocabulary) {
                const QString val = value.toString();
                if (m_geminiCustomVocabulary != val) {
                    m_geminiCustomVocabulary = val;
                    emit geminiCustomVocabularyChanged();
                }
            } else if (key == SettingsKeys::kMetaMusaModel) {
                const QString val = value.toString();
                if (m_metaMusaModel != val) {
                    m_metaMusaModel = val;
                    emit metaMusaModelChanged();
                }
            } else if (key == SettingsKeys::kMetaMusaKeywords) {
                const QString val = value.toString();
                if (m_metaMusaKeywords != val) {
                    m_metaMusaKeywords = val;
                    emit metaMusaKeywordsChanged();
                }
            } else if (key == SettingsKeys::kGroqLlmEnabled) {
                const bool val = value.toBool();
                if (m_llmEnabled != val) {
                    m_llmEnabled = val;
                    emit llmEnabledChanged();
                }
            } else if (key == SettingsKeys::kGroqLlmModel) {
                const QString val = value.toString();
                if (m_llmModel != val) {
                    m_llmModel = val;
                    emit llmModelChanged();
                }
            } else if (key == SettingsKeys::kGroqLlmPreset) {
                const QString val = value.toString();
                if (m_llmPreset != val) {
                    m_llmPreset = val;
                    emit llmPresetChanged();
                }
            } else if (key == SettingsKeys::kGroqLlmCustomPrompt) {
                const QString val = value.toString();
                if (m_llmCustomPrompt != val) {
                    m_llmCustomPrompt = val;
                    emit llmCustomPromptChanged();
                }
            } else if (key == SettingsKeys::kGroqLlmTemperature) {
                const double val = value.toDouble();
                if (!qFuzzyCompare(m_llmTemperature, val)) {
                    m_llmTemperature = val;
                    emit llmTemperatureChanged();
                }
            } else if (key == SettingsKeys::kAudioSoundEnabled) {
                const bool val = value.toBool();
                if (m_soundEnabled != val) {
                    m_soundEnabled = val;
                    emit soundEnabledChanged();
                }
            } else if (key == SettingsKeys::kNotificationsEnabled) {
                const bool val = value.toBool();
                if (m_notificationsEnabled != val) {
                    m_notificationsEnabled = val;
                    emit notificationsEnabledChanged();
                }
            }
        });
    }
}

void AppSettings::loadPersistedValues() {
    auto* sc = SettingsClient::instance();
    const auto getVal = [sc](QStringView key, const QVariant& def) {
        return sc ? sc->getSetting(key.toString(), def) : def;
    };

    m_groqSttModel = getVal(SettingsKeys::kGroqModel, kDefaultGroqModel).toString();
    if (m_groqSttModel != u"whisper-large-v3-turbo"_s && m_groqSttModel != u"whisper-large-v3"_s) {
        m_groqSttModel = kDefaultGroqModel;
    }
    m_groqLanguage = getVal(SettingsKeys::kGroqLanguage, QString()).toString();
    m_groqCustomPrompt = getVal(SettingsKeys::kGroqCustomPrompt, QString()).toString();

    m_geminiModel = getVal(SettingsKeys::kGeminiModel, kDefaultGeminiModel).toString();
    if (m_geminiModel.trimmed().isEmpty()) {
        m_geminiModel = kDefaultGeminiModel;
    }
    m_geminiMode = getVal(SettingsKeys::kGeminiMode, kDefaultGeminiMode).toString();
    if (m_geminiMode != u"smart"_s && m_geminiMode != u"verbatim"_s) {
        m_geminiMode = kDefaultGeminiMode;
    }
    m_geminiCustomVocabulary = getVal(SettingsKeys::kGeminiCustomVocabulary, QString()).toString();

    m_metaMusaModel = getVal(SettingsKeys::kMetaMusaModel, kDefaultMetaMusaModel).toString();
    if (m_metaMusaModel.trimmed().isEmpty()) {
        m_metaMusaModel = kDefaultMetaMusaModel;
    }
    m_metaMusaKeywords = getVal(SettingsKeys::kMetaMusaKeywords, QString()).toString();

    m_llmEnabled = getVal(SettingsKeys::kGroqLlmEnabled, false).toBool();
    m_llmModel = getVal(SettingsKeys::kGroqLlmModel, kDefaultLlmModel).toString();
    if (m_llmModel.isEmpty() || m_llmModel == u"llama-3.1-8b-instant"_s) {
        m_llmModel = kDefaultLlmModel;
    }
    m_llmPreset = getVal(SettingsKeys::kGroqLlmPreset, kDefaultLlmPreset).toString();
    if (m_llmPreset.isEmpty()) {
        m_llmPreset = kDefaultLlmPreset;
    }
    m_llmCustomPrompt = getVal(SettingsKeys::kGroqLlmCustomPrompt, QString()).toString();
    m_llmTemperature = getVal(SettingsKeys::kGroqLlmTemperature, 0.1).toDouble();

    m_soundEnabled = getVal(SettingsKeys::kAudioSoundEnabled, true).toBool();
    m_notificationsEnabled = getVal(SettingsKeys::kNotificationsEnabled, true).toBool();
}

QString AppSettings::groqApiKey() const {
    return m_groqApiKey;
}

void AppSettings::setGroqApiKey(const QString& key) {
    const QString trimmed = key.trimmed();
    if (m_groqApiKey == trimmed) {
        return;
    }
    m_groqApiKey = trimmed;
    emit groqApiKeyChanged();
    persistApiKey(ApiKeyKind::Groq, trimmed);
}

QString AppSettings::groqSttModel() const {
    return m_groqSttModel;
}

void AppSettings::setGroqSttModel(const QString& model) {
    if (m_groqSttModel == model) {
        return;
    }
    m_groqSttModel = model;
    persistValue(u"Groq/Model"_s, model);
    emit groqSttModelChanged();
}

QString AppSettings::groqLanguage() const {
    return m_groqLanguage;
}

void AppSettings::setGroqLanguage(const QString& language) {
    if (m_groqLanguage == language) {
        return;
    }
    m_groqLanguage = language;
    persistValue(u"Groq/Language"_s, language);
    emit groqLanguageChanged();
}

QString AppSettings::groqCustomPrompt() const {
    return m_groqCustomPrompt;
}

void AppSettings::setGroqCustomPrompt(const QString& prompt) {
    if (m_groqCustomPrompt == prompt) {
        return;
    }
    m_groqCustomPrompt = prompt;
    persistValue(u"Groq/CustomPrompt"_s, prompt);
    emit groqCustomPromptChanged();
}

QString AppSettings::geminiApiKey() const {
    return m_geminiApiKey;
}

void AppSettings::setGeminiApiKey(const QString& key) {
    const QString trimmed = key.trimmed();
    if (m_geminiApiKey == trimmed) {
        return;
    }
    m_geminiApiKey = trimmed;
    emit geminiApiKeyChanged();
    persistApiKey(ApiKeyKind::Gemini, trimmed);
}

QString AppSettings::geminiModel() const {
    return m_geminiModel;
}

void AppSettings::setGeminiModel(const QString& model) {
    if (m_geminiModel == model) {
        return;
    }
    m_geminiModel = model;
    persistValue(u"Gemini/Model"_s, model);
    emit geminiModelChanged();
}

QString AppSettings::geminiMode() const {
    return m_geminiMode;
}

void AppSettings::setGeminiMode(const QString& mode) {
    if (m_geminiMode == mode) {
        return;
    }
    m_geminiMode = mode;
    persistValue(u"Gemini/Mode"_s, mode);
    emit geminiModeChanged();
}

QString AppSettings::geminiCustomVocabulary() const {
    return m_geminiCustomVocabulary;
}

void AppSettings::setGeminiCustomVocabulary(const QString& vocabulary) {
    if (m_geminiCustomVocabulary == vocabulary) {
        return;
    }
    m_geminiCustomVocabulary = vocabulary;
    persistValue(u"Gemini/CustomVocabulary"_s, vocabulary);
    emit geminiCustomVocabularyChanged();
}

QString AppSettings::metaMusaApiKey() const {
    return m_metaMusaApiKey;
}

void AppSettings::setMetaMusaApiKey(const QString& key) {
    const QString trimmed = key.trimmed();
    if (m_metaMusaApiKey == trimmed) {
        return;
    }
    m_metaMusaApiKey = trimmed;
    emit metaMusaApiKeyChanged();
    persistApiKey(ApiKeyKind::MetaMusa, trimmed);
}

QString AppSettings::metaMusaModel() const {
    return m_metaMusaModel;
}

void AppSettings::setMetaMusaModel(const QString& model) {
    if (m_metaMusaModel == model) {
        return;
    }
    m_metaMusaModel = model;
    persistValue(u"MetaMusa/Model"_s, model);
    emit metaMusaModelChanged();
}

QString AppSettings::metaMusaKeywords() const {
    return m_metaMusaKeywords;
}

void AppSettings::setMetaMusaKeywords(const QString& keywords) {
    if (m_metaMusaKeywords == keywords) {
        return;
    }
    m_metaMusaKeywords = keywords;
    persistValue(u"MetaMusa/Keywords"_s, keywords);
    emit metaMusaKeywordsChanged();
}

bool AppSettings::llmEnabled() const {
    return m_llmEnabled;
}

void AppSettings::setLlmEnabled(bool enabled) {
    if (m_llmEnabled == enabled) {
        return;
    }
    m_llmEnabled = enabled;
    persistValue(u"Groq/LlmEnabled"_s, enabled);
    emit llmEnabledChanged();
}

QString AppSettings::llmModel() const {
    return m_llmModel;
}

void AppSettings::setLlmModel(const QString& model) {
    if (m_llmModel == model) {
        return;
    }
    m_llmModel = model;
    persistValue(u"Groq/LlmModel"_s, model);
    emit llmModelChanged();
}

QString AppSettings::llmPreset() const {
    return m_llmPreset;
}

void AppSettings::setLlmPreset(const QString& preset) {
    if (m_llmPreset == preset) {
        return;
    }
    m_llmPreset = preset;
    persistValue(u"Groq/LlmPreset"_s, preset);
    emit llmPresetChanged();
}

QString AppSettings::llmCustomPrompt() const {
    return m_llmCustomPrompt;
}

void AppSettings::setLlmCustomPrompt(const QString& prompt) {
    if (m_llmCustomPrompt == prompt) {
        return;
    }
    m_llmCustomPrompt = prompt;
    persistValue(u"Groq/LlmCustomPrompt"_s, prompt);
    emit llmCustomPromptChanged();
}

double AppSettings::llmTemperature() const {
    return m_llmTemperature;
}

void AppSettings::setLlmTemperature(double temperature) {
    if (qFuzzyCompare(m_llmTemperature, temperature)) {
        return;
    }
    m_llmTemperature = temperature;
    persistValue(u"Groq/LlmTemperature"_s, temperature);
    emit llmTemperatureChanged();
}

QString AppSettings::formattedLlmTemperature() const {
    return QString::number(m_llmTemperature, 'f', 2);
}

bool AppSettings::soundEnabled() const {
    return m_soundEnabled;
}

void AppSettings::setSoundEnabled(bool enabled) {
    if (m_soundEnabled == enabled) {
        return;
    }
    m_soundEnabled = enabled;
    persistValue(u"Audio/SoundEnabled"_s, enabled);
    emit soundEnabledChanged();
}

bool AppSettings::notificationsEnabled() const {
    return m_notificationsEnabled;
}

void AppSettings::setNotificationsEnabled(bool enabled) {
    if (m_notificationsEnabled == enabled) {
        return;
    }
    m_notificationsEnabled = enabled;
    persistValue(u"notifications/enabled"_s, enabled);
    emit notificationsEnabledChanged();
}

QString AppSettings::systemPromptForPreset(const QString& preset) const {
    return PresetProvider::systemPromptForPreset(preset, m_llmCustomPrompt);
}

QString& AppSettings::apiKeyRef(ApiKeyKind kind) {
    if (kind == ApiKeyKind::Groq) {
        return m_groqApiKey;
    }
    if (kind == ApiKeyKind::Gemini) {
        return m_geminiApiKey;
    }
    return m_metaMusaApiKey;
}

void AppSettings::emitApiKeyChanged(ApiKeyKind kind) {
    if (kind == ApiKeyKind::Groq) {
        emit groqApiKeyChanged();
    } else if (kind == ApiKeyKind::Gemini) {
        emit geminiApiKeyChanged();
    } else {
        emit metaMusaApiKeyChanged();
    }
}

void AppSettings::loadApiKey(ApiKeyKind kind) {
    const QString keychainKey = (kind == ApiKeyKind::Groq)     ? QString::fromLatin1(kGroqKeychainKey)
                                : (kind == ApiKeyKind::Gemini) ? QString::fromLatin1(kGeminiKeychainKey)
                                                               : QString::fromLatin1(kMetaMusaKeychainKey);
    const QString settingsKey = (kind == ApiKeyKind::Groq)     ? kGroqSettingsKey
                                : (kind == ApiKeyKind::Gemini) ? kGeminiSettingsKey
                                                               : kMetaMusaSettingsKey;

    auto* readJob = new QKeychain::ReadPasswordJob(QString::fromLatin1(kKeychainService));
    readJob->setKey(keychainKey);
    readJob->setAutoDelete(true);

    connect(readJob, &QKeychain::ReadPasswordJob::finished, this, [this, kind, settingsKey](QKeychain::Job* job) {
        auto* readJob = static_cast<QKeychain::ReadPasswordJob*>(job);
        if (!readJob->error()) {
            const QString key = readJob->textData().trimmed();
            if (!key.isEmpty()) {
                applyLoadedApiKey(kind, key);
                return;
            }
        }

        QSettings settings;
        QString fallback = settings.value(settingsKey).toString().trimmed();
        if (fallback.isEmpty() && kind == ApiKeyKind::Groq) {
            fallback = settings.value(kGroqLegacySettingsKey).toString().trimmed();
            if (!fallback.isEmpty()) {
                settings.setValue(settingsKey, fallback);
            }
        }
        if (!fallback.isEmpty()) {
            applyLoadedApiKey(kind, fallback);
        }
    });

    readJob->start();
}

void AppSettings::applyLoadedApiKey(ApiKeyKind kind, const QString& key) {
    auto& current = apiKeyRef(kind);
    if (!current.isEmpty()) {
        return;
    }
    current = key;
    emitApiKeyChanged(kind);
}

void AppSettings::persistApiKey(ApiKeyKind kind, const QString& value) {
    const QString keychainKey = (kind == ApiKeyKind::Groq)     ? QString::fromLatin1(kGroqKeychainKey)
                                : (kind == ApiKeyKind::Gemini) ? QString::fromLatin1(kGeminiKeychainKey)
                                                               : QString::fromLatin1(kMetaMusaKeychainKey);
    const QString settingsKey = (kind == ApiKeyKind::Groq)     ? kGroqSettingsKey
                                : (kind == ApiKeyKind::Gemini) ? kGeminiSettingsKey
                                                               : kMetaMusaSettingsKey;
    const QString legacyKey = (kind == ApiKeyKind::Groq) ? kGroqLegacySettingsKey : QString();

    QSettings settings;
    if (value.isEmpty()) {
        settings.remove(settingsKey);
        if (!legacyKey.isEmpty()) {
            settings.remove(legacyKey);
        }

        auto* deleteJob = new QKeychain::DeletePasswordJob(QString::fromLatin1(kKeychainService));
        deleteJob->setKey(keychainKey);
        deleteJob->setAutoDelete(true);
        connect(deleteJob, &QKeychain::DeletePasswordJob::finished, this, [](QKeychain::Job* job) {
            if (job->error() && job->error() != QKeychain::EntryNotFound) {
                qWarning() << "AppSettings: Failed to delete API key:" << job->errorString();
            }
            reloadDaemon();
        });
        deleteJob->start();
        return;
    }

    auto* writeJob = new QKeychain::WritePasswordJob(QString::fromLatin1(kKeychainService));
    writeJob->setKey(keychainKey);
    writeJob->setTextData(value);
    writeJob->setAutoDelete(true);

    connect(
        writeJob, &QKeychain::WritePasswordJob::finished, this, [value, settingsKey, legacyKey](QKeychain::Job* job) {
            QSettings settings;
            if (job->error()) {
                qWarning() << "AppSettings: Keychain write failed, using configuration fallback:" << job->errorString();
                settings.setValue(settingsKey, value);
            } else {
                settings.remove(settingsKey);
                if (!legacyKey.isEmpty()) {
                    settings.remove(legacyKey);
                }
            }
            reloadDaemon();
        });

    writeJob->start();
}

void AppSettings::persistValue(const QString& key, const QVariant& value) {
    if (auto* sc = SettingsClient::instance()) {
        sc->setSetting(key, value);
    }
}

void AppSettings::reloadDaemon() {
    if (auto* sc = SettingsClient::instance()) {
        sc->reloadSettings();
    }
}
