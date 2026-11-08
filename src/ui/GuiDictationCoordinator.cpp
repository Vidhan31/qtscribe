#include "GuiDictationCoordinator.h"

GuiDictationCoordinator::GuiDictationCoordinator(QObject* parent)
    : QObject(parent) {
    if (auto* client = DaemonClient::instance()) {
        connect(client, &DaemonClient::activeBackendChanged, this, &GuiDictationCoordinator::activeBackendChanged);
        connect(client, &DaemonClient::recordingModeChanged, this, &GuiDictationCoordinator::recordingModeChanged);
        connect(client, &DaemonClient::dictationStateChanged, this, &GuiDictationCoordinator::dictationStateChanged);
        connect(client, &DaemonClient::canRecordChanged, this, &GuiDictationCoordinator::canRecordChanged);
        connect(client, &DaemonClient::audioLevelChanged, this, &GuiDictationCoordinator::audioLevelChanged);
        connect(client, &DaemonClient::statusMessageChanged, this, &GuiDictationCoordinator::statusMessageChanged);
        connect(client, &DaemonClient::lastErrorChanged, this, &GuiDictationCoordinator::lastErrorChanged);
        connect(client, &DaemonClient::recordingChanged, this, &GuiDictationCoordinator::recordingChanged);
        connect(client, &DaemonClient::transcribingChanged, this, &GuiDictationCoordinator::transcribingChanged);
        connect(client, &DaemonClient::enhancingChanged, this, &GuiDictationCoordinator::enhancingChanged);
        connect(client, &DaemonClient::lastTranscriptionChanged, this,
                &GuiDictationCoordinator::lastTranscriptionChanged);
        connect(client, &DaemonClient::autoSwitchOfflineChanged, this,
                &GuiDictationCoordinator::autoSwitchOfflineChanged);
        connect(client, &DaemonClient::clientWindowFocusedChanged, this,
                &GuiDictationCoordinator::clientWindowFocusedChanged);
        connect(client, &DaemonClient::shortcutStatusChanged, this, &GuiDictationCoordinator::shortcutStatusChanged);
        connect(client, &DaemonClient::directTypingStatusChanged, this,
                &GuiDictationCoordinator::directTypingStatusChanged);
        connect(client, &DaemonClient::transcriptionFinished, this, &GuiDictationCoordinator::transcriptionFinished);
        connect(client, &DaemonClient::errorOccurred, this, &GuiDictationCoordinator::errorOccurred);
        connect(client, &DaemonClient::requestShowWindow, this, &GuiDictationCoordinator::requestShowWindow);
        connect(client, &DaemonClient::requestNavigate, this, &GuiDictationCoordinator::requestNavigate);
        connect(client, &DaemonClient::requestQuitApp, this, &GuiDictationCoordinator::requestQuitApp);
    }
}

GuiDictationCoordinator::TranscriptionBackend GuiDictationCoordinator::activeBackend() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? static_cast<TranscriptionBackend>(client->activeBackend()) : TranscriptionBackend::Cloud;
}

GuiDictationCoordinator::RecordingMode GuiDictationCoordinator::recordingMode() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? static_cast<RecordingMode>(client->recordingMode()) : RecordingMode::Toggle;
}

GuiDictationCoordinator::DictationState GuiDictationCoordinator::dictationState() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? static_cast<DictationState>(client->dictationState()) : DictationState::Idle;
}

bool GuiDictationCoordinator::isBusy() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->isBusy() : false;
}

bool GuiDictationCoordinator::canRecord() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->canRecord() : false;
}

qreal GuiDictationCoordinator::audioLevel() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->audioLevel() : 0.0;
}

QString GuiDictationCoordinator::statusMessage() const {
    auto* client = DaemonClient::instance();
    return client ? client->statusMessage() : QString();
}

QString GuiDictationCoordinator::lastError() const {
    auto* client = DaemonClient::instance();
    return client ? client->lastError() : QString();
}

bool GuiDictationCoordinator::recording() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->recording() : false;
}

bool GuiDictationCoordinator::transcribing() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->transcribing() : false;
}

bool GuiDictationCoordinator::enhancing() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->enhancing() : false;
}

QString GuiDictationCoordinator::lastTranscription() const {
    auto* client = DaemonClient::instance();
    return client ? client->lastTranscription() : QString();
}

bool GuiDictationCoordinator::autoSwitchOffline() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->autoSwitchOffline() : true;
}

bool GuiDictationCoordinator::clientWindowFocused() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->clientWindowFocused() : false;
}

bool GuiDictationCoordinator::pushToTalkSupported() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->pushToTalkSupported() : false;
}

bool GuiDictationCoordinator::shortcutAvailable() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->shortcutAvailable() : false;
}

bool GuiDictationCoordinator::shortcutSupported() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->shortcutSupported() : false;
}

QString GuiDictationCoordinator::shortcutStatusMessage() const {
    auto* client = DaemonClient::instance();
    return client ? client->shortcutStatusMessage() : QString();
}

bool GuiDictationCoordinator::directTypingConnected() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->directTypingConnected() : false;
}

bool GuiDictationCoordinator::directTypingHasFatalError() const noexcept {
    auto* client = DaemonClient::instance();
    return client ? client->directTypingHasFatalError() : false;
}

QString GuiDictationCoordinator::directTypingFatalErrorMessage() const {
    auto* client = DaemonClient::instance();
    return client ? client->directTypingFatalErrorMessage() : QString();
}

QString GuiDictationCoordinator::directTypingStatus() const {
    auto* client = DaemonClient::instance();
    return client ? client->directTypingStatus() : QString();
}

void GuiDictationCoordinator::setActiveBackend(TranscriptionBackend backend, bool persistSettings) {
    if (auto* client = DaemonClient::instance()) {
        client->setActiveBackend(static_cast<int>(backend), persistSettings);
    }
}

void GuiDictationCoordinator::setRecordingMode(RecordingMode mode) {
    if (auto* client = DaemonClient::instance()) {
        client->setRecordingMode(static_cast<int>(mode));
    }
}

void GuiDictationCoordinator::setAutoSwitchOffline(bool enable) {
    if (auto* client = DaemonClient::instance()) {
        client->setAutoSwitchOffline(enable);
    }
}

void GuiDictationCoordinator::setClientWindowFocused(bool focused) {
    if (auto* client = DaemonClient::instance()) {
        client->setClientWindowFocused(focused);
    }
}

void GuiDictationCoordinator::startRecording() {
    if (auto* client = DaemonClient::instance()) {
        client->startRecording();
    }
}

void GuiDictationCoordinator::stopRecording() {
    if (auto* client = DaemonClient::instance()) {
        client->stopRecording();
    }
}

void GuiDictationCoordinator::toggleRecording() {
    if (auto* client = DaemonClient::instance()) {
        client->toggleRecording();
    }
}

void GuiDictationCoordinator::cancelDictation() {
    if (auto* client = DaemonClient::instance()) {
        client->cancelDictation();
    }
}

void GuiDictationCoordinator::retryTranscription() {
    if (auto* client = DaemonClient::instance()) {
        client->retryTranscription();
    }
}

void GuiDictationCoordinator::clearLastTranscription() {
    if (auto* client = DaemonClient::instance()) {
        client->clearLastTranscription();
    }
}

void GuiDictationCoordinator::clearError() {
    if (auto* client = DaemonClient::instance()) {
        client->clearError();
    }
}

void GuiDictationCoordinator::reloadSettings() {
    if (auto* client = DaemonClient::instance()) {
        client->reloadSettings();
    }
}

void GuiDictationCoordinator::testDirectTyping() {
    if (auto* client = DaemonClient::instance()) {
        client->testDirectTyping();
    }
}

void GuiDictationCoordinator::restartDirectTyping() {
    if (auto* client = DaemonClient::instance()) {
        client->restartDirectTyping();
    }
}

void GuiDictationCoordinator::stopDirectTyping() {
    if (auto* client = DaemonClient::instance()) {
        client->stopDirectTyping();
    }
}
