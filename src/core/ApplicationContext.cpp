#include "ApplicationContext.h"

#include "AudioRecorder.h"
#include "DBusService.h"
#include "DaemonConnector.h"
#include "DictationCoordinator.h"
#include "GroqLlmClient.h"
#include "GroqSttClient.h"
#include "LoggingCategories.h"
#include "TextInjector.h"

#include "ApiKeyStore.h"
#include "AudioFeedbackPlayer.h"
#include "CloudSttRouter.h"
#include "CompositeSttEngine.h"
#include "GeminiSttClient.h"
#include "GlobalShortcutManager.h"
#include "LinuxNotificationService.h"
#include "NotificationPresenter.h"
#include "StatusNotifierService.h"
#include "SystemHealthMonitor.h"
#include "WhisperModelManager.h"
#include "WhisperSttClient.h"

#include <QDBusConnection>
#include <QDBusServiceWatcher>

using namespace Qt::StringLiterals;

ApplicationContext::ApplicationContext(QObject* parent)
    : QObject(parent) { }

void ApplicationContext::initializeHeadless(bool startServices) {
    if (m_initialized) {
        return;
    }

    m_groqSttClient = new GroqSttClient(this);
    m_geminiSttClient = new GeminiSttClient(this);
    m_cloudSttRouter = new CloudSttRouter(this);
    m_whisperModelManager = new WhisperModelManager(this);
    m_whisperSttClient = new WhisperSttClient(this);
    m_compositeSttEngine = new CompositeSttEngine(this);
    m_notificationPresenter = new NotificationPresenter(this);
    m_groqLlmClient = new GroqLlmClient(this);
    m_audioRecorder = new AudioRecorder(this);
    m_shortcutManager = new GlobalShortcutManager(this);
    m_daemonConnector = new DaemonConnector(this);
    m_textInjector = new TextInjector(m_daemonConnector, nullptr, this);
    m_healthMonitor = new SystemHealthMonitor(this);
    m_feedbackPlayer = new AudioFeedbackPlayer(this);
    m_dictationCoordinator = new DictationCoordinator(this);
    m_notificationService = new LinuxNotificationService(this);

    wireSubsystems();

    if (startServices) {
        if (m_daemonConnector) {
            QMetaObject::invokeMethod(m_daemonConnector, &DaemonConnector::connectToServer, Qt::QueuedConnection);
        }

        if (m_dictationCoordinator) {
            m_dbusService = new DBusService(this);
            m_dbusService->registerController(m_dictationCoordinator);

            m_statusNotifierService = new StatusNotifierService(this);
            m_statusNotifierService->registerController(m_dictationCoordinator);

            if (QDBusConnection::sessionBus().isConnected()) {
                auto* guiWatcher =
                    new QDBusServiceWatcher(u"io.github.qtscribe.SettingsGui"_s, QDBusConnection::sessionBus(),
                                            QDBusServiceWatcher::WatchForUnregistration, this);
                connect(guiWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString&) {
                    if (m_dictationCoordinator) {
                        m_dictationCoordinator->setClientWindowFocused(false);
                    }
                });
            }
        }
    }

    m_initialized = true;
    qCDebug(lcSpeech) << "ApplicationContext: Headless composition root successfully initialized";
}

void ApplicationContext::reloadSettings() {
    qCDebug(lcSpeech) << "ApplicationContext: Reloading subsystem settings from QSettings";
    if (m_cloudSttRouter) {
        m_cloudSttRouter->loadSettings();
    }
    if (m_compositeSttEngine) {
        m_compositeSttEngine->loadSettings();
    }
    if (m_groqSttClient) {
        m_groqSttClient->loadSettings();
    }
    if (m_geminiSttClient) {
        m_geminiSttClient->loadSettings();
    }
    if (m_groqLlmClient) {
        m_groqLlmClient->loadSettings();
    }
    if (m_whisperModelManager) {
        m_whisperModelManager->loadSettings();
    }
    if (m_whisperSttClient) {
        m_whisperSttClient->loadSettings();
    }
    if (m_feedbackPlayer) {
        m_feedbackPlayer->loadSettings();
    }
    if (m_notificationService) {
        m_notificationService->loadSettings();
    }
    if (m_textInjector) {
        m_textInjector->loadSettings();
    }
    if (m_dictationCoordinator) {
        m_dictationCoordinator->loadSettings();
    }
}

void ApplicationContext::wireSubsystems() {
    if (m_whisperSttClient && m_whisperModelManager) {
        m_whisperSttClient->setModelManager(m_whisperModelManager);
    }
    if (m_groqLlmClient && m_groqSttClient) {
        m_groqLlmClient->setKeyStore(m_groqSttClient->keyStore());
    }

    if (m_cloudSttRouter) {
        m_cloudSttRouter->registerProvider(u"groq"_s, m_groqSttClient);
        m_cloudSttRouter->registerProvider(u"gemini"_s, m_geminiSttClient);
    }

    if (m_compositeSttEngine) {
        AbstractSttClient* cloudClient = m_cloudSttRouter ? static_cast<AbstractSttClient*>(m_cloudSttRouter)
                                                          : static_cast<AbstractSttClient*>(m_groqSttClient);
        m_compositeSttEngine->setCloudClient(cloudClient);
        m_compositeSttEngine->setWhisperClient(m_whisperSttClient);
        m_compositeSttEngine->setModelManager(m_whisperModelManager);
        m_compositeSttEngine->setHealthMonitor(m_healthMonitor);
        m_compositeSttEngine->setAudioRecorder(m_audioRecorder);
    }

    if (m_healthMonitor) {
        m_healthMonitor->setShortcutManager(m_shortcutManager);
        m_healthMonitor->setDaemonConnector(m_daemonConnector);
        m_healthMonitor->setAudioRecorder(m_audioRecorder);
        m_healthMonitor->setActiveSttClient(m_compositeSttEngine);
    }

    if (m_notificationPresenter) {
        m_notificationPresenter->setNotificationService(m_notificationService);
        m_notificationPresenter->setDictationCoordinator(m_dictationCoordinator);
        m_notificationPresenter->setAudioRecorder(m_audioRecorder);
        m_notificationPresenter->setCompositeSttEngine(m_compositeSttEngine);

        if (m_dictationCoordinator) {
            connect(m_notificationPresenter, &NotificationPresenter::requestShowWindow, m_dictationCoordinator,
                    &DictationCoordinator::showWindow);
            connect(m_notificationPresenter, &NotificationPresenter::requestNavigate, m_dictationCoordinator,
                    &DictationCoordinator::navigateToSection);
            connect(m_notificationPresenter, &NotificationPresenter::requestQuitApp, m_dictationCoordinator,
                    &DictationCoordinator::quitApp);
        }
    }

    if (m_dictationCoordinator) {
        m_dictationCoordinator->setAudioRecorder(m_audioRecorder);
        m_dictationCoordinator->setSttEngine(m_compositeSttEngine);
        m_dictationCoordinator->setLlmClient(m_groqLlmClient);
        m_dictationCoordinator->setShortcutManager(m_shortcutManager);
        m_dictationCoordinator->setTextInjector(m_textInjector);
        m_dictationCoordinator->setDaemonConnector(m_daemonConnector);

        if (m_feedbackPlayer) {
            connect(m_dictationCoordinator, &DictationCoordinator::recordingChanged, this, [this]() {
                if (m_dictationCoordinator && m_feedbackPlayer) {
                    if (m_dictationCoordinator->recording()) {
                        m_feedbackPlayer->playStartSound();
                    } else {
                        m_feedbackPlayer->playStopSound();
                    }
                }
            });
        }

        m_dictationCoordinator->initialize();
    }

    if (m_whisperModelManager && m_notificationService) {
        connect(m_whisperModelManager, &WhisperModelManager::modelDownloadFinished, this,
                [this](const QString& modelId, bool success, const QString& error) {
                    if (success) {
                        m_notificationService->showCategoryNotification(
                            u"model_download"_s, tr("Offline Model Ready"),
                            tr("Whisper model '%1' downloaded and ready for dictation.").arg(modelId),
                            LinuxNotificationService::Urgency::Normal, u"software-update-available"_s, 5000,
                            {u"offline"_s, tr("Offline Settings")});
                    } else if (!error.isEmpty() && !error.contains(u"cancelled"_s, Qt::CaseInsensitive)) {
                        m_notificationService->showCategoryNotification(
                            u"model_download"_s, tr("Model Download Failed"),
                            tr("Failed to download '%1': %2").arg(modelId, error),
                            LinuxNotificationService::Urgency::Critical, u"dialog-error"_s, 6000,
                            {u"offline"_s, tr("Retry in Settings")});
                    }
                });
    }

    if (m_daemonConnector && m_notificationService) {
        connect(m_daemonConnector, &DaemonConnector::hasFatalErrorChanged, this, [this]() {
            if (m_daemonConnector->hasFatalError()) {
                m_notificationService->showCategoryNotification(
                    u"daemon_error"_s, tr("Direct Typing Service Stopped"),
                    m_daemonConnector->fatalErrorMessage().isEmpty()
                        ? tr("Key injection daemon stopped. Text falling back to clipboard paste.")
                        : m_daemonConnector->fatalErrorMessage(),
                    LinuxNotificationService::Urgency::Critical, u"dialog-warning"_s, 6000,
                    {u"system"_s, tr("System Settings")});
            }
        });
    }
}

bool ApplicationContext::isInitialized() const noexcept {
    return m_initialized;
}

DictationCoordinator* ApplicationContext::dictationCoordinator() const noexcept {
    return m_dictationCoordinator;
}

CompositeSttEngine* ApplicationContext::compositeSttEngine() const noexcept {
    return m_compositeSttEngine;
}

NotificationPresenter* ApplicationContext::notificationPresenter() const noexcept {
    return m_notificationPresenter;
}

SystemHealthMonitor* ApplicationContext::systemHealthMonitor() const noexcept {
    return m_healthMonitor;
}

AudioFeedbackPlayer* ApplicationContext::audioFeedbackPlayer() const noexcept {
    return m_feedbackPlayer;
}

ApiKeyStore* ApplicationContext::apiKeyStore() const noexcept {
    return m_groqSttClient ? m_groqSttClient->keyStore() : nullptr;
}

WhisperSttClient* ApplicationContext::whisperSttClient() const noexcept {
    return m_whisperSttClient;
}

WhisperModelManager* ApplicationContext::whisperModelManager() const noexcept {
    return m_whisperModelManager;
}

AbstractLlmClient* ApplicationContext::llmClient() const noexcept {
    return m_groqLlmClient;
}

GroqSttClient* ApplicationContext::groqSttClient() const noexcept {
    return m_groqSttClient;
}

GeminiSttClient* ApplicationContext::geminiSttClient() const noexcept {
    return m_geminiSttClient;
}

CloudSttRouter* ApplicationContext::cloudSttRouter() const noexcept {
    return m_cloudSttRouter;
}

AudioRecorder* ApplicationContext::audioRecorder() const noexcept {
    return m_audioRecorder;
}

GlobalShortcutManager* ApplicationContext::shortcutManager() const noexcept {
    return m_shortcutManager;
}

TextInjector* ApplicationContext::textInjector() const noexcept {
    return m_textInjector;
}

DaemonConnector* ApplicationContext::daemonConnector() const noexcept {
    return m_daemonConnector;
}

DBusService* ApplicationContext::dbusService() const noexcept {
    return m_dbusService;
}

StatusNotifierService* ApplicationContext::statusNotifierService() const noexcept {
    return m_statusNotifierService;
}

LinuxNotificationService* ApplicationContext::notificationService() const noexcept {
    return m_notificationService;
}
