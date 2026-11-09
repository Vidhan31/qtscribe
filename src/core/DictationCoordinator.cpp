#include "DictationCoordinator.h"

#include "AbstractTextInjector.h"
#include "AudioRecorder.h"
#include "LoggingCategories.h"

#include "common/HistoryPaths.h"
#include "common/SettingsKeys.h"
#include "AbstractCloudSttClient.h"
#include "AbstractLlmClient.h"
#include "AbstractSttClient.h"
#include "ApplicationContext.h"
#include "CompositeSttEngine.h"
#include "GlobalShortcutManager.h"
#include "HistoryRepository.h"
#include "SettingsManager.h"
#include "SystemService.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

DictationCoordinator::~DictationCoordinator() = default;

DictationCoordinator::DictationCoordinator(QObject* parent)
    : QObject(parent) {
    if (auto* inst = SettingsManager::instance()) {
        setSettingsManager(inst);
    } else {
        loadSettings();
    }

    setStatusMessage(tr("Ready"));
    qCDebug(lcSpeech) << "DictationCoordinator pure orchestrator constructed. Initial backend:"
                      << (m_activeBackend == TranscriptionBackend::WhisperCpp ? "WhisperCpp" : "Cloud")
                      << "mode:" << (m_recordingMode == RecordingMode::PushToTalk ? "PushToTalk" : "Toggle")
                      << "autoSwitchOffline:" << m_autoSwitchOffline;
}

void DictationCoordinator::setSettingsManager(SettingsManager* settings) {
    if (m_settingsManager == settings) {
        return;
    }
    if (m_settingsManager) {
        disconnect(m_settingsManager, nullptr, this, nullptr);
    }
    m_settingsManager = settings;
    if (m_settingsManager) {
        connect(m_settingsManager, &SettingsManager::speechConfigChanged, this,
                [this](const SettingsManager::SpeechConfig& config) {
                    const auto backend = (config.backend == SettingsManager::TranscriptionBackend::WhisperCpp)
                                             ? TranscriptionBackend::WhisperCpp
                                             : TranscriptionBackend::Cloud;
                    auto* comp = qobject_cast<CompositeSttEngine*>(m_sttEngine);
                    const bool isOfflineFallbackActive = comp && comp->isAutoSwitchedToOffline();
                    if (!isOfflineFallbackActive || backend == TranscriptionBackend::WhisperCpp) {
                        setActiveBackend(backend, false);
                    }
                    m_autoSwitchOffline = config.autoSwitchOffline;
                    emit autoSwitchOfflineChanged();
                    const auto mode = (config.recordingMode == SettingsManager::RecordingMode::PushToTalk)
                                          ? RecordingMode::PushToTalk
                                          : RecordingMode::Toggle;
                    if (m_recordingMode != mode) {
                        m_recordingMode = mode;
                        emit recordingModeChanged();
                    }
                });
    }
    loadSettings();
}

SettingsManager* DictationCoordinator::settingsManager() const noexcept {
    return m_settingsManager;
}

void DictationCoordinator::setHistoryRepository(HistoryRepository* repo) {
    if (m_historyRepository == repo) {
        return;
    }
    if (m_historyRepository && m_historyRepository != m_ownedHistoryRepository.get()) {
        disconnect(m_historyRepository, nullptr, this, nullptr);
    }
    m_historyRepository = repo;
    if (m_historyRepository) {
        connect(m_historyRepository, &HistoryRepository::historyCleared, this, &DictationCoordinator::historyCleared);
        connect(m_historyRepository, &HistoryRepository::historyChanged, this, &DictationCoordinator::historyChanged);
    }
}

HistoryRepository* DictationCoordinator::historyRepository() {
    if (!m_historyRepository) {
        m_ownedHistoryRepository = std::make_unique<HistoryRepository>(QString(), this);
        setHistoryRepository(m_ownedHistoryRepository.get());
    }
    return m_historyRepository;
}

void DictationCoordinator::setAudioRecorder(AudioRecorder* recorder) {
    if (m_recorder == recorder) {
        return;
    }

    if (m_recorder) {
        disconnect(m_recorder, &AudioRecorder::recordingFinished, this, &DictationCoordinator::onRecordingFinished);
        disconnect(m_recorder, &AudioRecorder::recordingChanged, this, nullptr);
        disconnect(m_recorder, &AudioRecorder::maxDurationReached, this, &DictationCoordinator::onMaxDurationReached);
        disconnect(m_recorder, &AudioRecorder::hasAudioInputDeviceChanged, this,
                   &DictationCoordinator::updateCoordinatorHealth);
        disconnect(m_recorder, &AudioRecorder::audioLevelChanged, this, nullptr);
    }

    m_recorder = recorder;

    if (m_recorder) {
        connect(m_recorder, &AudioRecorder::recordingFinished, this, &DictationCoordinator::onRecordingFinished);
        connect(m_recorder, &AudioRecorder::maxDurationReached, this, &DictationCoordinator::onMaxDurationReached);
        connect(m_recorder, &AudioRecorder::recordingChanged, this, [this]() {
            QMetaObject::invokeMethod(
                this,
                [this]() {
                    if (m_recorder && !m_recorder->recording() && m_state == DictationState::Recording) {
                        qCDebug(lcSpeech)
                            << "DictationCoordinator: Recorder stopped without finish event; resetting to Idle";
                        setState(DictationState::Idle);
                        setStatusMessage(tr("Ready"));
                    }
                },
                Qt::QueuedConnection);
        });
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

void DictationCoordinator::setSystemService(SystemService* systemService) {
    if (m_systemService == systemService) {
        return;
    }

    if (m_systemService) {
        disconnect(m_systemService, &SystemService::microphoneTestChanged, this,
                   &DictationCoordinator::canRecordChanged);
    }

    m_systemService = systemService;

    if (m_systemService) {
        connect(m_systemService, &SystemService::microphoneTestChanged, this, &DictationCoordinator::canRecordChanged);
    }

    emit canRecordChanged();
}

SystemService* DictationCoordinator::systemService() const noexcept {
    return m_systemService;
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
            disconnect(comp, &CompositeSttEngine::cloudClientChanged, this, &DictationCoordinator::setCloudClient);
        }
    }

    m_sttEngine = engine;

    if (m_sttEngine) {
        connect(m_sttEngine, &AbstractSttClient::transcriptionReady, this,
                &DictationCoordinator::onSttTranscriptionReady);
        connect(m_sttEngine, &AbstractSttClient::errorOccurred, this, &DictationCoordinator::onSttError);
        connect(m_sttEngine, &AbstractSttClient::readyChanged, this, &DictationCoordinator::updateCoordinatorHealth);

        if (auto* comp = qobject_cast<CompositeSttEngine*>(m_sttEngine)) {
            connect(comp, &CompositeSttEngine::activeBackendChanged, this, [this]() {
                if (m_state == DictationState::Enhancing) {
                    if (m_llmClient) {
                        m_llmClient->cancel();
                    }
                    const QString rawText = m_pendingRawText;
                    m_pendingRawText.clear();
                    if (!rawText.isEmpty()) {
                        completeTranscription(rawText);
                        setStatusMessage(tr("Ready (Used raw text)"));
                    } else {
                        cancelDictation();
                    }
                } else if (isBusy()) {
                    cancelDictation();
                }
                emit activeBackendChanged();
            });
            connect(comp, &CompositeSttEngine::autoSwitchOfflineChanged, this,
                    &DictationCoordinator::autoSwitchOfflineChanged);
            connect(comp, &CompositeSttEngine::cloudClientChanged, this, &DictationCoordinator::setCloudClient);
            setCloudClient(comp->cloudClient());
        } else if (auto* cloud = qobject_cast<AbstractCloudSttClient*>(m_sttEngine)) {
            setCloudClient(cloud);
        } else {
            setCloudClient(nullptr);
        }

        if (m_initialized) {
            m_sttEngine->activate();
        }
    } else {
        setCloudClient(nullptr);
    }

    updateCoordinatorHealth();
}

void DictationCoordinator::setCloudClient(AbstractCloudSttClient* client) {
    if (m_cloudClient == client) {
        return;
    }

    if (m_cloudClient) {
        disconnect(m_cloudClient, &AbstractCloudSttClient::apiKeySetChanged, this,
                   &DictationCoordinator::cloudStatusChanged);
        disconnect(m_cloudClient, &AbstractCloudSttClient::isApiKeyInvalidChanged, this,
                   &DictationCoordinator::cloudStatusChanged);
        disconnect(m_cloudClient, &AbstractCloudSttClient::isRateLimitedChanged, this,
                   &DictationCoordinator::cloudStatusChanged);
        disconnect(m_cloudClient, &AbstractCloudSttClient::retrySecondsRemainingChanged, this,
                   &DictationCoordinator::cloudStatusChanged);
        disconnect(m_cloudClient, &AbstractCloudSttClient::errorCategoryChanged, this,
                   &DictationCoordinator::cloudStatusChanged);
        disconnect(m_cloudClient, &AbstractSttClient::lastErrorChanged, this,
                   &DictationCoordinator::cloudStatusChanged);
    }

    m_cloudClient = client;

    if (m_cloudClient) {
        connect(m_cloudClient, &AbstractCloudSttClient::apiKeySetChanged, this,
                &DictationCoordinator::cloudStatusChanged);
        connect(m_cloudClient, &AbstractCloudSttClient::isApiKeyInvalidChanged, this,
                &DictationCoordinator::cloudStatusChanged);
        connect(m_cloudClient, &AbstractCloudSttClient::isRateLimitedChanged, this,
                &DictationCoordinator::cloudStatusChanged);
        connect(m_cloudClient, &AbstractCloudSttClient::retrySecondsRemainingChanged, this,
                &DictationCoordinator::cloudStatusChanged);
        connect(m_cloudClient, &AbstractCloudSttClient::errorCategoryChanged, this,
                &DictationCoordinator::cloudStatusChanged);
        connect(m_cloudClient, &AbstractSttClient::lastErrorChanged, this, &DictationCoordinator::cloudStatusChanged);
    }

    emit cloudStatusChanged();
}

AbstractCloudSttClient* DictationCoordinator::cloudClient() const noexcept {
    return m_cloudClient;
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
    }

    m_shortcutMgr = mgr;

    if (m_shortcutMgr) {
        connect(m_shortcutMgr, &GlobalShortcutManager::shortcutActivated, this,
                &DictationCoordinator::onShortcutActivated);
        connect(m_shortcutMgr, &GlobalShortcutManager::shortcutDeactivated, this,
                &DictationCoordinator::onShortcutDeactivated);
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

bool DictationCoordinator::cloudApiKeySet() const {
    return m_cloudClient && m_cloudClient->isApiKeySet();
}

bool DictationCoordinator::cloudApiKeyInvalid() const {
    return m_cloudClient && m_cloudClient->isApiKeyInvalid();
}

bool DictationCoordinator::cloudRateLimited() const {
    return m_cloudClient && m_cloudClient->isRateLimited();
}

int DictationCoordinator::cloudRetrySecondsRemaining() const {
    return m_cloudClient ? m_cloudClient->retrySecondsRemaining() : 0;
}

QString DictationCoordinator::cloudLastError() const {
    return m_cloudClient ? m_cloudClient->lastError() : QString();
}

DictationCoordinator::TranscriptionBackend DictationCoordinator::activeBackend() const {
    if (auto* comp = qobject_cast<CompositeSttEngine*>(m_sttEngine)) {
        return static_cast<TranscriptionBackend>(comp->activeBackend());
    }
    return m_activeBackend;
}

void DictationCoordinator::setActiveBackend(TranscriptionBackend backend, bool persistSettings) {
    if (auto* comp = qobject_cast<CompositeSttEngine*>(m_sttEngine)) {
        if (isBusy() && comp->activeBackend() != static_cast<CompositeSttEngine::TranscriptionBackend>(backend)) {
            if (m_state == DictationState::Enhancing) {
                if (m_llmClient) {
                    m_llmClient->cancel();
                }
                const QString rawText = m_pendingRawText;
                m_pendingRawText.clear();
                if (!rawText.isEmpty()) {
                    completeTranscription(rawText);
                    setStatusMessage(tr("Ready (Used raw text)"));
                } else {
                    cancelDictation();
                }
            } else {
                cancelDictation();
            }
        }
        comp->setActiveBackend(static_cast<CompositeSttEngine::TranscriptionBackend>(backend), persistSettings);
        return;
    }

    if (m_activeBackend != backend) {
        if (isBusy()) {
            cancelDictation();
        }
        m_activeBackend = backend;
        if (persistSettings) {
            if (m_settingsManager) {
                m_settingsManager->setBackend(m_activeBackend == TranscriptionBackend::WhisperCpp
                                                  ? SettingsManager::TranscriptionBackend::WhisperCpp
                                                  : SettingsManager::TranscriptionBackend::Cloud);
            } else {
                QSettings settings;
                settings.setValue(SettingsKeys::kSpeechTranscriptionBackend.toString(), static_cast<int>(backend));
                settings.setValue(SettingsKeys::kSpeechBackend.toString(),
                                  m_activeBackend == TranscriptionBackend::WhisperCpp ? u"WhisperCpp"_s : u"Cloud"_s);
            }
        }
        emit activeBackendChanged();
        updateCoordinatorHealth();
    }
}

void DictationCoordinator::setActiveBackend(int backend, bool persistSettings) {
    setActiveBackend(backend == 1 ? TranscriptionBackend::WhisperCpp : TranscriptionBackend::Cloud, persistSettings);
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
        if (m_settingsManager) {
            m_settingsManager->setAutoSwitchOffline(enable);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kSpeechAutoSwitchOffline.toString(), enable);
        }
        emit autoSwitchOfflineChanged();
    }
}

DictationCoordinator::RecordingMode DictationCoordinator::recordingMode() const {
    return m_recordingMode;
}

void DictationCoordinator::setRecordingMode(RecordingMode mode) {
    if (mode == RecordingMode::PushToTalk && m_shortcutMgr && !m_shortcutMgr->isSupported()) {
        qCDebug(lcSpeech)
            << "Push-to-Talk requested but not supported on current desktop environment; falling back to Toggle";
        mode = RecordingMode::Toggle;
    }

    if (m_recordingMode != mode) {
        m_recordingMode = mode;
        if (m_settingsManager) {
            m_settingsManager->setRecordingMode(m_recordingMode == RecordingMode::PushToTalk
                                                    ? SettingsManager::RecordingMode::PushToTalk
                                                    : SettingsManager::RecordingMode::Toggle);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kSpeechRecordingMode.toString(),
                              m_recordingMode == RecordingMode::PushToTalk ? u"PushToTalk"_s : u"Toggle"_s);
        }
        emit recordingModeChanged();
    }
}

void DictationCoordinator::setRecordingMode(int mode) {
    setRecordingMode(mode == 1 ? RecordingMode::PushToTalk : RecordingMode::Toggle);
}

QString DictationCoordinator::historyFilePath() {
    return HistoryRepository::defaultHistoryFilePath();
}

void DictationCoordinator::removeHistoryFile() {
    HistoryRepository::removeHistoryFile();
}

void DictationCoordinator::loadSettings() {
    TranscriptionBackend backend = TranscriptionBackend::Cloud;
    bool autoSwitch = true;
    RecordingMode mode = RecordingMode::Toggle;

    if (m_settingsManager) {
        const auto& config = m_settingsManager->speechConfig();
        backend = (config.backend == SettingsManager::TranscriptionBackend::WhisperCpp)
                      ? TranscriptionBackend::WhisperCpp
                      : TranscriptionBackend::Cloud;
        autoSwitch = config.autoSwitchOffline;
        mode = (config.recordingMode == SettingsManager::RecordingMode::PushToTalk) ? RecordingMode::PushToTalk
                                                                                    : RecordingMode::Toggle;
    } else {
        const QSettings settings;
        const QString backendStr = settings.value(SettingsKeys::kSpeechBackend.toString(), u"Cloud"_s).toString();
        backend = (backendStr == u"WhisperCpp"_s) ? TranscriptionBackend::WhisperCpp : TranscriptionBackend::Cloud;
        autoSwitch = settings.value(SettingsKeys::kSpeechAutoSwitchOffline.toString(), true).toBool();
        const QString modeStr = settings.value(SettingsKeys::kSpeechRecordingMode.toString(), u"Toggle"_s).toString();
        mode = (modeStr == u"PushToTalk"_s) ? RecordingMode::PushToTalk : RecordingMode::Toggle;
    }

    auto* comp = qobject_cast<CompositeSttEngine*>(m_sttEngine);
    const bool isOfflineFallbackActive = comp && comp->isAutoSwitchedToOffline();

    if (!isOfflineFallbackActive || backend == TranscriptionBackend::WhisperCpp) {
        setActiveBackend(backend, false);
    }
    setAutoSwitchOffline(autoSwitch);
    setRecordingMode(mode);
}

void DictationCoordinator::reloadSettings() {
    loadSettings();
    if (auto* parentCtx = qobject_cast<ApplicationContext*>(parent())) {
        parentCtx->reloadSettings();
    } else if (auto* comp = qobject_cast<CompositeSttEngine*>(m_sttEngine)) {
        comp->loadSettings();
    }
    updateCoordinatorHealth();
}

void DictationCoordinator::initialize() {
    if (m_initialized) {
        return;
    }
    m_initialized = true;

    const QString filePath = historyFilePath();
    if (QFile::exists(filePath)) {
        QFile::setPermissions(filePath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }

    if (m_sttEngine) {
        m_sttEngine->activate();
    }
    updateCoordinatorHealth();
    qCDebug(lcSpeech) << "DictationCoordinator: initialized and ready for dictation";
}

void DictationCoordinator::updateCoordinatorHealth() {
    if (m_shortcutMgr && m_recordingMode == RecordingMode::PushToTalk && !m_shortcutMgr->isSupported()) {
        qCDebug(lcSpeech) << "Push-to-talk not supported by portal; reverting to Toggle mode";
        m_recordingMode = RecordingMode::Toggle;
        if (m_settingsManager) {
            m_settingsManager->setRecordingMode(SettingsManager::RecordingMode::Toggle);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kSpeechRecordingMode.toString(), u"Toggle"_s);
        }
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
    const bool micReady = m_recorder ? m_recorder->hasAudioInputDevice() : true;
    const bool sttReady = m_sttEngine ? m_sttEngine->isReady() : true;
    const bool micTestActive = m_systemService ? m_systemService->microphoneTestActive() : false;
    return notProcessing && micReady && sttReady && !micTestActive;
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

bool DictationCoordinator::clientWindowFocused() const noexcept {
    return m_clientWindowFocused;
}

void DictationCoordinator::setClientWindowFocused(bool focused) {
    if (m_clientWindowFocused != focused) {
        m_clientWindowFocused = focused;
        emit clientWindowFocusedChanged();
    }
}

void DictationCoordinator::startRecording() {
    loadSettings();

    if (isBusy()) {
        qCDebug(lcSpeech) << "DictationCoordinator: startRecording ignored — already busy";
        return;
    }

    if (m_systemService && m_systemService->microphoneTestActive()) {
        m_systemService->stopMicrophoneTest();
    }

    if (m_recorder && m_recorder->recording()) {
        qCDebug(lcSpeech) << "DictationCoordinator: startRecording ignored — audio recorder is already recording";
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
        if (!m_recorder->recording()) {
            qCDebug(lcSpeech) << "DictationCoordinator: Recorder already stopped; resetting to Idle";
            setState(DictationState::Idle);
            setStatusMessage(tr("Ready"));
            return;
        }
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

    m_pendingRawText.clear();
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

void DictationCoordinator::clearHistory() {
    historyRepository()->clearHistory();
    m_lastTranscription.clear();
    emit lastTranscriptionChanged();
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
    setStatusMessage(tr("Transcribing…"));

    if (m_sttEngine) {
        m_sttEngine->transcribe(m_lastWavData);
    } else {
        qWarning() << "DictationCoordinator: No active STT engine to transcribe audio";
        setLastError(tr("Speech-to-text service is unavailable"));
        setState(DictationState::Error);
    }
}

void DictationCoordinator::onMaxDurationReached() {
    qCDebug(lcSpeech) << "DictationCoordinator: Max recording duration reached";
    emit maxDurationWarningTriggered();
    stopRecording();
}

void DictationCoordinator::onSttTranscriptionReady(const QString& text) {
    if (m_state != DictationState::Transcribing || (sender() && sender() != m_sttEngine)) {
        qCDebug(lcSpeech) << "DictationCoordinator: Ignoring STT response: not in Transcribing state or sender is not "
                             "active STT engine";
        return;
    }

    qCDebug(lcSpeech) << "DictationCoordinator: Raw STT transcription received (" << text.size() << "chars)";

    const bool isCloud = (activeBackend() == TranscriptionBackend::Cloud);
    if (isCloud && m_llmClient && m_llmClient->enabled()) {
        if (m_sttEngine && m_sttEngine->handlesSmartFormatting()) {
            qCDebug(lcSpeech)
                << "DictationCoordinator: Active STT engine handled smart formatting natively; skipping LLM pass";
            completeTranscription(text);
            return;
        }

        qCDebug(lcSpeech) << "DictationCoordinator: Passing transcription to LLM post-processing...";
        m_pendingRawText = text;
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
    m_pendingRawText.clear();
    completeTranscription(enhancedText);
}

void DictationCoordinator::onLlmError(const QString& error, const QString& fallbackRawText) {
    qWarning() << "DictationCoordinator: LLM enhancement failed:" << error << "-> Falling back to raw transcription.";
    emit llmFallbackWarningTriggered(error);

    const QString rawToUse = !fallbackRawText.isEmpty() ? fallbackRawText : m_pendingRawText;
    m_pendingRawText.clear();
    if (!rawToUse.isEmpty()) {
        completeTranscription(rawToUse);
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

    if (!text.trimmed().isEmpty()) {
        historyRepository()->appendRecord(text);
    }

    emit transcriptionFinished(text);
}

void DictationCoordinator::finishTranscriptionAndInject(const QString& text) {
    if (text.isEmpty()) {
        return;
    }

    if (m_injector && !m_clientWindowFocused) {
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
