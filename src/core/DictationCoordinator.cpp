#include "DictationCoordinator.h"

#include "AbstractTextInjector.h"
#include "AudioRecorder.h"
#include "LoggingCategories.h"

#include "AbstractLlmClient.h"
#include "CompositeSttEngine.h"
#include "GlobalShortcutManager.h"

#include <QDebug>
#include <QGuiApplication>
#include <QSettings>

using namespace Qt::StringLiterals;

DictationCoordinator::DictationCoordinator(QObject* parent)
    : QObject(parent) {
    QSettings settings;
    const QString modeStr = settings.value(u"Speech/RecordingMode"_s, u"Toggle"_s).toString();
    if (modeStr == u"PushToTalk"_s) {
        m_recordingMode = RecordingMode::PushToTalk;
    } else {
        m_recordingMode = RecordingMode::Toggle;
    }

    const QString backendStr = settings.value(u"Speech/Backend"_s, u"WhisperCpp"_s).toString();
    if (backendStr == u"Cloud"_s || backendStr == u"Groq"_s) {
        m_activeBackend = TranscriptionBackend::Cloud;
    } else {
        m_activeBackend = TranscriptionBackend::WhisperCpp;
    }

    m_autoSwitchOffline = settings.value(u"Speech/AutoSwitchOffline"_s, true).toBool();

    setStatusMessage(tr("Ready"));
    qCDebug(lcSpeech) << "DictationCoordinator pure orchestrator constructed. Initial backend:"
                      << (m_activeBackend == TranscriptionBackend::WhisperCpp ? "WhisperCpp" : "Cloud")
                      << "mode:" << (m_recordingMode == RecordingMode::PushToTalk ? "PushToTalk" : "Toggle")
                      << "autoSwitchOffline:" << m_autoSwitchOffline;
}

void DictationCoordinator::setAudioRecorder(AudioRecorder* recorder) {
    if (m_recorder == recorder) {
        return;
    }

    if (m_recorder) {
        disconnect(m_recorder, &AudioRecorder::recordingFinished, this, &DictationCoordinator::onRecordingFinished);
        disconnect(m_recorder, &AudioRecorder::maxDurationReached, this, &DictationCoordinator::onMaxDurationReached);
        disconnect(m_recorder, &AudioRecorder::hasAudioInputDeviceChanged, this,
                   &DictationCoordinator::updateCoordinatorHealth);
        disconnect(m_recorder, &AudioRecorder::audioLevelChanged, this, nullptr);
    }

    m_recorder = recorder;

    if (m_recorder) {
        connect(m_recorder, &AudioRecorder::recordingFinished, this, &DictationCoordinator::onRecordingFinished);
        connect(m_recorder, &AudioRecorder::maxDurationReached, this, &DictationCoordinator::onMaxDurationReached);
        connect(m_recorder, &AudioRecorder::hasAudioInputDeviceChanged, this, [this]() {
            if (m_recorder) {
                if (!m_recorder->hasAudioInputDevice()) {
                    const QString errMsg = tr("No microphone found. Please connect a microphone.");
                    setLastError(errMsg);
                    setStatusMessage(tr("No microphone found"));
                } else if (m_lastError == tr("No microphone found. Please connect a microphone.")) {
                    clearError();
                    setStatusMessage(tr("Ready"));
                }
            }
            updateCoordinatorHealth();
        });
        connect(m_recorder, &AudioRecorder::audioLevelChanged, this, [this]() {
            if (m_recorder) {
                emit audioLevelChanged(m_recorder->audioLevel());
            }
        });
    }

    updateCoordinatorHealth();
}

void DictationCoordinator::setSttEngine(AbstractSttClient* engine) {
    if (m_sttEngine == engine) {
        return;
    }

    if (m_sttEngine) {
        disconnect(m_sttEngine, &AbstractSttClient::transcriptionReady, this,
                   &DictationCoordinator::onSttTranscriptionReady);
        disconnect(m_sttEngine, &AbstractSttClient::errorOccurred, this, &DictationCoordinator::onSttError);
        disconnect(m_sttEngine, &AbstractSttClient::readyChanged, this, &DictationCoordinator::updateCoordinatorHealth);

        if (auto* comp = qobject_cast<CompositeSttEngine*>(m_sttEngine)) {
            disconnect(comp, &CompositeSttEngine::activeBackendChanged, this,
                       &DictationCoordinator::activeBackendChanged);
            disconnect(comp, &CompositeSttEngine::autoSwitchOfflineChanged, this,
                       &DictationCoordinator::autoSwitchOfflineChanged);
        }
    }

    m_sttEngine = engine;

    if (m_sttEngine) {
        connect(m_sttEngine, &AbstractSttClient::transcriptionReady, this,
                &DictationCoordinator::onSttTranscriptionReady);
        connect(m_sttEngine, &AbstractSttClient::errorOccurred, this, &DictationCoordinator::onSttError);
        connect(m_sttEngine, &AbstractSttClient::readyChanged, this, &DictationCoordinator::updateCoordinatorHealth);

        if (auto* comp = qobject_cast<CompositeSttEngine*>(m_sttEngine)) {
            connect(comp, &CompositeSttEngine::activeBackendChanged, this, &DictationCoordinator::activeBackendChanged);
            connect(comp, &CompositeSttEngine::autoSwitchOfflineChanged, this,
                    &DictationCoordinator::autoSwitchOfflineChanged);
        }

        if (m_initialized) {
            m_sttEngine->activate();
        }
    }

    updateCoordinatorHealth();
}

AbstractSttClient* DictationCoordinator::sttEngine() const noexcept {
    return m_sttEngine;
}

AbstractSttClient* DictationCoordinator::activeSttClient() const noexcept {
    return m_sttEngine;
}

void DictationCoordinator::setLlmClient(AbstractLlmClient* llmClient) {
    if (m_llmClient == llmClient) {
        return;
    }

    if (m_llmClient) {
        disconnect(m_llmClient, &AbstractLlmClient::enhancementReady, this,
                   &DictationCoordinator::onLlmEnhancementReady);
        disconnect(m_llmClient, &AbstractLlmClient::errorOccurred, this, &DictationCoordinator::onLlmError);
    }

    m_llmClient = llmClient;

    if (m_llmClient) {
        connect(m_llmClient, &AbstractLlmClient::enhancementReady, this, &DictationCoordinator::onLlmEnhancementReady);
        connect(m_llmClient, &AbstractLlmClient::errorOccurred, this, &DictationCoordinator::onLlmError);
    }
}

void DictationCoordinator::setShortcutManager(GlobalShortcutManager* mgr) {
    if (m_shortcutMgr == mgr) {
        return;
    }

    if (m_shortcutMgr) {
        disconnect(m_shortcutMgr, &GlobalShortcutManager::shortcutActivated, this,
                   &DictationCoordinator::onShortcutActivated);
        disconnect(m_shortcutMgr, &GlobalShortcutManager::shortcutDeactivated, this,
                   &DictationCoordinator::onShortcutDeactivated);
        disconnect(m_shortcutMgr, &GlobalShortcutManager::supportedChanged, this,
                   &DictationCoordinator::updateCoordinatorHealth);
    }

    m_shortcutMgr = mgr;

    if (m_shortcutMgr) {
        connect(m_shortcutMgr, &GlobalShortcutManager::shortcutActivated, this,
                &DictationCoordinator::onShortcutActivated);
        connect(m_shortcutMgr, &GlobalShortcutManager::shortcutDeactivated, this,
                &DictationCoordinator::onShortcutDeactivated);
        connect(m_shortcutMgr, &GlobalShortcutManager::supportedChanged, this,
                &DictationCoordinator::updateCoordinatorHealth);
    }

    updateCoordinatorHealth();
}

void DictationCoordinator::setTextInjector(AbstractTextInjector* injector) {
    m_injector = injector;
    updateCoordinatorHealth();
}

AbstractTextInjector* DictationCoordinator::textInjector() const noexcept {
    return m_injector;
}

DictationCoordinator::TranscriptionBackend DictationCoordinator::activeBackend() const {
    if (auto* comp = qobject_cast<CompositeSttEngine*>(m_sttEngine)) {
        return static_cast<TranscriptionBackend>(comp->activeBackend());
    }
    return m_activeBackend;
}

void DictationCoordinator::setActiveBackend(TranscriptionBackend backend, bool persistSettings) {
    if (auto* comp = qobject_cast<CompositeSttEngine*>(m_sttEngine)) {
        comp->setActiveBackend(static_cast<CompositeSttEngine::TranscriptionBackend>(backend), persistSettings);
        return;
    }

    if (m_activeBackend != backend) {
        if (isBusy()) {
            cancelDictation();
        }
        m_activeBackend = backend;
        if (persistSettings) {
            QSettings settings;
            settings.setValue(u"Speech/TranscriptionBackend"_s, static_cast<int>(backend));
            settings.setValue(u"Speech/Backend"_s,
                              m_activeBackend == TranscriptionBackend::WhisperCpp ? u"WhisperCpp"_s : u"Cloud"_s);
        }
        emit activeBackendChanged();
        updateCoordinatorHealth();
    }
}

bool DictationCoordinator::autoSwitchOffline() const {
    if (auto* comp = qobject_cast<CompositeSttEngine*>(m_sttEngine)) {
        return comp->autoSwitchOffline();
    }
    return m_autoSwitchOffline;
}

void DictationCoordinator::setAutoSwitchOffline(bool enable) {
    if (auto* comp = qobject_cast<CompositeSttEngine*>(m_sttEngine)) {
        comp->setAutoSwitchOffline(enable);
        return;
    }

    if (m_autoSwitchOffline != enable) {
        m_autoSwitchOffline = enable;
        QSettings settings;
        settings.setValue(u"Speech/AutoSwitchOffline"_s, enable);
        emit autoSwitchOfflineChanged();
    }
}

DictationCoordinator::RecordingMode DictationCoordinator::recordingMode() const {
    return m_recordingMode;
}

void DictationCoordinator::setRecordingMode(RecordingMode mode) {
    const bool pttSupported = m_shortcutMgr ? m_shortcutMgr->isSupported() : false;
    if (mode == RecordingMode::PushToTalk && !pttSupported) {
        qCDebug(lcSpeech)
            << "Push-to-Talk requested but not supported on current desktop environment; falling back to Toggle";
        mode = RecordingMode::Toggle;
    }

    if (m_recordingMode != mode) {
        m_recordingMode = mode;
        QSettings settings;
        settings.setValue(u"Speech/RecordingMode"_s,
                          m_recordingMode == RecordingMode::PushToTalk ? u"PushToTalk"_s : u"Toggle"_s);
        emit recordingModeChanged();
    }
}

void DictationCoordinator::initialize() {
    if (m_initialized) {
        return;
    }
    m_initialized = true;
    if (m_sttEngine) {
        m_sttEngine->activate();
    }
    updateCoordinatorHealth();
    qCDebug(lcSpeech) << "DictationCoordinator: initialized and ready for dictation";
}

void DictationCoordinator::updateCoordinatorHealth() {
    const bool pttSupported = m_shortcutMgr ? m_shortcutMgr->isSupported() : false;
    if (m_recordingMode == RecordingMode::PushToTalk && !pttSupported) {
        qCDebug(lcSpeech) << "Push-to-talk not supported by portal; reverting to Toggle mode";
        m_recordingMode = RecordingMode::Toggle;
        QSettings settings;
        settings.setValue(u"Speech/RecordingMode"_s, u"Toggle"_s);
        emit recordingModeChanged();
    }
    emit canRecordChanged();
}

DictationCoordinator::DictationState DictationCoordinator::dictationState() const {
    return m_state;
}

bool DictationCoordinator::isBusy() const {
    return m_state == DictationState::Recording || m_state == DictationState::Transcribing ||
           m_state == DictationState::Enhancing;
}

bool DictationCoordinator::canRecord() const {
    const bool notProcessing = (m_state != DictationState::Transcribing && m_state != DictationState::Enhancing);
    const bool micReady = m_recorder && m_recorder->hasAudioInputDevice();
    const bool sttReady = m_sttEngine ? m_sttEngine->isReady() : false;
    return notProcessing && micReady && sttReady;
}

qreal DictationCoordinator::audioLevel() const {
    return m_recorder ? m_recorder->audioLevel() : 0.0;
}

QString DictationCoordinator::statusMessage() const {
    return m_statusMessage;
}

QString DictationCoordinator::lastError() const {
    return m_lastError;
}

bool DictationCoordinator::recording() const {
    return m_state == DictationState::Recording;
}

bool DictationCoordinator::transcribing() const {
    return m_state == DictationState::Transcribing;
}

bool DictationCoordinator::enhancing() const {
    return m_state == DictationState::Enhancing;
}

QString DictationCoordinator::lastTranscription() const {
    return m_lastTranscription;
}

void DictationCoordinator::showWindow() {
    qCDebug(lcSpeech) << "DictationCoordinator: showWindow requested via IPC/D-Bus";
    emit requestShowWindow();
}

void DictationCoordinator::navigateToSection(const QString& section) {
    qCDebug(lcSpeech) << "DictationCoordinator: navigateToSection requested:" << section;
    showWindow();
    emit requestNavigate(section);
}

void DictationCoordinator::quitApp() {
    qCDebug(lcSpeech) << "DictationCoordinator: quitApp requested via IPC/D-Bus";
    emit requestQuitApp();
}

void DictationCoordinator::startRecording() {
    if (isBusy()) {
        qCDebug(lcSpeech) << "DictationCoordinator: startRecording ignored — already busy";
        return;
    }

    if (m_injector) {
        m_injector->cancel();
    }

    if (!m_recorder || !m_recorder->hasAudioInputDevice()) {
        qWarning() << "DictationCoordinator: No microphone input device available";
        const QString errMsg = tr("No microphone found. Please connect a microphone.");
        setLastError(errMsg);
        setStatusMessage(tr("No microphone found"));
        setState(DictationState::Error);
        emit errorOccurred(errMsg);
        return;
    }

    setLastError({});
    setState(DictationState::Recording);
    setStatusMessage(tr("Listening…"));
    m_recorder->startRecording();
}

void DictationCoordinator::stopRecording() {
    if (m_state != DictationState::Recording) {
        qCDebug(lcSpeech) << "DictationCoordinator: stopRecording ignored — not currently recording";
        return;
    }

    if (m_recorder) {
        m_recorder->stopRecording();
    }
}

void DictationCoordinator::toggleRecording() {
    if (m_state == DictationState::Recording) {
        stopRecording();
    } else if (m_state == DictationState::Idle || m_state == DictationState::Error) {
        startRecording();
    } else {
        qCDebug(lcSpeech) << "DictationCoordinator: Toggle ignored while busy (state:" << static_cast<int>(m_state)
                          << ")";
    }
}

void DictationCoordinator::cancelDictation() {
    if (m_state == DictationState::Idle) {
        return;
    }

    qCDebug(lcSpeech) << "DictationCoordinator: Cancelling operation in state:" << static_cast<int>(m_state);

    if (m_injector) {
        m_injector->cancel();
    }

    if (m_state == DictationState::Recording && m_recorder) {
        m_recorder->cancelRecording();
    }

    if (m_sttEngine) {
        m_sttEngine->cancel();
    }

    if (m_llmClient) {
        m_llmClient->cancel();
    }

    m_lastWavData.clear();
    setState(DictationState::Idle);
    setStatusMessage(tr("Ready"));
}

void DictationCoordinator::retryTranscription() {
    if (m_state != DictationState::Error || m_lastWavData.isEmpty()) {
        qCDebug(lcSpeech) << "DictationCoordinator: Cannot retry — not in Error state or no audio cached";
        return;
    }

    setLastError({});
    setState(DictationState::Transcribing);
    setStatusMessage(tr("Retrying transcription…"));

    if (m_sttEngine) {
        m_sttEngine->transcribe(m_lastWavData);
    } else {
        setLastError(tr("Speech-to-text service is unavailable"));
        setState(DictationState::Error);
    }
}

void DictationCoordinator::clearLastTranscription() {
    if (!m_lastTranscription.isEmpty()) {
        m_lastTranscription.clear();
        emit lastTranscriptionChanged();
    }
}

void DictationCoordinator::clearError() {
    setLastError({});
    if (m_state == DictationState::Error) {
        setState(DictationState::Idle);
        setStatusMessage(tr("Ready"));
    }
}

void DictationCoordinator::onRecordingFinished(const QByteArray& wavData) {
    if (m_state != DictationState::Recording) {
        qCDebug(lcSpeech) << "DictationCoordinator: Ignoring recordingFinished — not in Recording state";
        return;
    }

    if (wavData.isEmpty()) {
        qCDebug(lcSpeech) << "DictationCoordinator: Empty audio payload, resetting to Idle";
        setState(DictationState::Idle);
        setStatusMessage(tr("Ready"));
        return;
    }

    m_lastWavData = wavData;
    setState(DictationState::Transcribing);
    setStatusMessage(tr("Transcribing audio…"));

    if (m_sttEngine) {
        m_sttEngine->transcribe(wavData);
    } else {
        setLastError(tr("Speech-to-text service is unavailable"));
        setState(DictationState::Error);
    }
}

void DictationCoordinator::onMaxDurationReached() {
    qWarning() << "DictationCoordinator: Maximum recording duration safety limit reached";
    emit maxDurationWarningTriggered();
    stopRecording();
}

void DictationCoordinator::onSttTranscriptionReady(const QString& text) {
    if (m_state != DictationState::Transcribing || (sender() && sender() != m_sttEngine)) {
        qCDebug(lcSpeech) << "DictationCoordinator: Ignoring transcriptionReady: not in Transcribing state or sender "
                             "is not active STT engine";
        return;
    }

    qCDebug(lcSpeech) << "DictationCoordinator: STT transcription received (" << text.size() << "chars)";

    if (text.trimmed().isEmpty()) {
        setState(DictationState::Idle);
        setStatusMessage(tr("Ready"));
        updateCoordinatorHealth();
        return;
    }

    const bool isCloud = (activeBackend() == TranscriptionBackend::Cloud);
    if (isCloud && m_llmClient && m_llmClient->enabled()) {
        if (m_sttEngine && m_sttEngine->handlesSmartFormatting()) {
            qCDebug(lcSpeech)
                << "DictationCoordinator: Active STT engine handled smart formatting natively; skipping LLM pass";
            completeTranscription(text);
            return;
        }

        qCDebug(lcSpeech) << "DictationCoordinator: Passing transcription to LLM post-processing...";
        setState(DictationState::Enhancing);
        setStatusMessage(tr("Enhancing text with AI…"));
        m_llmClient->processText(text);
        updateCoordinatorHealth();
        return;
    }

    completeTranscription(text);
}

void DictationCoordinator::onLlmEnhancementReady(const QString& enhancedText) {
    qCDebug(lcSpeech) << "DictationCoordinator: LLM enhanced text received (" << enhancedText.size() << "chars)";
    completeTranscription(enhancedText);
}

void DictationCoordinator::onLlmError(const QString& error, const QString& fallbackRawText) {
    qWarning() << "DictationCoordinator: LLM enhancement failed:" << error << "-> Falling back to raw transcription.";
    emit llmFallbackWarningTriggered(error);

    if (!fallbackRawText.isEmpty()) {
        completeTranscription(fallbackRawText);
        setStatusMessage(tr("Ready (Used raw Whisper text)"));
    } else {
        setState(DictationState::Idle);
        setStatusMessage(tr("Ready"));
        updateCoordinatorHealth();
    }
}

void DictationCoordinator::completeTranscription(const QString& text) {
    m_lastTranscription = text;
    emit lastTranscriptionChanged();
    setState(DictationState::Idle);
    setStatusMessage(tr("Ready"));
    updateCoordinatorHealth();
    finishTranscriptionAndInject(text);
    emit transcriptionFinished(text);
}

void DictationCoordinator::finishTranscriptionAndInject(const QString& text) {
    if (text.isEmpty()) {
        return;
    }

    const bool isAppActive = (QGuiApplication::focusWindow() != nullptr);
    if (m_injector && !isAppActive) {
        m_injector->inject(text);
    }
}

void DictationCoordinator::onSttError(const QString& error) {
    if (m_state != DictationState::Transcribing || (sender() && sender() != m_sttEngine)) {
        qCDebug(lcSpeech)
            << "DictationCoordinator: Ignoring STT error: not in Transcribing state or sender is not active STT engine";
        return;
    }

    qWarning() << "DictationCoordinator: STT Error:" << error;
    setLastError(error);
    setStatusMessage(error);
    setState(DictationState::Error);
    updateCoordinatorHealth();
    emit errorOccurred(error);
}

void DictationCoordinator::onShortcutActivated(const QString& shortcutId) {
    if (shortcutId == u"toggle-recording"_s || shortcutId == u"record"_s) {
        qCDebug(lcSpeech) << "DictationCoordinator: Global shortcut activated (mode:"
                          << (m_recordingMode == RecordingMode::PushToTalk ? "PushToTalk" : "Toggle") << ")";
        if (m_recordingMode == RecordingMode::PushToTalk) {
            if (!recording() && canRecord()) {
                startRecording();
            }
        } else {
            toggleRecording();
        }
    }
}

void DictationCoordinator::onShortcutDeactivated(const QString& shortcutId) {
    if (shortcutId == u"toggle-recording"_s || shortcutId == u"record"_s) {
        qCDebug(lcSpeech) << "DictationCoordinator: Global shortcut deactivated (mode:"
                          << (m_recordingMode == RecordingMode::PushToTalk ? "PushToTalk" : "Toggle") << ")";
        if (m_recordingMode == RecordingMode::PushToTalk) {
            if (recording()) {
                stopRecording();
            }
        }
    }
}

void DictationCoordinator::setState(DictationState state) {
    if (m_state != state) {
        m_state = state;
        emit dictationStateChanged();
        emit recordingChanged();
        emit transcribingChanged();
        emit enhancingChanged();
        emit canRecordChanged();
    }
}

void DictationCoordinator::setStatusMessage(const QString& msg) {
    if (m_statusMessage != msg) {
        m_statusMessage = msg;
        emit statusMessageChanged();
    }
}

void DictationCoordinator::setLastError(const QString& error) {
    if (m_lastError != error) {
        m_lastError = error;
        emit lastErrorChanged();
    }
}
