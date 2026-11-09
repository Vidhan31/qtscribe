#include "DaemonDiagnosticModel.h"

#include "common/SettingsKeys.h"
#include "DaemonClient.h"
#include "SettingsClient.h"

#include <QDebug>
#include <QProcessEnvironment>
#include <QSettings>

using namespace Qt::StringLiterals;

#ifndef QTSCRIBE_IS_DEV_BUILD
#if defined(DEBUG) || !defined(NDEBUG)
#define QTSCRIBE_IS_DEV_BUILD true
#else
#define QTSCRIBE_IS_DEV_BUILD false
#endif
#endif

DaemonDiagnosticModel::DaemonDiagnosticModel(QObject* parent)
    : QObject(parent) {
    if (auto* sc = SettingsClient::instance()) {
        m_preventClipboardHistory = sc->getSetting(SettingsKeys::kClipboardPreventHistory.toString(), true).toBool();
        m_injectionDelay = sc->getSetting(SettingsKeys::kTypingPreInjectionDelayMs.toString(), 200).toInt();

        connect(sc, &SettingsClient::settingChanged, this, [this](const QString& key, const QVariant& value) {
            if (key == SettingsKeys::kClipboardPreventHistory) {
                const bool prevent = value.toBool();
                if (m_preventClipboardHistory != prevent) {
                    m_preventClipboardHistory = prevent;
                    emit preventClipboardHistoryChanged();
                }
            } else if (key == SettingsKeys::kTypingPreInjectionDelayMs) {
                const int delay = value.toInt();
                if (m_injectionDelay != delay) {
                    m_injectionDelay = delay;
                    emit injectionDelayChanged();
                }
            }
        });
    }

    if (auto* client = DaemonClient::instance()) {
        connect(client, &DaemonClient::textInjectionStatusChanged, this, [this]() {
            emit connectedChanged();
            emit hasFatalErrorChanged();
            emit fatalErrorMessageChanged();
            emit statusMessageChanged();
            emit lastErrorChanged();
        });
    }
}

bool DaemonDiagnosticModel::isConnected() {
    if (auto* client = DaemonClient::instance()) {
        return client->textInjectionConnected();
    }
    return false;
}

bool DaemonDiagnosticModel::hasFatalError() {
    if (auto* client = DaemonClient::instance()) {
        return client->textInjectionHasFatalError();
    }
    return false;
}

QString DaemonDiagnosticModel::fatalErrorMessage() {
    if (auto* client = DaemonClient::instance()) {
        return client->textInjectionFatalErrorMessage();
    }
    return {};
}

QString DaemonDiagnosticModel::lastError() {
    return {};
}

QString DaemonDiagnosticModel::statusMessage() {
    if (auto* client = DaemonClient::instance()) {
        return client->textInjectionStatus();
    }
    return {};
}

bool DaemonDiagnosticModel::preventClipboardHistory() const {
    return m_preventClipboardHistory;
}

void DaemonDiagnosticModel::setPreventClipboardHistory(bool prevent) {
    if (m_preventClipboardHistory != prevent) {
        m_preventClipboardHistory = prevent;
        if (auto* sc = SettingsClient::instance()) {
            sc->setSetting(SettingsKeys::kClipboardPreventHistory.toString(), prevent);
        }
        emit preventClipboardHistoryChanged();
    }
}

int DaemonDiagnosticModel::injectionDelay() const {
    return m_injectionDelay;
}

void DaemonDiagnosticModel::setInjectionDelay(int delayMs) {
    if (m_injectionDelay != delayMs) {
        m_injectionDelay = delayMs;
        if (auto* sc = SettingsClient::instance()) {
            sc->setSetting(SettingsKeys::kTypingPreInjectionDelayMs.toString(), delayMs);
        }
        emit injectionDelayChanged();
    }
}

bool DaemonDiagnosticModel::isKde() {
    const QString desktop =
        QProcessEnvironment::systemEnvironment().value(u"XDG_CURRENT_DESKTOP"_s, QString()).toLower();
    return desktop.contains(u"kde"_s);
}

bool DaemonDiagnosticModel::isDevBuild() {
    return QTSCRIBE_IS_DEV_BUILD;
}

bool DaemonDiagnosticModel::clipboardWarningAcknowledged() const {
    return m_clipboardWarningAcknowledged;
}

void DaemonDiagnosticModel::setClipboardWarningAcknowledged(bool acknowledged) {
    if (m_clipboardWarningAcknowledged != acknowledged) {
        m_clipboardWarningAcknowledged = acknowledged;
        emit clipboardWarningAcknowledgedChanged();
        emit clipboardWarningRequiredChanged();
    }
}

bool DaemonDiagnosticModel::clipboardWarningRequired() const {
    return (isDevBuild() || !isKde()) && !m_clipboardWarningAcknowledged;
}

bool DaemonDiagnosticModel::clipboardBannerDismissed() const {
    return m_clipboardBannerDismissed;
}

void DaemonDiagnosticModel::setClipboardBannerDismissed(bool dismissed) {
    if (m_clipboardBannerDismissed != dismissed) {
        m_clipboardBannerDismissed = dismissed;
        emit clipboardBannerDismissedChanged();
    }
}

void DaemonDiagnosticModel::connectToServer() {
    if (auto* client = DaemonClient::instance()) {
        client->restartTextInjection();
    }
}

void DaemonDiagnosticModel::disconnectFromServer() {
    if (auto* client = DaemonClient::instance()) {
        client->stopTextInjection();
    }
}

void DaemonDiagnosticModel::stopDaemon() {
    if (auto* client = DaemonClient::instance()) {
        client->stopTextInjection();
    }
}

void DaemonDiagnosticModel::restartService() {
    if (auto* client = DaemonClient::instance()) {
        client->restartTextInjection();
    }
}

bool DaemonDiagnosticModel::testTextInjection(const QString& /*text*/) {
    if (auto* client = DaemonClient::instance()) {
        client->testTextInjection();
        return true;
    }
    return false;
}

bool DaemonDiagnosticModel::testTyping(const QString& text) {
    return testTextInjection(text);
}

void DaemonDiagnosticModel::reloadDaemonSettings() {
    if (auto* sc = SettingsClient::instance()) {
        sc->reloadSettings();
    }
}
