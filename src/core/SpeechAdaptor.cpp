#include "SpeechAdaptor.h"

#include "DictationCoordinator.h"

#include "HistoryRepository.h"

SpeechAdaptor::SpeechAdaptor(DictationCoordinator* coordinator, QObject* parent)
    : QDBusAbstractAdaptor(parent)
    , m_coordinator(coordinator) {
    setAutoRelaySignals(true);

    if (m_coordinator) {
        connect(m_coordinator, &DictationCoordinator::activeBackendChanged, this, &SpeechAdaptor::activeBackendChanged);
        connect(m_coordinator, &DictationCoordinator::recordingModeChanged, this, &SpeechAdaptor::recordingModeChanged);
        connect(m_coordinator, &DictationCoordinator::dictationStateChanged, this,
                &SpeechAdaptor::dictationStateChanged);
        connect(m_coordinator, &DictationCoordinator::canRecordChanged, this, &SpeechAdaptor::canRecordChanged);
        connect(m_coordinator, &DictationCoordinator::audioLevelChanged, this, &SpeechAdaptor::audioLevelChanged);
        connect(m_coordinator, &DictationCoordinator::statusMessageChanged, this, &SpeechAdaptor::statusMessageChanged);
        connect(m_coordinator, &DictationCoordinator::lastErrorChanged, this, &SpeechAdaptor::lastErrorChanged);
        connect(m_coordinator, &DictationCoordinator::recordingChanged, this, &SpeechAdaptor::recordingChanged);
        connect(m_coordinator, &DictationCoordinator::transcribingChanged, this, &SpeechAdaptor::transcribingChanged);
        connect(m_coordinator, &DictationCoordinator::enhancingChanged, this, &SpeechAdaptor::enhancingChanged);
        connect(m_coordinator, &DictationCoordinator::lastTranscriptionChanged, this,
                &SpeechAdaptor::lastTranscriptionChanged);
        connect(m_coordinator, &DictationCoordinator::autoSwitchOfflineChanged, this,
                &SpeechAdaptor::autoSwitchOfflineChanged);
        connect(m_coordinator, &DictationCoordinator::clientWindowFocusedChanged, this,
                &SpeechAdaptor::clientWindowFocusedChanged);
        connect(m_coordinator, &DictationCoordinator::cloudStatusChanged, this, &SpeechAdaptor::cloudStatusChanged);
        connect(m_coordinator, &DictationCoordinator::transcriptionFinished, this,
                &SpeechAdaptor::transcriptionFinished);
        connect(m_coordinator, &DictationCoordinator::historyCleared, this, &SpeechAdaptor::historyCleared);
        connect(m_coordinator, &DictationCoordinator::historyChanged, this, &SpeechAdaptor::historyChanged);
        connect(m_coordinator, &DictationCoordinator::errorOccurred, this, &SpeechAdaptor::errorOccurred);
    }
}

int SpeechAdaptor::activeBackend() const {
    return m_coordinator ? static_cast<int>(m_coordinator->activeBackend()) : 0;
}

void SpeechAdaptor::setActiveBackend(int backend, bool persistSettings) {
    if (m_coordinator) {
        m_coordinator->setActiveBackend(backend, persistSettings);
    }
}

int SpeechAdaptor::recordingMode() const {
    return m_coordinator ? static_cast<int>(m_coordinator->recordingMode()) : 0;
}

void SpeechAdaptor::setRecordingMode(int mode) {
    if (m_coordinator) {
        m_coordinator->setRecordingMode(mode);
    }
}

int SpeechAdaptor::dictationState() const {
    return m_coordinator ? static_cast<int>(m_coordinator->dictationState()) : 0;
}

bool SpeechAdaptor::isBusy() const {
    return m_coordinator ? m_coordinator->isBusy() : false;
}

bool SpeechAdaptor::canRecord() const {
    return m_coordinator ? m_coordinator->canRecord() : false;
}

double SpeechAdaptor::audioLevel() const {
    return m_coordinator ? static_cast<double>(m_coordinator->audioLevel()) : 0.0;
}

QString SpeechAdaptor::statusMessage() const {
    return m_coordinator ? m_coordinator->statusMessage() : QString();
}

QString SpeechAdaptor::lastError() const {
    return m_coordinator ? m_coordinator->lastError() : QString();
}

bool SpeechAdaptor::recording() const {
    return m_coordinator ? m_coordinator->recording() : false;
}

bool SpeechAdaptor::transcribing() const {
    return m_coordinator ? m_coordinator->transcribing() : false;
}

bool SpeechAdaptor::enhancing() const {
    return m_coordinator ? m_coordinator->enhancing() : false;
}

QString SpeechAdaptor::lastTranscription() const {
    return m_coordinator ? m_coordinator->lastTranscription() : QString();
}

bool SpeechAdaptor::autoSwitchOffline() const {
    return m_coordinator ? m_coordinator->autoSwitchOffline() : true;
}

void SpeechAdaptor::setAutoSwitchOffline(bool enable) {
    if (m_coordinator) {
        m_coordinator->setAutoSwitchOffline(enable);
    }
}

bool SpeechAdaptor::clientWindowFocused() const {
    return m_coordinator ? m_coordinator->clientWindowFocused() : false;
}

void SpeechAdaptor::setClientWindowFocused(bool focused) {
    if (m_coordinator) {
        m_coordinator->setClientWindowFocused(focused);
    }
}

bool SpeechAdaptor::cloudApiKeySet() const {
    return m_coordinator ? m_coordinator->cloudApiKeySet() : false;
}

bool SpeechAdaptor::cloudApiKeyInvalid() const {
    return m_coordinator ? m_coordinator->cloudApiKeyInvalid() : false;
}

bool SpeechAdaptor::cloudRateLimited() const {
    return m_coordinator ? m_coordinator->cloudRateLimited() : false;
}

int SpeechAdaptor::cloudRetrySecondsRemaining() const {
    return m_coordinator ? m_coordinator->cloudRetrySecondsRemaining() : 0;
}

QString SpeechAdaptor::cloudLastError() const {
    return m_coordinator ? m_coordinator->cloudLastError() : QString();
}

QString SpeechAdaptor::historyJson() const {
    if (m_coordinator && m_coordinator->historyRepository()) {
        return m_coordinator->historyRepository()->loadRecordsJson();
    }
    return QStringLiteral("[]");
}

QString SpeechAdaptor::getHistoryJson() const {
    return historyJson();
}

void SpeechAdaptor::toggleRecording() {
    if (m_coordinator) {
        m_coordinator->toggleRecording();
    }
}

void SpeechAdaptor::startRecording() {
    if (m_coordinator) {
        m_coordinator->startRecording();
    }
}

void SpeechAdaptor::stopRecording() {
    if (m_coordinator) {
        m_coordinator->stopRecording();
    }
}

void SpeechAdaptor::cancelDictation() {
    if (m_coordinator) {
        m_coordinator->cancelDictation();
    }
}

void SpeechAdaptor::retryTranscription() {
    if (m_coordinator) {
        m_coordinator->retryTranscription();
    }
}

void SpeechAdaptor::clearLastTranscription() {
    if (m_coordinator) {
        m_coordinator->clearLastTranscription();
    }
}

void SpeechAdaptor::clearError() {
    if (m_coordinator) {
        m_coordinator->clearError();
    }
}

void SpeechAdaptor::clearHistory() {
    if (m_coordinator) {
        m_coordinator->clearHistory();
    }
}

void SpeechAdaptor::reloadSettings() {
    if (m_coordinator) {
        m_coordinator->reloadSettings();
    }
}
