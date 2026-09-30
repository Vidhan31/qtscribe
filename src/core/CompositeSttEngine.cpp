#include "CompositeSttEngine.h"

#include "AudioRecorder.h"
#include "LoggingCategories.h"

#include "SystemHealthMonitor.h"
#include "WhisperModelManager.h"
#include "WhisperSttClient.h"

#include <QDebug>
#include <QSettings>

using namespace Qt::StringLiterals;

CompositeSttEngine::CompositeSttEngine(QObject* parent)
    : AbstractSttClient(parent) {
    QSettings settings;
    const QString backendStr = settings.value(u"Speech/Backend"_s, u"WhisperCpp"_s).toString();
    if (backendStr == u"Cloud"_s || backendStr == u"Groq"_s) {
        m_activeBackend = TranscriptionBackend::Cloud;
    } else {
        m_activeBackend = TranscriptionBackend::WhisperCpp;
    }

    m_autoSwitchOffline = settings.value(u"Speech/AutoSwitchOffline"_s, true).toBool();
}

void CompositeSttEngine::setCloudClient(AbstractSttClient* client) {
    if (m_cloudClient == client) {
        return;
    }

    if (m_cloudClient) {
        disconnect(m_cloudClient, &AbstractSttClient::transcriptionReady, this,
                   &CompositeSttEngine::onActiveClientTranscriptionReady);
        disconnect(m_cloudClient, &AbstractSttClient::errorOccurred, this, &CompositeSttEngine::onActiveClientError);
        disconnect(m_cloudClient, &AbstractSttClient::busyChanged, this, &AbstractSttClient::busyChanged);
        disconnect(m_cloudClient, &AbstractSttClient::readyChanged, this, &AbstractSttClient::readyChanged);
        disconnect(m_cloudClient, &AbstractSttClient::apiKeySetChanged, this, &AbstractSttClient::apiKeySetChanged);
        disconnect(m_cloudClient, &AbstractSttClient::isApiKeyInvalidChanged, this,
                   &AbstractSttClient::isApiKeyInvalidChanged);
        disconnect(m_cloudClient, &AbstractSttClient::isRateLimitedChanged, this,
                   &AbstractSttClient::isRateLimitedChanged);
        disconnect(m_cloudClient, &AbstractSttClient::retrySecondsRemainingChanged, this,
                   &AbstractSttClient::retrySecondsRemainingChanged);
        disconnect(m_cloudClient, &AbstractSttClient::errorCategoryChanged, this,
                   &AbstractSttClient::errorCategoryChanged);
    }

    m_cloudClient = client;

    if (m_cloudClient) {
        connect(m_cloudClient, &AbstractSttClient::transcriptionReady, this,
                &CompositeSttEngine::onActiveClientTranscriptionReady);
        connect(m_cloudClient, &AbstractSttClient::errorOccurred, this, &CompositeSttEngine::onActiveClientError);
        connect(m_cloudClient, &AbstractSttClient::busyChanged, this, &AbstractSttClient::busyChanged);
        connect(m_cloudClient, &AbstractSttClient::readyChanged, this, &AbstractSttClient::readyChanged);
        connect(m_cloudClient, &AbstractSttClient::apiKeySetChanged, this, &AbstractSttClient::apiKeySetChanged);
        connect(m_cloudClient, &AbstractSttClient::isApiKeyInvalidChanged, this,
                &AbstractSttClient::isApiKeyInvalidChanged);
        connect(m_cloudClient, &AbstractSttClient::isRateLimitedChanged, this,
                &AbstractSttClient::isRateLimitedChanged);
        connect(m_cloudClient, &AbstractSttClient::retrySecondsRemainingChanged, this,
                &AbstractSttClient::retrySecondsRemainingChanged);
        connect(m_cloudClient, &AbstractSttClient::errorCategoryChanged, this,
                &AbstractSttClient::errorCategoryChanged);

        if (m_active && m_activeBackend == TranscriptionBackend::Cloud) {
            m_cloudClient->activate();
        }
    }

    emit readyChanged();
}

AbstractSttClient* CompositeSttEngine::cloudClient() const noexcept {
    return m_cloudClient;
}

void CompositeSttEngine::setWhisperClient(AbstractSttClient* client) {
    if (m_whisperClient == client) {
        return;
    }

    if (m_whisperClient) {
        disconnect(m_whisperClient, &AbstractSttClient::transcriptionReady, this,
                   &CompositeSttEngine::onActiveClientTranscriptionReady);
        disconnect(m_whisperClient, &AbstractSttClient::errorOccurred, this, &CompositeSttEngine::onActiveClientError);
        disconnect(m_whisperClient, &AbstractSttClient::busyChanged, this, &AbstractSttClient::busyChanged);
        disconnect(m_whisperClient, &AbstractSttClient::readyChanged, this, &AbstractSttClient::readyChanged);
        if (auto* whisper = qobject_cast<WhisperSttClient*>(m_whisperClient.data())) {
            disconnect(whisper, &WhisperSttClient::modelLoadFailed, this,
                       &CompositeSttEngine::onWhisperModelLoadFailed);
        }
    }

    m_whisperClient = client;

    if (m_whisperClient) {
        connect(m_whisperClient, &AbstractSttClient::transcriptionReady, this,
                &CompositeSttEngine::onActiveClientTranscriptionReady);
        connect(m_whisperClient, &AbstractSttClient::errorOccurred, this, &CompositeSttEngine::onActiveClientError);
        connect(m_whisperClient, &AbstractSttClient::busyChanged, this, &AbstractSttClient::busyChanged);
        connect(m_whisperClient, &AbstractSttClient::readyChanged, this, &AbstractSttClient::readyChanged);
        if (auto* whisper = qobject_cast<WhisperSttClient*>(m_whisperClient.data())) {
            connect(whisper, &WhisperSttClient::modelLoadFailed, this, &CompositeSttEngine::onWhisperModelLoadFailed);
        }

        if (m_active && m_activeBackend == TranscriptionBackend::WhisperCpp) {
            m_whisperClient->activate();
        }
    }

    emit readyChanged();
}

AbstractSttClient* CompositeSttEngine::whisperClient() const noexcept {
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
    emit busyChanged();
}

AudioRecorder* CompositeSttEngine::audioRecorder() const noexcept {
    return m_recorder;
}

CompositeSttEngine::TranscriptionBackend CompositeSttEngine::activeBackend() const {
    return m_activeBackend;
}

void CompositeSttEngine::setActiveBackend(TranscriptionBackend backend, bool persistSettings) {
    if (m_activeBackend == backend) {
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
        QSettings settings;
        settings.setValue(u"Speech/TranscriptionBackend"_s, static_cast<int>(backend));
        settings.setValue(u"Speech/Backend"_s,
                          m_activeBackend == TranscriptionBackend::WhisperCpp ? u"WhisperCpp"_s : u"Cloud"_s);
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
    emit apiKeySetChanged();
    emit isApiKeyInvalidChanged();
    emit isRateLimitedChanged();
    emit retrySecondsRemainingChanged();
    emit errorCategoryChanged();
}

bool CompositeSttEngine::autoSwitchOffline() const {
    return m_autoSwitchOffline;
}

void CompositeSttEngine::setAutoSwitchOffline(bool enable) {
    if (m_autoSwitchOffline == enable) {
        return;
    }

    m_autoSwitchOffline = enable;
    QSettings settings;
    settings.setValue(u"Speech/AutoSwitchOffline"_s, enable);
    emit autoSwitchOfflineChanged();

    if (!m_autoSwitchOffline && m_autoSwitchedToOffline && !isBusy()) {
        restoreOnlineBackend();
    }
}

bool CompositeSttEngine::isAutoSwitchedToOffline() const {
    return m_autoSwitchedToOffline;
}

AbstractSttClient* CompositeSttEngine::activeSttClient() const {
    if (m_activeBackend == TranscriptionBackend::Cloud) {
        return m_cloudClient;
    }
    return m_whisperClient;
}

void CompositeSttEngine::transcribe(const QByteArray& wavData) {
    m_lastWavData = wavData;
    m_isBusy = true;
    emit busyChanged();

    auto* client = activeSttClient();
    if (client) {
        client->transcribe(wavData);
    } else {
        m_isBusy = false;
        emit busyChanged();
        const QString errMsg = tr("Speech-to-text service is unavailable");
        setLastError(errMsg);
        emit errorOccurred(errMsg);
    }
}

void CompositeSttEngine::cancel() {
    m_lastWavData.clear();
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
    if (m_lastWavData.isEmpty()) {
        return;
    }
    transcribe(m_lastWavData);
}

bool CompositeSttEngine::isReady() const {
    auto* client = activeSttClient();
    return client ? client->isReady() : false;
}

bool CompositeSttEngine::isBusy() const {
    if (m_isBusy) {
        return true;
    }
    if (m_recorder && m_recorder->recording()) {
        return true;
    }
    auto* client = activeSttClient();
    return client ? client->isBusy() : false;
}

bool CompositeSttEngine::handlesSmartFormatting() const {
    auto* client = activeSttClient();
    return client ? client->handlesSmartFormatting() : false;
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

bool CompositeSttEngine::isApiKeySet() const {
    auto* client = activeSttClient();
    return client ? client->isApiKeySet() : false;
}

bool CompositeSttEngine::isApiKeyInvalid() const {
    auto* client = activeSttClient();
    return client ? client->isApiKeyInvalid() : false;
}

bool CompositeSttEngine::isRateLimited() const {
    auto* client = activeSttClient();
    return client ? client->isRateLimited() : false;
}

int CompositeSttEngine::retrySecondsRemaining() const {
    auto* client = activeSttClient();
    return client ? client->retrySecondsRemaining() : 0;
}

AbstractSttClient::ErrorCategory CompositeSttEngine::errorCategory() const {
    auto* client = activeSttClient();
    return client ? client->errorCategory() : ErrorCategory::None;
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
    if (!m_autoSwitchOffline) {
        m_pendingOfflineSwitch = false;
        return;
    }

    if (isBusy()) {
        return;
    }

    const bool isOnline = m_healthMonitor ? m_healthMonitor->isNetworkOnline() : true;

    if (m_pendingOfflineSwitch) {
        m_pendingOfflineSwitch = false;
        if (!isOnline && m_activeBackend == TranscriptionBackend::Cloud) {
            applyOfflineSwitch();
            return;
        }
    }

    if (m_autoSwitchedToOffline && isOnline) {
        restoreOnlineBackend();
    }
}

void CompositeSttEngine::applyOfflineSwitch() {
    if (!isOfflineModelAvailable()) {
        emit offlineModelRequired();
        return;
    }

    qCInfo(lcSpeech) << "CompositeSttEngine: Offline model verified. Switching active backend to WhisperCpp";
    setActiveBackend(TranscriptionBackend::WhisperCpp, /*persistSettings=*/false);
    m_autoSwitchedToOffline = true;
    emit autoSwitchedToOfflineChanged();
    emit failoverTriggered();
}

void CompositeSttEngine::restoreOnlineBackend() {
    qCInfo(lcSpeech) << "CompositeSttEngine: Restoring active backend back to Cloud";
    setActiveBackend(TranscriptionBackend::Cloud, /*persistSettings=*/false);
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

    if (auto* whisper = qobject_cast<WhisperSttClient*>(m_whisperClient.data())) {
        return whisper->isModelInstalled();
    }

    if (m_whisperClient) {
        return true;
    }

    return false;
}

void CompositeSttEngine::setLastError(const QString& error) {
    if (m_lastError != error) {
        m_lastError = error;
        emit lastErrorChanged();
    }
}
