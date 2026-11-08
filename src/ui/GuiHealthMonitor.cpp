#include "GuiHealthMonitor.h"

#include <QNetworkInformation>

GuiHealthMonitor::GuiHealthMonitor(QObject* parent)
    : QObject(parent) {
    if (auto* client = DaemonClient::instance()) {
        connect(client, &DaemonClient::shortcutStatusChanged, this, &GuiHealthMonitor::systemHealthChanged);
        connect(client, &DaemonClient::directTypingStatusChanged, this, &GuiHealthMonitor::systemHealthChanged);
        connect(client, &DaemonClient::connectedChanged, this, &GuiHealthMonitor::systemHealthChanged);
    }

    if (QNetworkInformation::loadDefaultBackend()) {
        auto* netInfo = QNetworkInformation::instance();
        if (netInfo) {
            m_networkOnline = (netInfo->reachability() == QNetworkInformation::Reachability::Online);
            connect(netInfo, &QNetworkInformation::reachabilityChanged, this,
                    [this](QNetworkInformation::Reachability reachability) {
                        const bool online = (reachability == QNetworkInformation::Reachability::Online);
                        if (m_networkOnline != online) {
                            m_networkOnline = online;
                            emit networkOnlineChanged(online);
                        }
                    });
        }
    }
}

bool GuiHealthMonitor::systemShortcutHasIssue() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->systemShortcutHasIssue() : false;
}

bool GuiHealthMonitor::systemShortcutSupported() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->shortcutSupported() : false;
}

QString GuiHealthMonitor::systemShortcutStatus() const {
    auto* client = DaemonClient::instance();
    return client ? client->shortcutStatusMessage() : QString();
}

bool GuiHealthMonitor::directTypingHasIssue() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->directTypingHasIssue() : false;
}

bool GuiHealthMonitor::directTypingConnected() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->directTypingConnected() : false;
}

bool GuiHealthMonitor::directTypingFatalError() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->directTypingHasFatalError() : false;
}

QString GuiHealthMonitor::directTypingStatus() const {
    auto* client = DaemonClient::instance();
    return client ? client->directTypingStatus() : QString();
}

bool GuiHealthMonitor::pushToTalkSupported() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->pushToTalkSupported() : false;
}

bool GuiHealthMonitor::isNetworkOnline() const noexcept {
    return m_networkOnlineOverride.value_or(m_networkOnline);
}

bool GuiHealthMonitor::canRecord(bool notProcessing) const noexcept {
    auto* client = DaemonClient::instance();
    return notProcessing && client && client->canRecord();
}
