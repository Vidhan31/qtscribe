#include "DaemonDiagnosticModel.h"

#include "AbstractTextInjector.h"
#include "DaemonConnector.h"
#include "TextInjector.h"

#include <QSettings>

#include <algorithm>
#include <array>
#include <ranges>

using namespace Qt::StringLiterals;

DaemonDiagnosticModel::DaemonDiagnosticModel(QObject* parent)
    : QObject(parent) {
    QSettings settings;
    m_preventClipboardHistory = settings.value(u"Clipboard/PreventHistory"_s, true).toBool();
    m_injectionDelay = settings.value(u"Typing/PreInjectionDelayMs"_s, 200).toInt();
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
    emit statusMessageChanged();
    emit lastErrorChanged();
}

DaemonConnector* DaemonDiagnosticModel::daemonConnector() const noexcept {
    return m_connector;
}

void DaemonDiagnosticModel::setTextInjector(AbstractTextInjector* injector) {
    m_injector = injector;
    if (auto* concrete = qobject_cast<TextInjector*>(m_injector)) {
        concrete->setInjectionDelay(m_injectionDelay);
        concrete->setPreventClipboardHistory(m_preventClipboardHistory);
    }
}

AbstractTextInjector* DaemonDiagnosticModel::textInjector() const noexcept {
    return m_injector;
}

bool DaemonDiagnosticModel::isConnected() const {
    return m_connector && m_connector->isConnected();
}

bool DaemonDiagnosticModel::hasFatalError() const {
    return m_connector && m_connector->hasFatalError();
}

QString DaemonDiagnosticModel::fatalErrorMessage() const {
    return m_connector ? m_connector->fatalErrorMessage() : QString();
}

QString DaemonDiagnosticModel::lastError() const {
    return !m_lastError.isEmpty() ? m_lastError : (m_connector ? m_connector->lastError() : QString());
}

QString DaemonDiagnosticModel::statusMessage() const {
    return !m_statusMessage.isEmpty() ? m_statusMessage : (m_connector ? m_connector->statusMessage() : QString());
}

bool DaemonDiagnosticModel::preventClipboardHistory() const {
    return m_preventClipboardHistory;
}

void DaemonDiagnosticModel::setPreventClipboardHistory(bool prevent) {
    if (m_preventClipboardHistory != prevent) {
        m_preventClipboardHistory = prevent;
        QSettings settings;
        settings.setValue(u"Clipboard/PreventHistory"_s, prevent);
        if (auto* concrete = qobject_cast<TextInjector*>(m_injector)) {
            concrete->setPreventClipboardHistory(prevent);
        }
        emit preventClipboardHistoryChanged();
    }
}

int DaemonDiagnosticModel::injectionDelay() const {
    return m_injectionDelay;
}

void DaemonDiagnosticModel::setInjectionDelay(int delayMs) {
    delayMs = std::clamp(delayMs, 0, 10000);
    if (m_injectionDelay != delayMs) {
        m_injectionDelay = delayMs;
        QSettings settings;
        settings.setValue(u"Typing/PreInjectionDelayMs"_s, delayMs);
        if (auto* concrete = qobject_cast<TextInjector*>(m_injector)) {
            concrete->setInjectionDelay(delayMs);
        }
        emit injectionDelayChanged();
    }
}

bool DaemonDiagnosticModel::isKde() const {
    if (!qEnvironmentVariableIsEmpty("KDE_FULL_SESSION")) {
        return true;
    }
    const auto desktops = std::to_array<QStringView>(
        {qEnvironmentVariable("XDG_CURRENT_DESKTOP"), qEnvironmentVariable("XDG_SESSION_DESKTOP")});
    return std::ranges::any_of(desktops, [](QStringView dt) {
        return dt.contains(u"kde", Qt::CaseInsensitive) || dt.contains(u"plasma", Qt::CaseInsensitive);
    });
}

bool DaemonDiagnosticModel::isDevBuild() const {
#ifdef QT_DEBUG
    return true;
#else
    return !qEnvironmentVariableIsEmpty("QTSCRIBE_DEV") || !qEnvironmentVariableIsEmpty("QTSCRIBE_DEV_MODE");
#endif
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
    const bool eligibleEnvironment = isDevBuild() || !isKde();
    return eligibleEnvironment && !m_clipboardWarningAcknowledged;
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
    }
}

void DaemonDiagnosticModel::disconnectFromServer() {
    if (m_connector) {
        m_connector->disconnectFromServer();
    }
}

void DaemonDiagnosticModel::stopDaemon() {
    if (m_connector) {
        m_connector->stopDaemon();
    }
}

void DaemonDiagnosticModel::restartService() {
    if (m_connector) {
        m_connector->restartService();
    }
}

bool DaemonDiagnosticModel::testTyping(const QString& text) {
    const QString injectionText = text.isEmpty() ? u" [QtScribe Test] "_s : text;
    if (m_injector) {
        return m_injector->inject(injectionText);
    }
    return false;
}
