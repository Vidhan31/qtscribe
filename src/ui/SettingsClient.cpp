#include "SettingsClient.h"

#include <QCoreApplication>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDebug>
#include <QSettings>

using namespace Qt::StringLiterals;

static const auto kServiceName = u"io.github.qtscribe.SpeechService"_s;
static const auto kObjectPath = u"/io/github/qtscribe/SpeechService"_s;
static const auto kInterface = u"io.github.qtscribe.SettingsService"_s;

SettingsClient::SettingsClient(QObject* parent)
    : QObject(parent) {
    s_instance = this;

    const bool disableRemoteAttach = !qEnvironmentVariableIsEmpty("QTSCRIBE_DISABLE_REMOTE_ATTACH") &&
                                     qEnvironmentVariable("QTSCRIBE_DISABLE_REMOTE_ATTACH") != "0";
    if (disableRemoteAttach) {
        return;
    }

    if (QDBusConnection::sessionBus().isConnected()) {
        m_watcher = new QDBusServiceWatcher(
            kServiceName, QDBusConnection::sessionBus(),
            QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration, this);
        connect(m_watcher, &QDBusServiceWatcher::serviceRegistered, this, &SettingsClient::onServiceRegistered);
        connect(m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, &SettingsClient::onServiceUnregistered);

        auto* iface = QDBusConnection::sessionBus().interface();
        if (iface && iface->isServiceRegistered(kServiceName)) {
            onServiceRegistered(kServiceName);
        }
    }
}

SettingsClient::~SettingsClient() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

SettingsClient* SettingsClient::instance() {
    if (!s_instance) {
        s_instance = new SettingsClient(QCoreApplication::instance());
    }
    return s_instance;
}

SettingsClient* SettingsClient::create(QQmlEngine* /*qmlEngine*/, QJSEngine* /*jsEngine*/) {
    return instance();
}

bool SettingsClient::isConnected() const noexcept {
    return m_connected;
}

void SettingsClient::onServiceRegistered(const QString& /*serviceName*/) {
    setupInterface();
}

void SettingsClient::onServiceUnregistered(const QString& /*serviceName*/) {
    m_connected = false;
    emit connectedChanged();
    if (m_interface) {
        delete m_interface;
        m_interface = nullptr;
    }
}

void SettingsClient::setupInterface() {
    delete m_interface;

    m_interface = new QDBusInterface(kServiceName, kObjectPath, kInterface, QDBusConnection::sessionBus(), this);
    if (m_interface->isValid()) {
        m_connected = true;
        emit connectedChanged();

        QDBusConnection::sessionBus().connect(kServiceName, kObjectPath, kInterface, u"settingChanged"_s, this,
                                              SLOT(onRemoteSettingChanged(QString, QDBusVariant)));
    } else {
        m_connected = false;
        emit connectedChanged();
    }
}

void SettingsClient::onRemoteSettingChanged(const QString& key, const QDBusVariant& value) {
    emit settingChanged(key, value.variant());
}

void SettingsClient::setSetting(const QString& key, const QVariant& value) {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"setSetting"_s, key, QVariant::fromValue(QDBusVariant(value)));
    } else {
        QSettings settings;
        settings.setValue(key, value);
        settings.sync();
        emit settingChanged(key, value);
    }
}

QVariant SettingsClient::getSetting(const QString& key, const QVariant& defaultValue) const {
    if (m_interface && m_interface->isValid()) {
        const QDBusReply<QDBusVariant> reply = m_interface->call(u"getSetting"_s, key);
        if (reply.isValid()) {
            const QVariant val = reply.value().variant();
            if (val.isValid()) {
                return val;
            }
        }
    }
    const QSettings settings;
    return settings.value(key, defaultValue);
}

void SettingsClient::reloadSettings() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"reloadSettings"_s);
    }
}
