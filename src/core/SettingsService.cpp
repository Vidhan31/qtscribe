#include "SettingsService.h"

#include "ApiKeyStore.h"
#include "SettingsManager.h"

SettingsService::SettingsService(SettingsManager* manager, QObject* parent)
    : QObject(parent)
    , m_manager(manager) {
    if (m_manager) {
        connect(m_manager, &SettingsManager::settingChanged, this,
                [this](const QString& key, const QVariant& value) { emit settingChanged(key, QDBusVariant(value)); });
    }
}

void SettingsService::registerKeyStore(ApiKeyStore* store) {
    if (store && !m_keyStores.contains(store)) {
        m_keyStores.append(store);
    }
}

void SettingsService::setSetting(const QString& key, const QDBusVariant& value) {
    if (m_manager) {
        m_manager->setSetting(key, value.variant());
    }
}

void SettingsService::setStringSetting(const QString& key, const QString& value) {
    if (m_manager) {
        m_manager->setSetting(key, value);
    }
}

void SettingsService::setBoolSetting(const QString& key, bool value) {
    if (m_manager) {
        m_manager->setSetting(key, value);
    }
}

void SettingsService::setIntSetting(const QString& key, int value) {
    if (m_manager) {
        m_manager->setSetting(key, value);
    }
}

void SettingsService::setDoubleSetting(const QString& key, double value) {
    if (m_manager) {
        m_manager->setSetting(key, value);
    }
}

QDBusVariant SettingsService::getSetting(const QString& key) const {
    if (m_manager) {
        return QDBusVariant(m_manager->getSetting(key));
    }
    return QDBusVariant(QVariant());
}

QVariantMap SettingsService::allSettings() const {
    if (m_manager) {
        return m_manager->allSettings();
    }
    return {};
}

void SettingsService::reloadSettings() {
    if (m_manager) {
        m_manager->load();
    }
    for (auto* store : m_keyStores) {
        if (store) {
            store->loadApiKey();
        }
    }
    emit settingsReloadRequested();
}
