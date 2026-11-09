#include "SystemService.h"

#include "AudioRecorder.h"
#include "DaemonConnector.h"
#include "DictationCoordinator.h"
#include "LoggingCategories.h"
#include "TextInjector.h"

#include "AudioFeedbackPlayer.h"
#include "GlobalShortcutManager.h"

using namespace Qt::StringLiterals;

SystemService::SystemService(QObject* parent)
    : QObject(parent) {
    connect(this, &SystemService::textInjectionStatusChanged, this, &SystemService::directTypingStatusChanged);
}

void SystemService::setShortcutManager(GlobalShortcutManager* mgr) {
    if (m_shortcutMgr == mgr) {
        return;
    }

    if (m_shortcutMgr) {
        disconnect(m_shortcutMgr, &GlobalShortcutManager::supportedChanged, this,
                   &SystemService::shortcutStatusChanged);
        disconnect(m_shortcutMgr, &GlobalShortcutManager::availableChanged, this,
                   &SystemService::shortcutStatusChanged);
        disconnect(m_shortcutMgr, &GlobalShortcutManager::statusMessageChanged, this,
                   &SystemService::shortcutStatusChanged);
    }

    m_shortcutMgr = mgr;

    if (m_shortcutMgr) {
        connect(m_shortcutMgr, &GlobalShortcutManager::supportedChanged, this, &SystemService::shortcutStatusChanged);
        connect(m_shortcutMgr, &GlobalShortcutManager::availableChanged, this, &SystemService::shortcutStatusChanged);
        connect(m_shortcutMgr, &GlobalShortcutManager::statusMessageChanged, this,
                &SystemService::shortcutStatusChanged);
    }

    emit shortcutStatusChanged();
}

GlobalShortcutManager* SystemService::shortcutManager() const noexcept {
    return m_shortcutMgr;
}

void SystemService::setDaemonConnector(DaemonConnector* connector) {
    if (m_daemonConnector == connector) {
        return;
    }

    if (m_daemonConnector) {
        disconnect(m_daemonConnector, &DaemonConnector::connectedChanged, this,
                   &SystemService::textInjectionStatusChanged);
        disconnect(m_daemonConnector, &DaemonConnector::hasFatalErrorChanged, this,
                   &SystemService::textInjectionStatusChanged);
        disconnect(m_daemonConnector, &DaemonConnector::statusMessageChanged, this,
                   &SystemService::textInjectionStatusChanged);
    }

    m_daemonConnector = connector;

    if (m_daemonConnector) {
        connect(m_daemonConnector, &DaemonConnector::connectedChanged, this,
                &SystemService::textInjectionStatusChanged);
        connect(m_daemonConnector, &DaemonConnector::hasFatalErrorChanged, this,
                &SystemService::textInjectionStatusChanged);
        connect(m_daemonConnector, &DaemonConnector::statusMessageChanged, this,
                &SystemService::textInjectionStatusChanged);
    }

    emit textInjectionStatusChanged();
}

DaemonConnector* SystemService::daemonConnector() const noexcept {
    return m_daemonConnector;
}

void SystemService::setTextInjector(AbstractTextInjector* injector) {
    m_injector = injector;
}

AbstractTextInjector* SystemService::textInjector() const noexcept {
    return m_injector;
}

void SystemService::setAudioRecorder(AudioRecorder* recorder) {
    if (m_recorder == recorder) {
        return;
    }

    if (m_recorder) {
        disconnect(m_recorder, &AudioRecorder::hasAudioInputDeviceChanged, this,
                   &SystemService::microphoneStatusChanged);
        disconnect(m_recorder, &AudioRecorder::audioLevelChanged, this, nullptr);
    }

    m_recorder = recorder;

    if (m_recorder) {
        connect(m_recorder, &AudioRecorder::hasAudioInputDeviceChanged, this, &SystemService::microphoneStatusChanged);
        connect(m_recorder, &AudioRecorder::audioLevelChanged, this, [this]() {
            if (m_microphoneTestActive && m_recorder) {
                onMicrophoneTestAudioLevel(m_recorder->audioLevel());
            }
        });
        connect(m_recorder, &AudioRecorder::recordingChanged, this, [this]() {
            if (m_microphoneTestActive && m_recorder && !m_recorder->recording()) {
                m_microphoneTestActive = false;
                m_microphoneTestLevel = 0.0;
                emit microphoneTestChanged();
                emit microphoneTestLevelChanged(0.0);
            }
        });
    }

    emit microphoneStatusChanged();
}

void SystemService::setDictationCoordinator(DictationCoordinator* coordinator) {
    m_coordinator = coordinator;
}

DictationCoordinator* SystemService::dictationCoordinator() const noexcept {
    return m_coordinator;
}

AudioRecorder* SystemService::audioRecorder() const noexcept {
    return m_recorder;
}

void SystemService::setAudioFeedbackPlayer(AudioFeedbackPlayer* player) {
    m_feedbackPlayer = player;
}

AudioFeedbackPlayer* SystemService::audioFeedbackPlayer() const noexcept {
    return m_feedbackPlayer;
}

bool SystemService::pushToTalkSupported() const noexcept {
    return m_shortcutMgr && m_shortcutMgr->isSupported();
}

bool SystemService::shortcutAvailable() const noexcept {
    return m_shortcutMgr && m_shortcutMgr->isAvailable();
}

bool SystemService::shortcutSupported() const noexcept {
    return m_shortcutMgr && m_shortcutMgr->isSupported();
}

QString SystemService::shortcutStatusMessage() const {
    return m_shortcutMgr ? m_shortcutMgr->statusMessage() : QString();
}

bool SystemService::textInjectionConnected() const noexcept {
    return m_daemonConnector && m_daemonConnector->isConnected();
}

bool SystemService::directTypingConnected() const noexcept {
    return textInjectionConnected();
}

bool SystemService::textInjectionHasFatalError() const noexcept {
    return m_daemonConnector && m_daemonConnector->hasFatalError();
}

bool SystemService::directTypingHasFatalError() const noexcept {
    return textInjectionHasFatalError();
}

QString SystemService::textInjectionFatalErrorMessage() const {
    return m_daemonConnector ? m_daemonConnector->fatalErrorMessage() : QString();
}

QString SystemService::directTypingFatalErrorMessage() const {
    return textInjectionFatalErrorMessage();
}

QString SystemService::textInjectionStatus() const {
    return m_daemonConnector ? m_daemonConnector->statusMessage() : QString();
}

QString SystemService::directTypingStatus() const {
    return textInjectionStatus();
}

bool SystemService::microphoneAvailable() const noexcept {
    return m_recorder && m_recorder->hasAudioInputDevice();
}

bool SystemService::microphoneTestActive() const noexcept {
    return m_microphoneTestActive;
}

qreal SystemService::microphoneTestLevel() const noexcept {
    return m_microphoneTestLevel;
}

void SystemService::testTextInjection() {
    if (m_injector) {
        m_injector->inject(u"Hello from QtScribe!"_s);
    }
}

void SystemService::testDirectTyping() {
    testTextInjection();
}

void SystemService::restartTextInjection() {
    if (m_daemonConnector) {
        m_daemonConnector->restartService();
    }
}

void SystemService::restartDirectTyping() {
    restartTextInjection();
}

void SystemService::stopTextInjection() {
    if (m_daemonConnector) {
        m_daemonConnector->stopDaemon();
    }
}

void SystemService::stopDirectTyping() {
    stopTextInjection();
}

void SystemService::startMicrophoneTest() {
    if (!m_recorder || m_microphoneTestActive) {
        return;
    }

    if (m_recorder->recording() || (m_coordinator && m_coordinator->isBusy())) {
        qCDebug(lcSpeech)
            << "SystemService: Cannot start microphone test while dictation is active or recorder is busy";
        return;
    }

    m_microphoneTestActive = true;
    m_microphoneTestLevel = 0.0;
    emit microphoneTestChanged();
    emit microphoneTestLevelChanged(m_microphoneTestLevel);

    m_recorder->startRecording();
    if (!m_recorder->recording()) {
        m_microphoneTestActive = false;
        m_microphoneTestLevel = 0.0;
        emit microphoneTestChanged();
        emit microphoneTestLevelChanged(0.0);
    }
}

void SystemService::stopMicrophoneTest() {
    if (!m_microphoneTestActive) {
        return;
    }

    m_microphoneTestActive = false;
    m_microphoneTestLevel = 0.0;
    if (m_recorder) {
        m_recorder->cancelRecording();
    }
    emit microphoneTestChanged();
    emit microphoneTestLevelChanged(m_microphoneTestLevel);
}

void SystemService::onMicrophoneTestAudioLevel(qreal level) {
    if (!m_microphoneTestActive) {
        return;
    }
    m_microphoneTestLevel = level;
    emit microphoneTestLevelChanged(level);
}

void SystemService::playStartSound() {
    if (m_feedbackPlayer) {
        m_feedbackPlayer->playStartSound();
    }
}

void SystemService::playStopSound() {
    if (m_feedbackPlayer) {
        m_feedbackPlayer->playStopSound();
    }
}

void SystemService::showWindow() {
    emit requestShowWindow();
}

void SystemService::quitApp() {
    emit requestQuitApp();
}

void SystemService::navigateToSection(const QString& section) {
    emit requestNavigate(section);
}
