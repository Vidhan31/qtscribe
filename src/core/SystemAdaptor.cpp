#include "SystemAdaptor.h"

#include "SystemService.h"

SystemAdaptor::SystemAdaptor(SystemService* service, QObject* parent)
    : QDBusAbstractAdaptor(parent)
    , m_service(service) {
    setAutoRelaySignals(true);

    if (m_service) {
        connect(m_service, &SystemService::shortcutStatusChanged, this, &SystemAdaptor::shortcutStatusChanged);
        connect(m_service, &SystemService::textInjectionStatusChanged, this,
                &SystemAdaptor::textInjectionStatusChanged);
        connect(m_service, &SystemService::directTypingStatusChanged, this, &SystemAdaptor::directTypingStatusChanged);
        connect(m_service, &SystemService::microphoneStatusChanged, this, &SystemAdaptor::microphoneStatusChanged);
        connect(m_service, &SystemService::microphoneTestChanged, this, &SystemAdaptor::microphoneTestChanged);
        connect(m_service, &SystemService::microphoneTestLevelChanged, this,
                &SystemAdaptor::microphoneTestLevelChanged);
        connect(m_service, &SystemService::requestShowWindow, this, &SystemAdaptor::requestShowWindow);
        connect(m_service, &SystemService::requestNavigate, this, &SystemAdaptor::requestNavigate);
        connect(m_service, &SystemService::requestQuitApp, this, &SystemAdaptor::requestQuitApp);
    }
}

bool SystemAdaptor::pushToTalkSupported() const {
    return m_service && m_service->pushToTalkSupported();
}

bool SystemAdaptor::shortcutAvailable() const {
    return m_service && m_service->shortcutAvailable();
}

bool SystemAdaptor::shortcutSupported() const {
    return m_service && m_service->shortcutSupported();
}

QString SystemAdaptor::shortcutStatusMessage() const {
    return m_service ? m_service->shortcutStatusMessage() : QString();
}

bool SystemAdaptor::textInjectionConnected() const {
    return m_service && m_service->textInjectionConnected();
}

bool SystemAdaptor::directTypingConnected() const {
    return textInjectionConnected();
}

bool SystemAdaptor::textInjectionHasFatalError() const {
    return m_service && m_service->textInjectionHasFatalError();
}

bool SystemAdaptor::directTypingHasFatalError() const {
    return textInjectionHasFatalError();
}

QString SystemAdaptor::textInjectionFatalErrorMessage() const {
    return m_service ? m_service->textInjectionFatalErrorMessage() : QString();
}

QString SystemAdaptor::directTypingFatalErrorMessage() const {
    return textInjectionFatalErrorMessage();
}

QString SystemAdaptor::textInjectionStatus() const {
    return m_service ? m_service->textInjectionStatus() : QString();
}

QString SystemAdaptor::directTypingStatus() const {
    return textInjectionStatus();
}

bool SystemAdaptor::microphoneAvailable() const {
    return m_service && m_service->microphoneAvailable();
}

bool SystemAdaptor::microphoneTestActive() const {
    return m_service && m_service->microphoneTestActive();
}

double SystemAdaptor::microphoneTestLevel() const {
    return m_service ? static_cast<double>(m_service->microphoneTestLevel()) : 0.0;
}

void SystemAdaptor::testTextInjection() {
    if (m_service) {
        m_service->testTextInjection();
    }
}

void SystemAdaptor::testDirectTyping() {
    testTextInjection();
}

void SystemAdaptor::restartTextInjection() {
    if (m_service) {
        m_service->restartTextInjection();
    }
}

void SystemAdaptor::restartDirectTyping() {
    restartTextInjection();
}

void SystemAdaptor::stopTextInjection() {
    if (m_service) {
        m_service->stopTextInjection();
    }
}

void SystemAdaptor::stopDirectTyping() {
    stopTextInjection();
}

void SystemAdaptor::startMicrophoneTest() {
    if (m_service) {
        m_service->startMicrophoneTest();
    }
}

void SystemAdaptor::stopMicrophoneTest() {
    if (m_service) {
        m_service->stopMicrophoneTest();
    }
}

void SystemAdaptor::playStartSound() {
    if (m_service) {
        m_service->playStartSound();
    }
}

void SystemAdaptor::playStopSound() {
    if (m_service) {
        m_service->playStopSound();
    }
}

void SystemAdaptor::showWindow() {
    if (m_service) {
        m_service->showWindow();
    }
}

void SystemAdaptor::quitApp() {
    if (m_service) {
        m_service->quitApp();
    }
}

void SystemAdaptor::navigateToSection(const QString& section) {
    if (m_service) {
        m_service->navigateToSection(section);
    }
}
