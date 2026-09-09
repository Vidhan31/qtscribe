#include "SystemHealthMonitor.h"

#include "AudioRecorder.h"
#include "DaemonConnector.h"
#include "LoggingCategories.h"

#include "AbstractSttClient.h"
#include "GlobalShortcutManager.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusVariant>
#include <QNetworkInformation>
#include <QNetworkInterface>

using namespace Qt::StringLiterals;

SystemHealthMonitor::SystemHealthMonitor(QObject* parent)
    : QObject(parent) {
    if (!QNetworkInformation::loadBackendByName(QStringView(u"networkmanager"_s))) {
        if (!QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability)) {
            QNetworkInformation::loadDefaultBackend();
        }
    }
    if (auto* info = QNetworkInformation::instance()) {
        connect(info, &QNetworkInformation::reachabilityChanged, this, &SystemHealthMonitor::onReachabilityChanged);
    }

    if (QDBusConnection::systemBus().isConnected()) {
        QDBusConnection::systemBus().connect(u"org.freedesktop.NetworkManager"_s, u"/org/freedesktop/NetworkManager"_s,
                                             u"org.freedesktop.DBus.Properties"_s, u"PropertiesChanged"_s, this,
                                             SLOT(onReachabilityChanged()));
    }

    m_networkOnline = computeNetworkOnline();
}

void SystemHealthMonitor::setShortcutManager(GlobalShortcutManager* mgr) {
    if (m_shortcutMgr == mgr) {
        return;
    }
    if (m_shortcutMgr) {
        disconnect(m_shortcutMgr, &GlobalShortcutManager::availableChanged, this,
                   &SystemHealthMonitor::notifyHealthChanged);
        disconnect(m_shortcutMgr, &GlobalShortcutManager::supportedChanged, this,
                   &SystemHealthMonitor::notifyHealthChanged);
        disconnect(m_shortcutMgr, &GlobalShortcutManager::statusMessageChanged, this,
                   &SystemHealthMonitor::notifyHealthChanged);
    }
    m_shortcutMgr = mgr;
    if (m_shortcutMgr) {
        connect(m_shortcutMgr, &GlobalShortcutManager::availableChanged, this,
                &SystemHealthMonitor::notifyHealthChanged);
        connect(m_shortcutMgr, &GlobalShortcutManager::supportedChanged, this,
                &SystemHealthMonitor::notifyHealthChanged);
        connect(m_shortcutMgr, &GlobalShortcutManager::statusMessageChanged, this,
                &SystemHealthMonitor::notifyHealthChanged);
    }
    notifyHealthChanged();
}

void SystemHealthMonitor::setDaemonConnector(DaemonConnector* connector) {
    if (m_connector == connector) {
        return;
    }
    if (m_connector) {
        disconnect(m_connector, &DaemonConnector::connectedChanged, this, &SystemHealthMonitor::notifyHealthChanged);
        disconnect(m_connector, &DaemonConnector::hasFatalErrorChanged, this,
                   &SystemHealthMonitor::notifyHealthChanged);
        disconnect(m_connector, &DaemonConnector::fatalErrorMessageChanged, this,
                   &SystemHealthMonitor::notifyHealthChanged);
    }
    m_connector = connector;
    if (m_connector) {
        connect(m_connector, &DaemonConnector::connectedChanged, this, &SystemHealthMonitor::notifyHealthChanged);
        connect(m_connector, &DaemonConnector::hasFatalErrorChanged, this, &SystemHealthMonitor::notifyHealthChanged);
        connect(m_connector, &DaemonConnector::fatalErrorMessageChanged, this,
                &SystemHealthMonitor::notifyHealthChanged);
    }
    notifyHealthChanged();
}

void SystemHealthMonitor::setAudioRecorder(AudioRecorder* recorder) {
    if (m_recorder == recorder) {
        return;
    }
    if (m_recorder) {
        disconnect(m_recorder, &AudioRecorder::hasAudioInputDeviceChanged, this,
                   &SystemHealthMonitor::notifyHealthChanged);
    }
    m_recorder = recorder;
    if (m_recorder) {
        connect(m_recorder, &AudioRecorder::hasAudioInputDeviceChanged, this,
                &SystemHealthMonitor::notifyHealthChanged);
    }
    notifyHealthChanged();
}

void SystemHealthMonitor::setActiveSttClient(AbstractSttClient* client) {
    if (m_activeSttClient == client) {
        return;
    }
    if (m_activeSttClient) {
        disconnect(m_activeSttClient, &AbstractSttClient::readyChanged, this,
                   &SystemHealthMonitor::notifyHealthChanged);
        disconnect(m_activeSttClient, &AbstractSttClient::busyChanged, this, &SystemHealthMonitor::notifyHealthChanged);
    }
    m_activeSttClient = client;
    if (m_activeSttClient) {
        connect(m_activeSttClient, &AbstractSttClient::readyChanged, this, &SystemHealthMonitor::notifyHealthChanged);
        connect(m_activeSttClient, &AbstractSttClient::busyChanged, this, &SystemHealthMonitor::notifyHealthChanged);
    }
    notifyHealthChanged();
}

bool SystemHealthMonitor::systemShortcutHasIssue() const {
    return m_shortcutMgr && !m_shortcutMgr->isAvailable();
}

bool SystemHealthMonitor::systemShortcutSupported() const {
    return m_shortcutMgr && m_shortcutMgr->isSupported();
}

QString SystemHealthMonitor::systemShortcutStatus() const {
    return m_shortcutMgr ? m_shortcutMgr->statusMessage() : QString();
}

bool SystemHealthMonitor::directTypingHasIssue() const {
    return m_connector && (!m_connector->isConnected() || m_connector->hasFatalError());
}

bool SystemHealthMonitor::directTypingConnected() const {
    return m_connector && m_connector->isConnected();
}

bool SystemHealthMonitor::directTypingFatalError() const {
    return m_connector && m_connector->hasFatalError();
}

QString SystemHealthMonitor::directTypingStatus() const {
    if (!m_connector) {
        return QString();
    }
    if (m_connector->hasFatalError()) {
        return m_connector->fatalErrorMessage().isEmpty() ? tr("Direct Typing Error")
                                                          : m_connector->fatalErrorMessage();
    }
    if (!m_connector->isConnected()) {
        return tr("Direct Typing Not Connected");
    }
    return QString();
}

bool SystemHealthMonitor::pushToTalkSupported() const {
    return m_shortcutMgr && m_shortcutMgr->isSupported();
}

bool SystemHealthMonitor::isNetworkOnline() const {
    return m_networkOnline;
}

void SystemHealthMonitor::setNetworkOnlineForTesting(std::optional<bool> online) {
    m_networkOnlineOverride = online;
    onReachabilityChanged();
}

bool SystemHealthMonitor::computeNetworkOnline() const {
    if (m_networkOnlineOverride.has_value()) {
        return m_networkOnlineOverride.value();
    }

    if (QDBusConnection::systemBus().isConnected()) {
        QDBusInterface nm(u"org.freedesktop.NetworkManager"_s, u"/org/freedesktop/NetworkManager"_s,
                          u"org.freedesktop.NetworkManager"_s, QDBusConnection::systemBus());
        if (nm.isValid()) {
            const bool nmNetworking = nm.property("NetworkingEnabled").toBool();
            const uint nmState = nm.property("State").toUInt();
            if (!nmNetworking || (nmState > 0 && nmState <= 20)) {
                return false;
            }
        }
    }

    auto* info = QNetworkInformation::instance();
    if (info) {
        const auto reachability = info->reachability();
        if (reachability == QNetworkInformation::Reachability::Disconnected ||
            reachability == QNetworkInformation::Reachability::Local) {
            return false;
        }
        if (reachability == QNetworkInformation::Reachability::Online ||
            reachability == QNetworkInformation::Reachability::Site) {
            return true;
        }
    }

    for (const auto& iface : QNetworkInterface::allInterfaces()) {
        if (!iface.flags().testFlag(QNetworkInterface::IsLoopBack) && iface.flags().testFlag(QNetworkInterface::IsUp) &&
            iface.flags().testFlag(QNetworkInterface::IsRunning)) {
            for (const auto& entry : iface.addressEntries()) {
                const auto ip = entry.ip();
                if (!ip.isLoopback() && !ip.isNull()) {
                    return true;
                }
            }
        }
    }

    return false;
}

void SystemHealthMonitor::onReachabilityChanged() {
    const bool current = computeNetworkOnline();
    if (m_networkOnline != current) {
        m_networkOnline = current;
        emit networkOnlineChanged(m_networkOnline);
        notifyHealthChanged();
    }
}

bool SystemHealthMonitor::canRecord(bool notProcessing) const {
    const bool micReady = m_recorder && m_recorder->hasAudioInputDevice();
    const bool sttReady = m_activeSttClient && m_activeSttClient->isReady();
    return micReady && notProcessing && sttReady;
}

void SystemHealthMonitor::notifyHealthChanged() {
    emit systemHealthChanged();
    emit canRecordChanged();
}
