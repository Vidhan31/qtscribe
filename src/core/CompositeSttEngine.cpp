#include "CompositeSttEngine.h"

#include "AudioRecorder.h"
#include "LoggingCategories.h"

#include "common/SettingsKeys.h"
#include "AbstractCloudSttClient.h"
#include "AbstractOfflineSttClient.h"
#include "SettingsManager.h"
#include "SystemHealthMonitor.h"
#include "WhisperModelManager.h"

#include <QDebug>
#include <QSettings>

using namespace Qt::StringLiterals;

CompositeSttEngine::CompositeSttEngine(QObject* parent)
    : AbstractSttClient(parent) {
    if (auto* inst = SettingsManager::instance()) {
        setSettingsManager(inst);
    } else {
        loadSettings();
    }
}

void CompositeSttEngine::setSettingsManager(SettingsManager* settings) {
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
                    const auto targetBackend = (config.backend == SettingsManager::TranscriptionBackend::WhisperCpp)
                                                   ? TranscriptionBackend::WhisperCpp
                                                   : TranscriptionBackend::Cloud;
                    if (!m_autoSwitchedToOffline || targetBackend == TranscriptionBackend::WhisperCpp) {
                        setActiveBackend(targetBackend, false);
                    }

                    if (m_autoSwitchOffline != config.autoSwitchOffline) {
                        m_autoSwitchOffline = config.autoSwitchOffline;
                        emit autoSwitchOfflineChanged();
                        if (!m_autoSwitchOffline && m_autoSwitchedToOffline && !isBusy()) {
                            restoreOnlineBackend();
                        }
                    }
                });
    }
    loadSettings();
}

SettingsManager* CompositeSttEngine::settingsManager() const noexcept {
    return m_settingsManager;
}

void CompositeSttEngine::loadSettings() {
    int backendInt = 0;
    bool autoSwitch = true;

    if (m_settingsManager) {
        const auto config = m_settingsManager->speechConfig();
        backendInt = (config.backend == SettingsManager::TranscriptionBackend::WhisperCpp) ? 1 : 0;
        autoSwitch = config.autoSwitchOffline;
    } else {
        const QSettings settings;
        backendInt = settings.value(SettingsKeys::kSpeechTranscriptionBackend.toString(), 0).toInt();
        const QString backendStr = settings.value(SettingsKeys::kSpeechBackend.toString(), QString()).toString();
        if (!backendStr.isEmpty()) {
            backendInt = (backendStr == u"WhisperCpp"_s) ? 1 : 0;
        }
        autoSwitch = settings.value(SettingsKeys::kSpeechAutoSwitchOffline.toString(), true).toBool();
    }

    const auto targetBackend = (backendInt == 1) ? TranscriptionBackend::WhisperCpp : TranscriptionBackend::Cloud;

    if (!m_autoSwitchedToOffline || targetBackend == TranscriptionBackend::WhisperCpp) {
        if (m_activeBackend != targetBackend) {
            m_activeBackend = targetBackend;
            emit activeBackendChanged();
        }
    }

    if (m_autoSwitchOffline != autoSwitch) {
        m_autoSwitchOffline = autoSwitch;
        emit autoSwitchOfflineChanged();
        if (!m_autoSwitchOffline && m_autoSwitchedToOffline && !isBusy()) {
            restoreOnlineBackend();
        }
    }

    if (m_whisperClient) {
        m_whisperClient->loadSettings();
    }
}

void CompositeSttEngine::setCloudClient(AbstractCloudSttClient* client) {
    if (m_cloudClient == client) {
        return;
    }

    if (m_cloudClient) {
        disconnect(m_cloudClient, &AbstractSttClient::transcriptionReady, this,
                   &CompositeSttEngine::onActiveClientTranscriptionReady);
        disconnect(m_cloudClient, &AbstractSttClient::errorOccurred, this, &CompositeSttEngine::onActiveClientError);
        disconnect(m_cloudClient, &AbstractSttClient::busyChanged, this, &AbstractSttClient::busyChanged);
        disconnect(m_cloudClient, &AbstractSttClient::readyChanged, this, &AbstractSttClient::readyChanged);
    }

    m_cloudClient = client;

    if (m_cloudClient) {
        connect(m_cloudClient, &AbstractSttClient::transcriptionReady, this,
                &CompositeSttEngine::onActiveClientTranscriptionReady);
        connect(m_cloudClient, &AbstractSttClient::errorOccurred, this, &CompositeSttEngine::onActiveClientError);
        connect(m_cloudClient, &AbstractSttClient::busyChanged, this, &AbstractSttClient::busyChanged);
        connect(m_cloudClient, &AbstractSttClient::readyChanged, this, &AbstractSttClient::readyChanged);

        if (m_active && m_activeBackend == TranscriptionBackend::Cloud) {
            m_cloudClient->activate();
        }
    }

    emit cloudClientChanged(m_cloudClient);
    emit readyChanged();
}

AbstractCloudSttClient* CompositeSttEngine::cloudClient() const noexcept {
    return m_cloudClient;
}

void CompositeSttEngine::setWhisperClient(AbstractOfflineSttClient* client) {
    if (m_whisperClient == client) {
        return;
    }

    if (m_whisperClient) {
        disconnect(m_whisperClient, &AbstractSttClient::transcriptionReady, this,
                   &CompositeSttEngine::onActiveClientTranscriptionReady);
        disconnect(m_whisperClient, &AbstractSttClient::errorOccurred, this, &CompositeSttEngine::onActiveClientError);
        disconnect(m_whisperClient, &AbstractSttClient::busyChanged, this, &AbstractSttClient::busyChanged);
        disconnect(m_whisperClient, &AbstractSttClient::readyChanged, this, &AbstractSttClient::readyChanged);
        disconnect(m_whisperClient, &AbstractOfflineSttClient::modelLoadFailed, this,
                   &CompositeSttEngine::onWhisperModelLoadFailed);
        disconnect(m_whisperClient, &AbstractOfflineSttClient::modelStatusChanged, this,
                   &CompositeSttEngine::modelStatusChanged);
        disconnect(m_whisperClient, &AbstractOfflineSttClient::computeDeviceChanged, this,
                   &CompositeSttEngine::computeDeviceChanged);
    }

    m_whisperClient = client;

    if (m_whisperClient) {
        connect(m_whisperClient, &AbstractSttClient::transcriptionReady, this,
                &CompositeSttEngine::onActiveClientTranscriptionReady);
        connect(m_whisperClient, &AbstractSttClient::errorOccurred, this, &CompositeSttEngine::onActiveClientError);
        connect(m_whisperClient, &AbstractSttClient::busyChanged, this, &AbstractSttClient::busyChanged);
        connect(m_whisperClient, &AbstractSttClient::readyChanged, this, &AbstractSttClient::readyChanged);
        connect(m_whisperClient, &AbstractOfflineSttClient::modelLoadFailed, this,
                &CompositeSttEngine::onWhisperModelLoadFailed);
        connect(m_whisperClient, &AbstractOfflineSttClient::modelStatusChanged, this,
                &CompositeSttEngine::modelStatusChanged);
        connect(m_whisperClient, &AbstractOfflineSttClient::computeDeviceChanged, this,
                &CompositeSttEngine::computeDeviceChanged);

        if (m_active && m_activeBackend == TranscriptionBackend::WhisperCpp) {
            m_whisperClient->activate();
        }
    }

    emit readyChanged();
}

AbstractOfflineSttClient* CompositeSttEngine::whisperClient() const noexcept {
    return m_whisperClient;
}

void CompositeSttEngine::setModelManager(WhisperModelManager* manager) {
    m_modelManager = manager;
}

WhisperModelManager* CompositeSttEngine::modelManager() const noexcept {
    return m_modelManager;
}

void CompositeSttEngine::setHealthMonitor(SystemHealthMonitor* monitor) {
    if (m_healthMonitor == monitor) {
        return;
    }

    if (m_healthMonitor) {
        disconnect(m_healthMonitor, &SystemHealthMonitor::networkOnlineChanged, this,
                   &CompositeSttEngine::onNetworkOnlineChanged);
    }

    m_healthMonitor = monitor;

    if (m_healthMonitor) {
        connect(m_healthMonitor, &SystemHealthMonitor::networkOnlineChanged, this,
                &CompositeSttEngine::onNetworkOnlineChanged);
    }
}

SystemHealthMonitor* CompositeSttEngine::healthMonitor() const noexcept {
    return m_healthMonitor;
}

void CompositeSttEngine::setAudioRecorder(AudioRecorder* recorder) {
    if (m_recorder == recorder) {
        return;
    }

    if (m_recorder) {
        disconnect(m_recorder, &AudioRecorder::recordingChanged, this,
                   &CompositeSttEngine::checkPendingOfflineOrOnlineSwitch);
        disconnect(m_recorder, &AudioRecorder::recordingChanged, this, &AbstractSttClient::busyChanged);
    }

    m_recorder = recorder;

    if (m_recorder) {
        connect(m_recorder, &AudioRecorder::recordingChanged, this,
                &CompositeSttEngine::checkPendingOfflineOrOnlineSwitch);
        connect(m_recorder, &AudioRecorder::recordingChanged, this, &AbstractSttClient::busyChanged);
    }
}

AudioRecorder* CompositeSttEngine::audioRecorder() const noexcept {
    return m_recorder;
}

CompositeSttEngine::TranscriptionBackend CompositeSttEngine::activeBackend() const {
    return m_activeBackend;
}

void CompositeSttEngine::setActiveBackend(TranscriptionBackend backend, bool persistSettings) {
    if (m_activeBackend == backend) {
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
            if (m_autoSwitchedToOffline || m_pendingOfflineSwitch) {
                m_autoSwitchedToOffline = false;
                m_pendingOfflineSwitch = false;
                emit autoSwitchedToOfflineChanged();
            }
        }

        if (m_active && m_activeBackend == TranscriptionBackend::WhisperCpp) {
            if (m_whisperClient) {
                if (!m_whisperClient->isModelLoaded() && m_whisperClient->isModelInstalled() &&
                    !m_whisperClient->isExplicitlyUnloaded()) {
                    m_whisperClient->activate();
                }
            }
        }
        return;
    }

    if (isBusy()) {
        cancel();
    }

    if (auto* oldClient = activeSttClient()) {
        oldClient->deactivate();
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
        m_autoSwitchedToOffline = false;
        m_pendingOfflineSwitch = false;
        emit autoSwitchedToOfflineChanged();
    }

    if (m_active) {
        if (auto* newClient = activeSttClient()) {
            newClient->activate();
        }
    }

    emit activeBackendChanged();
    emit readyChanged();
    emit busyChanged();
}

bool CompositeSttEngine::autoSwitchOffline() const {
    return m_autoSwitchOffline;
}

void CompositeSttEngine::setAutoSwitchOffline(bool enable) {
    if (m_autoSwitchOffline == enable) {
        return;
    }

    m_autoSwitchOffline = enable;
    if (m_settingsManager) {
        m_settingsManager->setAutoSwitchOffline(enable);
    } else {
        QSettings settings;
        settings.setValue(SettingsKeys::kSpeechAutoSwitchOffline.toString(), enable);
    }
    emit autoSwitchOfflineChanged();

    if (!m_autoSwitchOffline) {
        m_pendingOfflineSwitch = false;
        if (m_autoSwitchedToOffline && !isBusy()) {
            restoreOnlineBackend();
        }
    }
}

bool CompositeSttEngine::isAutoSwitchedToOffline() const {
    return m_autoSwitchedToOffline;
}

AbstractSttClient* CompositeSttEngine::activeSttClient() const {
    if (m_activeBackend == TranscriptionBackend::WhisperCpp) {
        return m_whisperClient;
    }
    return m_cloudClient;
}

void CompositeSttEngine::transcribe(const QByteArray& wavData) {
    m_lastWavData = wavData;
    auto* client = activeSttClient();
    if (!client) {
        setLastError(u"No active STT client available"_s);
        emit errorOccurred(lastError());
        return;
    }

    m_isBusy = true;
    emit busyChanged();

    client->transcribe(wavData);
}

void CompositeSttEngine::cancel() {
    if (auto* client = activeSttClient()) {
        client->cancel();
    }
    if (m_isBusy) {
        m_isBusy = false;
        emit busyChanged();
    }
    checkPendingOfflineOrOnlineSwitch();
}

void CompositeSttEngine::retryLast() {
    if (!m_lastWavData.isEmpty()) {
        transcribe(m_lastWavData);
    } else if (auto* client = activeSttClient()) {
        client->retryLast();
    }
}

bool CompositeSttEngine::isReady() const {
    auto* client = activeSttClient();
    return client && client->isReady();
}

bool CompositeSttEngine::isBusy() const {
    if (m_recorder && m_recorder->recording()) {
        return true;
    }
    if (m_isBusy) {
        return true;
    }
    auto* client = activeSttClient();
    return client && client->isBusy();
}

bool CompositeSttEngine::handlesSmartFormatting() const {
    auto* client = activeSttClient();
    return client && client->handlesSmartFormatting();
}

QString CompositeSttEngine::lastError() const {
    if (!m_lastError.isEmpty()) {
        return m_lastError;
    }
    auto* client = activeSttClient();
    return client ? client->lastError() : QString();
}

void CompositeSttEngine::activate() {
    m_active = true;
    if (auto* client = activeSttClient()) {
        client->activate();
    }
}

void CompositeSttEngine::deactivate() {
    m_active = false;
    if (auto* client = activeSttClient()) {
        client->deactivate();
    }
}

void CompositeSttEngine::onActiveClientTranscriptionReady(const QString& text) {
    if (sender() && sender() != activeSttClient()) {
        return;
    }

    m_isBusy = false;
    emit busyChanged();
    emit transcriptionReady(text);
    checkPendingOfflineOrOnlineSwitch();
}

void CompositeSttEngine::onActiveClientError(const QString& error) {
    if (sender() && sender() != activeSttClient()) {
        return;
    }

    m_isBusy = false;
    emit busyChanged();
    setLastError(error);
    emit errorOccurred(error);
    checkPendingOfflineOrOnlineSwitch();
}

void CompositeSttEngine::onWhisperModelLoadFailed(const QString& modelPath, const QString& error) {
    emit modelLoadFailed(modelPath, error);
}

void CompositeSttEngine::onNetworkOnlineChanged(bool online) {
    qCInfo(lcSpeech) << "CompositeSttEngine: Network reachability changed. Online:" << online;
    if (!m_autoSwitchOffline) {
        return;
    }

    if (!online) {
        if (m_activeBackend != TranscriptionBackend::Cloud) {
            return;
        }

        if (isBusy()) {
            m_pendingOfflineSwitch = true;
            emit networkDisconnectedDuringSession();
        } else {
            applyOfflineSwitch();
        }
    } else {
        if (!m_autoSwitchedToOffline) {
            return;
        }

        if (isBusy()) {
            qCInfo(lcSpeech) << "CompositeSttEngine: Network restored while busy; will restore backend when idle";
        } else {
            restoreOnlineBackend();
        }
    }
}

void CompositeSttEngine::checkPendingOfflineOrOnlineSwitch() {
    if (isBusy()) {
        return;
    }

    if (m_pendingOfflineSwitch) {
        m_pendingOfflineSwitch = false;
        const bool networkOnline = m_healthMonitor ? m_healthMonitor->isNetworkOnline() : true;
        if (m_autoSwitchOffline && !networkOnline && m_activeBackend == TranscriptionBackend::Cloud) {
            applyOfflineSwitch();
            return;
        }
    }

    if (m_autoSwitchedToOffline && m_healthMonitor && m_healthMonitor->isNetworkOnline()) {
        restoreOnlineBackend();
    }
}

void CompositeSttEngine::applyOfflineSwitch() {
    if (!isOfflineModelAvailable()) {
        qCWarning(lcSpeech)
            << "CompositeSttEngine: Automatic switch to offline requested, but no Whisper model is available";
        emit offlineModelRequired();
        return;
    }

    qCInfo(lcSpeech) << "CompositeSttEngine: Offline model verified. Switching active backend to WhisperCpp";
    m_autoSwitchedToOffline = true;
    emit autoSwitchedToOfflineChanged();
    setActiveBackend(TranscriptionBackend::WhisperCpp, false);
    emit failoverTriggered();
}

void CompositeSttEngine::restoreOnlineBackend() {
    qCInfo(lcSpeech) << "CompositeSttEngine: Restoring active backend back to Cloud";
    setActiveBackend(TranscriptionBackend::Cloud, false);
    m_autoSwitchedToOffline = false;
    emit autoSwitchedToOfflineChanged();
    emit onlineRestored();
}

bool CompositeSttEngine::isOfflineModelAvailable() const {
    if (m_modelManager) {
        if (!m_modelManager->isSelectedModelInstalled() && m_modelManager->hasAnyModelInstalled()) {
            const QString fallbackId = m_modelManager->firstInstalledModelId();
            qCInfo(lcSpeech) << "CompositeSttEngine: Selected model not installed. Auto-selecting installed model:"
                             << fallbackId;
            m_modelManager->setSelectedModelId(fallbackId);
        }
        return m_modelManager->isSelectedModelInstalled();
    }

    if (m_whisperClient) {
        return m_whisperClient->isModelInstalled();
    }

    return false;
}

void CompositeSttEngine::setLastError(const QString& error) {
    if (m_lastError != error) {
        m_lastError = error;
        emit lastErrorChanged();
    }
}

void CompositeSttEngine::loadModel(const QString& customPath) {
    if (m_whisperClient) {
        m_whisperClient->loadModel(customPath);
    }
}

void CompositeSttEngine::unloadModel() {
    if (m_whisperClient) {
        m_whisperClient->unloadModel();
    }
}

void CompositeSttEngine::checkModelStatus() {
    if (m_whisperClient) {
        m_whisperClient->checkModelStatus();
    }
}
