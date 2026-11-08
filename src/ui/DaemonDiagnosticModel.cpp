#include "DaemonDiagnosticModel.h"

#include "AbstractTextInjector.h"
#include "DaemonConnector.h"

#include "DaemonClient.h"

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
    QSettings settings;
    m_preventClipboardHistory = settings.value(u"Clipboard/PreventHistory"_s, true).toBool();
    m_injectionDelay = settings.value(u"Typing/PreInjectionDelayMs"_s, 200).toInt();

    if (auto* client = DaemonClient::instance()) {
        connect(client, &DaemonClient::directTypingStatusChanged, this, [this]() {
            if (!m_connector) {
                emit connectedChanged();
                emit hasFatalErrorChanged();
                emit fatalErrorMessageChanged();
                emit statusMessageChanged();
            }
        });
    }
}

void DaemonDiagnosticModel::setDaemonConnector(DaemonConnector* connector) {
    if (m_connector == connector) {
        return;
    }

    if (m_connector) {
        disconnect(m_connector, &DaemonConnector::connectedChanged, this, &DaemonDiagnosticModel::connectedChanged);
        disconnect(m_connector, &DaemonConnector::hasFatalErrorChanged, this,
                   &DaemonDiagnosticModel::hasFatalErrorChanged);
        disconnect(m_connector, &DaemonConnector::fatalErrorMessageChanged, this,
                   &DaemonDiagnosticModel::fatalErrorMessageChanged);
        disconnect(m_connector, &DaemonConnector::lastErrorChanged, this, nullptr);
        disconnect(m_connector, &DaemonConnector::statusMessageChanged, this, nullptr);
    }

    m_connector = connector;

    if (m_connector) {
        connect(m_connector, &DaemonConnector::connectedChanged, this, &DaemonDiagnosticModel::connectedChanged);
        connect(m_connector, &DaemonConnector::hasFatalErrorChanged, this,
                &DaemonDiagnosticModel::hasFatalErrorChanged);
        connect(m_connector, &DaemonConnector::fatalErrorMessageChanged, this,
                &DaemonDiagnosticModel::fatalErrorMessageChanged);
        connect(m_connector, &DaemonConnector::lastErrorChanged, this, [this]() {
            m_lastError = m_connector->lastError();
            emit lastErrorChanged();
        });
        connect(m_connector, &DaemonConnector::statusMessageChanged, this, [this]() {
            m_statusMessage = m_connector->statusMessage();
            emit statusMessageChanged();
        });
    }

    emit connectedChanged();
    emit hasFatalErrorChanged();
    emit fatalErrorMessageChanged();
}

DaemonConnector* DaemonDiagnosticModel::daemonConnector() const noexcept {
    return m_connector;
}

void DaemonDiagnosticModel::setTextInjector(AbstractTextInjector* injector) {
    if (m_injector == injector) {
        return;
    }
    m_injector = injector;
}

AbstractTextInjector* DaemonDiagnosticModel::textInjector() const noexcept {
    return m_injector;
}

bool DaemonDiagnosticModel::isConnected() const {
    if (m_connector) {
        return m_connector->isConnected();
    }
    if (auto* client = DaemonClient::instance()) {
        return client->directTypingConnected();
    }
    return false;
}

bool DaemonDiagnosticModel::hasFatalError() const {
    if (m_connector) {
        return m_connector->hasFatalError();
    }
    if (auto* client = DaemonClient::instance()) {
        return client->directTypingHasFatalError();
    }
    return false;
}

QString DaemonDiagnosticModel::fatalErrorMessage() const {
    if (m_connector) {
        return m_connector->fatalErrorMessage();
    }
    if (auto* client = DaemonClient::instance()) {
        return client->directTypingFatalErrorMessage();
    }
    return QString();
}

QString DaemonDiagnosticModel::lastError() const {
    if (!m_lastError.isEmpty()) {
        return m_lastError;
    }
    if (m_connector) {
        return m_connector->lastError();
    }
    return QString();
}

QString DaemonDiagnosticModel::statusMessage() const {
    if (!m_statusMessage.isEmpty()) {
        return m_statusMessage;
    }
    if (m_connector) {
        return m_connector->statusMessage();
    }
    if (auto* client = DaemonClient::instance()) {
        return client->directTypingStatus();
    }
    return QString();
}

bool DaemonDiagnosticModel::preventClipboardHistory() const {
    return m_preventClipboardHistory;
}

void DaemonDiagnosticModel::setPreventClipboardHistory(bool prevent) {
    if (m_preventClipboardHistory != prevent) {
        m_preventClipboardHistory = prevent;
        {
            QSettings settings;
            settings.setValue(u"Clipboard/PreventHistory"_s, prevent);
            settings.sync();
            if (settings.status() != QSettings::NoError) {
                qWarning() << "DaemonDiagnosticModel: Failed to save Clipboard/PreventHistory:" << settings.status();
            }
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
        {
            QSettings settings;
            settings.setValue(u"Typing/PreInjectionDelayMs"_s, delayMs);
            settings.sync();
            if (settings.status() != QSettings::NoError) {
                qWarning() << "DaemonDiagnosticModel: Failed to save Typing/PreInjectionDelayMs:" << settings.status();
            }
        }
        emit injectionDelayChanged();
    }
}

bool DaemonDiagnosticModel::isKde() const {
    const QString desktop =
        QProcessEnvironment::systemEnvironment().value(u"XDG_CURRENT_DESKTOP"_s, QString()).toLower();
    return desktop.contains(u"kde"_s);
}

bool DaemonDiagnosticModel::isDevBuild() const {
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
    if (m_connector) {
        m_connector->connectToServer();
    } else if (auto* client = DaemonClient::instance()) {
        client->restartDirectTyping();
    }
}

void DaemonDiagnosticModel::disconnectFromServer() {
    if (m_connector) {
        m_connector->disconnectFromServer();
    } else if (auto* client = DaemonClient::instance()) {
        client->stopDirectTyping();
    }
}

void DaemonDiagnosticModel::stopDaemon() {
    if (m_connector) {
        m_connector->stopDaemon();
    } else if (auto* client = DaemonClient::instance()) {
        client->stopDirectTyping();
    }
}

void DaemonDiagnosticModel::restartService() {
    if (m_connector) {
        m_connector->restartService();
    } else if (auto* client = DaemonClient::instance()) {
        client->restartDirectTyping();
    }
}

bool DaemonDiagnosticModel::testTyping(const QString& text) {
    const QString injectionText = text.isEmpty() ? u" [QtScribe Test] "_s : text;
    if (m_injector) {
        return m_injector->inject(injectionText);
    }
    if (auto* client = DaemonClient::instance()) {
        client->testDirectTyping();
        return true;
    }
    return false;
}
