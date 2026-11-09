#include "SystemClient.h"

#include <QCoreApplication>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDebug>

using namespace Qt::StringLiterals;

static const auto kServiceName = u"io.github.qtscribe.SpeechService"_s;
static const auto kObjectPath = u"/io/github/qtscribe/SpeechService"_s;
static const auto kInterface = u"io.github.qtscribe.SystemService"_s;

SystemClient::SystemClient(QObject* parent)
    : QObject(parent) {
    s_instance = this;
    connect(this, &SystemClient::textInjectionStatusChanged, this, &SystemClient::directTypingStatusChanged);

    const bool disableRemoteAttach = !qEnvironmentVariableIsEmpty("QTSCRIBE_DISABLE_REMOTE_ATTACH") &&
                                     qEnvironmentVariable("QTSCRIBE_DISABLE_REMOTE_ATTACH") != "0";
    if (disableRemoteAttach) {
        return;
    }

    if (QDBusConnection::sessionBus().isConnected()) {
        m_watcher = new QDBusServiceWatcher(
            kServiceName, QDBusConnection::sessionBus(),
            QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration, this);
        connect(m_watcher, &QDBusServiceWatcher::serviceRegistered, this, &SystemClient::onServiceRegistered);
        connect(m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, &SystemClient::onServiceUnregistered);

        auto* iface = QDBusConnection::sessionBus().interface();
        if (iface && iface->isServiceRegistered(kServiceName)) {
            onServiceRegistered(kServiceName);
        }
    }
}

SystemClient::~SystemClient() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

SystemClient* SystemClient::instance() {
    if (!s_instance) {
        s_instance = new SystemClient(QCoreApplication::instance());
    }
    return s_instance;
}

SystemClient* SystemClient::create(QQmlEngine* /*qmlEngine*/, QJSEngine* /*jsEngine*/) {
    return instance();
}

bool SystemClient::isConnected() const noexcept {
    return m_connected;
}

bool SystemClient::pushToTalkSupported() const noexcept {
    return m_pushToTalkSupported;
}

bool SystemClient::shortcutAvailable() const noexcept {
    return m_shortcutAvailable;
}

bool SystemClient::shortcutSupported() const noexcept {
    return m_shortcutSupported;
}

QString SystemClient::shortcutStatusMessage() const {
    return m_shortcutStatusMessage;
}

bool SystemClient::systemShortcutHasIssue() const noexcept {
    return !m_shortcutSupported || !m_shortcutAvailable;
}

bool SystemClient::textInjectionConnected() const noexcept {
    return m_directTypingConnected;
}

bool SystemClient::directTypingConnected() const noexcept {
    return textInjectionConnected();
}

bool SystemClient::textInjectionHasFatalError() const noexcept {
    return m_directTypingHasFatalError;
}

bool SystemClient::directTypingHasFatalError() const noexcept {
    return textInjectionHasFatalError();
}

QString SystemClient::textInjectionFatalErrorMessage() const {
    return m_directTypingFatalErrorMessage;
}

QString SystemClient::directTypingFatalErrorMessage() const {
    return textInjectionFatalErrorMessage();
}

QString SystemClient::textInjectionStatus() const {
    return m_directTypingStatus;
}

QString SystemClient::directTypingStatus() const {
    return textInjectionStatus();
}

bool SystemClient::textInjectionHasIssue() const noexcept {
    return m_directTypingHasFatalError || !m_directTypingConnected;
}

bool SystemClient::directTypingHasIssue() const noexcept {
    return textInjectionHasIssue();
}

bool SystemClient::microphoneAvailable() const noexcept {
    return m_microphoneAvailable;
}

bool SystemClient::microphoneTestActive() const noexcept {
    return m_microphoneTestActive;
}

qreal SystemClient::microphoneTestLevel() const noexcept {
    return m_microphoneTestLevel;
}

bool SystemClient::sendRemoteCommand(const QString& command) {
    if (!QDBusConnection::sessionBus().isConnected()) {
        return false;
    }
    QDBusInterface iface(kServiceName, kObjectPath, kInterface, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        return false;
    }
    const QDBusMessage reply = iface.call(command);
    return reply.type() != QDBusMessage::ErrorMessage;
}

void SystemClient::testTextInjection() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"testTextInjection"_s);
    }
}

void SystemClient::testDirectTyping() {
    testTextInjection();
}

void SystemClient::restartTextInjection() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"restartTextInjection"_s);
    }
}

void SystemClient::restartDirectTyping() {
    restartTextInjection();
}

void SystemClient::stopTextInjection() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"stopTextInjection"_s);
    }
}

void SystemClient::stopDirectTyping() {
    stopTextInjection();
}

void SystemClient::startMicrophoneTest() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"startMicrophoneTest"_s);
    }
}

void SystemClient::stopMicrophoneTest() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"stopMicrophoneTest"_s);
    }
}

void SystemClient::playStartSound() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"playStartSound"_s);
    }
}

void SystemClient::playStopSound() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"playStopSound"_s);
    }
}

void SystemClient::showWindow() {
    emit requestShowWindow();
}

void SystemClient::quitApp() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"quitApp"_s);
    }
    emit requestQuitApp();
}

void SystemClient::navigateToSection(const QString& section) {
    emit requestNavigate(section);
}

void SystemClient::onServiceRegistered(const QString& serviceName) {
    if (serviceName != kServiceName) {
        return;
    }

    if (!m_interface) {
        m_interface = new QDBusInterface(kServiceName, kObjectPath, kInterface, QDBusConnection::sessionBus(), this);
    }

    setupDBusConnections();
    m_connected = true;
    emit connectedChanged();
    syncFromDaemon();
}

void SystemClient::onServiceUnregistered(const QString& serviceName) {
    if (serviceName != kServiceName) {
        return;
    }

    m_connected = false;
    m_microphoneAvailable = false;
    m_microphoneTestActive = false;
    m_microphoneTestLevel = 0.0;

    emit connectedChanged();
    emit shortcutStatusChanged();
    emit directTypingStatusChanged();
    emit microphoneStatusChanged();
    emit microphoneTestChanged();
    emit microphoneTestLevelChanged(0.0);
}

void SystemClient::setupDBusConnections() {
    if (!QDBusConnection::sessionBus().isConnected()) {
        return;
    }

    auto bus = QDBusConnection::sessionBus();

    bus.connect(kServiceName, kObjectPath, u"org.freedesktop.DBus.Properties"_s, u"PropertiesChanged"_s, this,
                SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));

    bus.connect(kServiceName, kObjectPath, kInterface, u"shortcutStatusChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"textInjectionStatusChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"directTypingStatusChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"microphoneStatusChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"microphoneTestChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"microphoneTestLevelChanged"_s, this,
                SLOT(onMicrophoneTestLevelChanged(double)));

    bus.connect(kServiceName, kObjectPath, kInterface, u"requestShowWindow"_s, this, SIGNAL(requestShowWindow()));

    bus.connect(kServiceName, kObjectPath, kInterface, u"requestNavigate"_s, this, SIGNAL(requestNavigate(QString)));

    bus.connect(kServiceName, kObjectPath, kInterface, u"requestQuitApp"_s, this, SIGNAL(requestQuitApp()));
}

void SystemClient::onMicrophoneTestLevelChanged(double level) {
    m_microphoneTestLevel = level;
    emit microphoneTestLevelChanged(m_microphoneTestLevel);
}

void SystemClient::syncFromDaemon() {
    if (!m_interface || !m_interface->isValid()) {
        return;
    }

    m_pushToTalkSupported = m_interface->property("pushToTalkSupported").toBool();
    m_shortcutAvailable = m_interface->property("shortcutAvailable").toBool();
    m_shortcutSupported = m_interface->property("shortcutSupported").toBool();
    m_shortcutStatusMessage = m_interface->property("shortcutStatusMessage").toString();

    const auto propConnected = m_interface->property("textInjectionConnected");
    m_directTypingConnected =
        propConnected.isValid() ? propConnected.toBool() : m_interface->property("directTypingConnected").toBool();

    const auto propFatal = m_interface->property("textInjectionHasFatalError");
    m_directTypingHasFatalError =
        propFatal.isValid() ? propFatal.toBool() : m_interface->property("directTypingHasFatalError").toBool();

    const auto propFatalMsg = m_interface->property("textInjectionFatalErrorMessage");
    m_directTypingFatalErrorMessage = propFatalMsg.isValid()
                                          ? propFatalMsg.toString()
                                          : m_interface->property("directTypingFatalErrorMessage").toString();

    const auto propStatus = m_interface->property("textInjectionStatus");
    m_directTypingStatus =
        propStatus.isValid() ? propStatus.toString() : m_interface->property("directTypingStatus").toString();

    m_microphoneAvailable = m_interface->property("microphoneAvailable").toBool();
    m_microphoneTestActive = m_interface->property("microphoneTestActive").toBool();
    m_microphoneTestLevel = m_interface->property("microphoneTestLevel").toReal();

    emit shortcutStatusChanged();
    emit textInjectionStatusChanged();
    emit microphoneStatusChanged();
    emit microphoneTestChanged();
    emit microphoneTestLevelChanged(m_microphoneTestLevel);
}

void SystemClient::onPropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties,
                                       const QStringList& invalidatedProperties) {
    Q_UNUSED(invalidatedProperties);
    if (interfaceName != kInterface) {
        return;
    }

    bool shortcutChanged = false;
    bool directTypingChanged = false;

    for (auto it = changedProperties.constBegin(); it != changedProperties.constEnd(); ++it) {
        const QString& key = it.key();
        const QVariant& val = it.value();

        if (key == u"pushToTalkSupported"_s) {
            m_pushToTalkSupported = val.toBool();
            shortcutChanged = true;
        } else if (key == u"shortcutAvailable"_s) {
            m_shortcutAvailable = val.toBool();
            shortcutChanged = true;
        } else if (key == u"shortcutSupported"_s) {
            m_shortcutSupported = val.toBool();
            shortcutChanged = true;
        } else if (key == u"shortcutStatusMessage"_s) {
            m_shortcutStatusMessage = val.toString();
            shortcutChanged = true;
        } else if (key == u"textInjectionConnected"_s || key == u"directTypingConnected"_s) {
            m_directTypingConnected = val.toBool();
            directTypingChanged = true;
        } else if (key == u"textInjectionHasFatalError"_s || key == u"directTypingHasFatalError"_s) {
            m_directTypingHasFatalError = val.toBool();
            directTypingChanged = true;
        } else if (key == u"textInjectionFatalErrorMessage"_s || key == u"directTypingFatalErrorMessage"_s) {
            m_directTypingFatalErrorMessage = val.toString();
            directTypingChanged = true;
        } else if (key == u"textInjectionStatus"_s || key == u"directTypingStatus"_s) {
            m_directTypingStatus = val.toString();
            directTypingChanged = true;
        } else if (key == u"microphoneAvailable"_s) {
            m_microphoneAvailable = val.toBool();
            emit microphoneStatusChanged();
        } else if (key == u"microphoneTestActive"_s) {
            m_microphoneTestActive = val.toBool();
            emit microphoneTestChanged();
        } else if (key == u"microphoneTestLevel"_s) {
            m_microphoneTestLevel = val.toReal();
            emit microphoneTestLevelChanged(m_microphoneTestLevel);
        }
    }

    if (shortcutChanged) {
        emit shortcutStatusChanged();
    }
    if (directTypingChanged) {
        emit textInjectionStatusChanged();
    }
}
