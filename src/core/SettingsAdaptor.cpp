#include "SettingsAdaptor.h"

#include "SettingsService.h"

SettingsAdaptor::SettingsAdaptor(SettingsService* service, QObject* parent)
    : QDBusAbstractAdaptor(parent)
    , m_service(service) {
    setAutoRelaySignals(true);
    if (m_service) {
        connect(m_service, &SettingsService::settingChanged, this, &SettingsAdaptor::settingChanged);
    }
}

void SettingsAdaptor::setSetting(const QString& key, const QDBusVariant& value) {
    if (m_service) {
        m_service->setSetting(key, value);
    }
}

void SettingsAdaptor::setStringSetting(const QString& key, const QString& value) {
    if (m_service) {
        m_service->setStringSetting(key, value);
    }
}

void SettingsAdaptor::setBoolSetting(const QString& key, bool value) {
    if (m_service) {
        m_service->setBoolSetting(key, value);
    }
}

void SettingsAdaptor::setIntSetting(const QString& key, int value) {
    if (m_service) {
        m_service->setIntSetting(key, value);
    }
}

void SettingsAdaptor::setDoubleSetting(const QString& key, double value) {
    if (m_service) {
        m_service->setDoubleSetting(key, value);
    }
}

QDBusVariant SettingsAdaptor::getSetting(const QString& key) const {
    if (m_service) {
        return m_service->getSetting(key);
    }
    return QDBusVariant(QVariant());
}

QVariantMap SettingsAdaptor::allSettings() const {
    if (m_service) {
        return m_service->allSettings();
    }
    return {};
}

void SettingsAdaptor::reloadSettings() {
    if (m_service) {
        m_service->reloadSettings();
    }
}
