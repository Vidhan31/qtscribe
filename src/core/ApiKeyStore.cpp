#include "ApiKeyStore.h"

#include "LoggingCategories.h"

#include <QSettings>
#include <qt6keychain/keychain.h>

using namespace Qt::StringLiterals;

ApiKeyStore::ApiKeyStore(QObject* parent)
    : QObject(parent) { }

QString ApiKeyStore::apiKey() const {
    return m_apiKey;
}

void ApiKeyStore::setApiKey(const QString& key) {
    const QString trimmed = key.trimmed();
    if (m_apiKey != trimmed) {
        const bool wasSet = apiKeySet();
        m_apiKey = trimmed;
        m_apiKeyLoaded = true;
        emit apiKeyChanged();

        if (wasSet != apiKeySet()) {
            emit apiKeySetChanged();
        }

        if (!m_apiKey.isEmpty()) {
            saveApiKeyToKeychain(m_apiKey);
        } else {
            deleteApiKeyFromKeychain();
        }
    }
}

bool ApiKeyStore::apiKeySet() const {
    return !m_apiKey.isEmpty();
}

bool ApiKeyStore::isLoading() const {
    return m_isLoadingApiKey;
}

void ApiKeyStore::setIsLoading(bool loading) {
    if (m_isLoadingApiKey != loading) {
        m_isLoadingApiKey = loading;
        emit isLoadingChanged();
    }
}

void ApiKeyStore::loadApiKey() {
    if (m_isLoadingApiKey) {
        return;
    }
    loadApiKeyFromKeychain();
}

void ApiKeyStore::ensureApiKeyLoaded() {
    if (m_apiKeyLoaded || m_isLoadingApiKey) {
        return;
    }
    loadApiKey();
}

void ApiKeyStore::setStorageKeys(const QString& keychainService, const QString& keychainKey,
                                 const QString& settingsKey) {
    m_keychainService = keychainService;
    m_keychainKey = keychainKey;
    if (!settingsKey.isEmpty()) {
        m_settingsKey = settingsKey;
    }
}

QString ApiKeyStore::keychainService() const {
    return m_keychainService;
}

QString ApiKeyStore::keychainKey() const {
    return m_keychainKey;
}

QString ApiKeyStore::settingsKey() const {
    return m_settingsKey;
}

void ApiKeyStore::loadApiKeyFromKeychain() {
    setIsLoading(true);
    auto* readJob = new QKeychain::ReadPasswordJob(m_keychainService, this);
    readJob->setKey(m_keychainKey);
    readJob->setAutoDelete(true);

    connect(readJob, &QKeychain::ReadPasswordJob::finished, this, [this](QKeychain::Job* job) {
        setIsLoading(false);
        m_apiKeyLoaded = true;
        auto* rJob = static_cast<QKeychain::ReadPasswordJob*>(job);
        if (!rJob->error()) {
            const QString key = rJob->textData().trimmed();
            if (!key.isEmpty()) {
                qCDebug(lcNetwork) << "API key loaded successfully from system keychain (key length:" << key.size()
                                   << ")";
                m_apiKey = key;
                emit apiKeyChanged();
                emit apiKeySetChanged();
                return;
            }
        } else if (rJob->error() == QKeychain::EntryNotFound) {
            qCDebug(lcNetwork) << "No API key found in system keychain, checking QSettings fallback";
        } else {
            qWarning("ApiKeyStore: Failed to read API key from keychain (%s), checking QSettings fallback",
                     qPrintable(rJob->errorString()));
        }

        loadApiKeyFromSettingsFallback();
    });

    readJob->start();
}

void ApiKeyStore::loadApiKeyFromSettingsFallback() {
    QSettings settings;
    QString key = settings.value(m_settingsKey).toString().trimmed();
    if (key.isEmpty() && m_settingsKey == kSettingsApiKey.toString()) {
        key = settings.value(kLegacySettingsApiKey.toString()).toString().trimmed();
        if (!key.isEmpty()) {
            qCDebug(lcNetwork) << "Migrating API key from legacy QSettings key to" << m_settingsKey;
            settings.setValue(m_settingsKey, key);
            settings.remove(kLegacySettingsApiKey.toString());
        }
    }
    if (!key.isEmpty()) {
        qCDebug(lcNetwork) << "API key loaded from QSettings fallback (key length:" << key.size() << ")";
        m_apiKey = key;
        emit apiKeyChanged();
        emit apiKeySetChanged();
    } else {
        qCDebug(lcNetwork) << "No API key found in QSettings fallback";
    }
}

void ApiKeyStore::saveApiKeyToKeychain(const QString& key) {
    auto* writeJob = new QKeychain::WritePasswordJob(m_keychainService, this);
    writeJob->setKey(m_keychainKey);
    writeJob->setTextData(key);
    writeJob->setAutoDelete(true);

    connect(writeJob, &QKeychain::WritePasswordJob::finished, this, [this, key](QKeychain::Job* job) {
        if (job->error()) {
            qWarning("ApiKeyStore: Failed to save API key to keychain (%s), saving to QSettings fallback",
                     qPrintable(job->errorString()));
            QSettings settings;
            settings.setValue(m_settingsKey, key);
        } else {
            qCDebug(lcNetwork) << "API key persisted into system keychain";
            QSettings settings;
            settings.remove(m_settingsKey);
            if (m_settingsKey == kSettingsApiKey.toString()) {
                settings.remove(kLegacySettingsApiKey.toString());
            }
        }
    });

    writeJob->start();
}

void ApiKeyStore::deleteApiKeyFromKeychain() {
    QSettings settings;
    settings.remove(m_settingsKey);
    if (m_settingsKey == kSettingsApiKey.toString()) {
        settings.remove(kLegacySettingsApiKey.toString());
    }

    auto* deleteJob = new QKeychain::DeletePasswordJob(m_keychainService, this);
    deleteJob->setKey(m_keychainKey);
    deleteJob->setAutoDelete(true);

    connect(deleteJob, &QKeychain::DeletePasswordJob::finished, this, [](QKeychain::Job* job) {
        if (job->error() && job->error() != QKeychain::EntryNotFound) {
            qWarning("ApiKeyStore: Failed to delete API key from keychain: %s", qPrintable(job->errorString()));
        } else {
            qCDebug(lcNetwork) << "API key deleted from system keychain";
        }
    });

    deleteJob->start();
}
