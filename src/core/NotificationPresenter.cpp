#include "NotificationPresenter.h"

#include "AudioRecorder.h"
#include "DictationCoordinator.h"
#include "LoggingCategories.h"

#include "CompositeSttEngine.h"
#include "LinuxNotificationService.h"

#include <QDebug>

using namespace Qt::StringLiterals;

NotificationPresenter::NotificationPresenter(QObject* parent)
    : QObject(parent) { }

void NotificationPresenter::setNotificationService(LinuxNotificationService* service) {
    if (m_notificationService == service) {
        return;
    }

    if (m_notificationService) {
        disconnect(m_notificationService, &LinuxNotificationService::actionInvoked, this,
                   &NotificationPresenter::onNotificationActionInvoked);
    }

    m_notificationService = service;

    if (m_notificationService) {
        connect(m_notificationService, &LinuxNotificationService::actionInvoked, this,
                &NotificationPresenter::onNotificationActionInvoked);
    }
}

LinuxNotificationService* NotificationPresenter::notificationService() const noexcept {
    return m_notificationService;
}

void NotificationPresenter::setDictationCoordinator(DictationCoordinator* coordinator) {
    if (m_dictationCoordinator == coordinator) {
        return;
    }

    if (m_dictationCoordinator) {
        disconnect(m_dictationCoordinator, &DictationCoordinator::errorOccurred, this,
                   &NotificationPresenter::onCoordinatorErrorOccurred);
        disconnect(m_dictationCoordinator, &DictationCoordinator::transcriptionFinished, this,
                   &NotificationPresenter::onCoordinatorTranscriptionFinished);
        disconnect(m_dictationCoordinator, &DictationCoordinator::maxDurationWarningTriggered, this,
                   &NotificationPresenter::onCoordinatorMaxDurationWarning);
        disconnect(m_dictationCoordinator, &DictationCoordinator::llmFallbackWarningTriggered, this,
                   &NotificationPresenter::onCoordinatorLlmFallbackWarning);
    }

    m_dictationCoordinator = coordinator;

    if (m_dictationCoordinator) {
        connect(m_dictationCoordinator, &DictationCoordinator::errorOccurred, this,
                &NotificationPresenter::onCoordinatorErrorOccurred);
        connect(m_dictationCoordinator, &DictationCoordinator::transcriptionFinished, this,
                &NotificationPresenter::onCoordinatorTranscriptionFinished);
        connect(m_dictationCoordinator, &DictationCoordinator::maxDurationWarningTriggered, this,
                &NotificationPresenter::onCoordinatorMaxDurationWarning);
        connect(m_dictationCoordinator, &DictationCoordinator::llmFallbackWarningTriggered, this,
                &NotificationPresenter::onCoordinatorLlmFallbackWarning);
    }
}

DictationCoordinator* NotificationPresenter::dictationCoordinator() const noexcept {
    return m_dictationCoordinator;
}

void NotificationPresenter::setAudioRecorder(AudioRecorder* recorder) {
    if (m_audioRecorder == recorder) {
        return;
    }

    if (m_audioRecorder) {
        disconnect(m_audioRecorder, &AudioRecorder::audioInputDeviceDisconnected, this,
                   &NotificationPresenter::onAudioDeviceDisconnected);
        disconnect(m_audioRecorder, &AudioRecorder::hasAudioInputDeviceChanged, this,
                   &NotificationPresenter::onAudioDeviceAvailabilityChanged);
    }

    m_audioRecorder = recorder;

    if (m_audioRecorder) {
        connect(m_audioRecorder, &AudioRecorder::audioInputDeviceDisconnected, this,
                &NotificationPresenter::onAudioDeviceDisconnected);
        connect(m_audioRecorder, &AudioRecorder::hasAudioInputDeviceChanged, this,
                &NotificationPresenter::onAudioDeviceAvailabilityChanged);
    }
}

AudioRecorder* NotificationPresenter::audioRecorder() const noexcept {
    return m_audioRecorder;
}

void NotificationPresenter::setCompositeSttEngine(CompositeSttEngine* engine) {
    if (m_sttEngine == engine) {
        return;
    }

    if (m_sttEngine) {
        disconnect(m_sttEngine, &CompositeSttEngine::failoverTriggered, this,
                   &NotificationPresenter::onFailoverTriggered);
        disconnect(m_sttEngine, &CompositeSttEngine::onlineRestored, this, &NotificationPresenter::onOnlineRestored);
        disconnect(m_sttEngine, &CompositeSttEngine::offlineModelRequired, this,
                   &NotificationPresenter::onOfflineModelRequired);
        disconnect(m_sttEngine, &CompositeSttEngine::networkDisconnectedDuringSession, this,
                   &NotificationPresenter::onNetworkDisconnectedDuringSession);
        disconnect(m_sttEngine, &CompositeSttEngine::modelLoadFailed, this, &NotificationPresenter::onModelLoadFailed);
    }

    m_sttEngine = engine;

    if (m_sttEngine) {
        connect(m_sttEngine, &CompositeSttEngine::failoverTriggered, this, &NotificationPresenter::onFailoverTriggered);
        connect(m_sttEngine, &CompositeSttEngine::onlineRestored, this, &NotificationPresenter::onOnlineRestored);
        connect(m_sttEngine, &CompositeSttEngine::offlineModelRequired, this,
                &NotificationPresenter::onOfflineModelRequired);
        connect(m_sttEngine, &CompositeSttEngine::networkDisconnectedDuringSession, this,
                &NotificationPresenter::onNetworkDisconnectedDuringSession);
        connect(m_sttEngine, &CompositeSttEngine::modelLoadFailed, this, &NotificationPresenter::onModelLoadFailed);
    }
}

CompositeSttEngine* NotificationPresenter::compositeSttEngine() const noexcept {
    return m_sttEngine;
}

void NotificationPresenter::onNotificationActionInvoked(uint id, const QString& actionKey) {
    Q_UNUSED(id);
    qCDebug(lcSpeech) << "NotificationPresenter: action invoked:" << actionKey;
    if (actionKey == u"default"_s || actionKey == u"open"_s) {
        emit requestShowWindow();
    } else if (actionKey == u"settings"_s || actionKey == u"audio"_s || actionKey == u"system"_s) {
        emit requestNavigate(u"system"_s);
    } else if (actionKey == u"cloud"_s) {
        emit requestNavigate(u"cloud"_s);
    } else if (actionKey == u"offline"_s) {
        emit requestNavigate(u"offline"_s);
    } else if (actionKey == u"quit"_s) {
        emit requestQuitApp();
    }
}

void NotificationPresenter::onCoordinatorErrorOccurred(const QString& error) {
    sendCategoryNotification(u"transcription"_s, tr("Transcription Failed"), error,
                             static_cast<int>(LinuxNotificationService::Urgency::Critical), u"dialog-error"_s,
                             {u"default"_s, tr("Open QtScribe")});
}

void NotificationPresenter::onCoordinatorTranscriptionFinished(const QString& text) {
    Q_UNUSED(text);
    if (m_notificationService) {
        m_notificationService->clearCategory(u"transcription"_s);
    }
}

void NotificationPresenter::onCoordinatorMaxDurationWarning() {
    sendCategoryNotification(
        u"recording_limit"_s, tr("Recording Limit Reached"),
        tr("Maximum recording duration (5m) reached. Dictation stopped and audio sent for transcription."),
        static_cast<int>(LinuxNotificationService::Urgency::Normal), u"dialog-information"_s,
        {u"default"_s, tr("Open QtScribe")});
}

void NotificationPresenter::onCoordinatorLlmFallbackWarning(const QString& warning) {
    sendCategoryNotification(u"llm_fallback"_s, tr("AI Formatting Unavailable"),
                             tr("Enhancement failed (%1). Injected raw speech transcript instead.").arg(warning),
                             static_cast<int>(LinuxNotificationService::Urgency::Low), u"dialog-warning"_s,
                             {u"default"_s, tr("Open QtScribe")});
}

void NotificationPresenter::onAudioDeviceDisconnected(const QString& deviceName, const QString& fallbackDeviceName) {
    if (fallbackDeviceName.isEmpty()) {
        return;
    }

    sendCategoryNotification(u"microphone"_s, tr("Microphone Disconnected"),
                             tr("'%1' was disconnected. Switched to '%2'.").arg(deviceName, fallbackDeviceName),
                             static_cast<int>(LinuxNotificationService::Urgency::Normal), u"audio-input-microphone"_s,
                             {u"settings"_s, tr("Audio Settings")});
}

void NotificationPresenter::onAudioDeviceAvailabilityChanged() {
    if (!m_audioRecorder) {
        return;
    }

    if (!m_audioRecorder->hasAudioInputDevice()) {
        sendCategoryNotification(u"microphone"_s, tr("No Microphone Detected"),
                                 tr("Microphone disconnected. Please reconnect your microphone to dictate."),
                                 static_cast<int>(LinuxNotificationService::Urgency::Normal),
                                 u"audio-input-microphone"_s, {u"settings"_s, tr("Audio Settings")});
    } else {
        if (m_notificationService) {
            m_notificationService->clearCategory(u"microphone"_s);
        }
    }
}

void NotificationPresenter::onFailoverTriggered() {
    sendNotification(tr("QtScribe: Switched to Offline Dictation"),
                     tr("Internet connection offline. Automatically switched to local whisper.cpp model."),
                     static_cast<int>(LinuxNotificationService::Urgency::Normal), u"network-offline"_s);
}

void NotificationPresenter::onOnlineRestored() {
    sendNotification(tr("QtScribe: Internet Connection Restored"), tr("Switched back to Cloud transcription engine."),
                     static_cast<int>(LinuxNotificationService::Urgency::Normal), u"network-wireless"_s);
}

void NotificationPresenter::onOfflineModelRequired() {
    sendNotification(tr("QtScribe: Offline Model Required"),
                     tr("No internet connection available. Please open QtScribe and download a Whisper model to "
                        "use offline dictation."),
                     static_cast<int>(LinuxNotificationService::Urgency::Critical), u"dialog-warning"_s);
}

void NotificationPresenter::onNetworkDisconnectedDuringSession() {
    sendNotification(tr("QtScribe: Network Disconnected"),
                     tr("Internet connection lost while recording. Model will not be changed during active session."),
                     static_cast<int>(LinuxNotificationService::Urgency::Normal), u"network-offline"_s);
}

void NotificationPresenter::onModelLoadFailed(const QString& modelPath, const QString& error) {
    Q_UNUSED(error);
    sendNotification(
        tr("QtScribe: Model Load Failed"),
        tr("Could not load offline speech model (%1). Please verify model integrity in Settings.").arg(modelPath),
        static_cast<int>(LinuxNotificationService::Urgency::Normal), u"dialog-error"_s);
}

void NotificationPresenter::sendNotification(const QString& title, const QString& body, int urgency,
                                             const QString& icon) {
    if (m_notificationService) {
        m_notificationService->showNotification(title, body, static_cast<LinuxNotificationService::Urgency>(urgency),
                                                icon);
    }
}

void NotificationPresenter::sendCategoryNotification(const QString& categoryKey, const QString& title,
                                                     const QString& body, int urgency, const QString& icon,
                                                     const QStringList& actions) {
    if (m_notificationService) {
        m_notificationService->showCategoryNotification(
            categoryKey, title, body, static_cast<LinuxNotificationService::Urgency>(urgency), icon, 5000, actions);
    }
}
