#pragma once

#include <QObject>

class AudioRecorder;
class ApiKeyStore;
class AudioFeedbackPlayer;
class DBusService;
class DictationCoordinator;
class GlobalShortcutManager;
class GroqLlmClient;
class GroqSttClient;
class AbstractLlmClient;
class LinuxNotificationService;
class StatusNotifierService;
class SystemHealthMonitor;
class AbstractTextInjector;
class TextInjector;
class DaemonConnector;
class WhisperModelManager;
class WhisperSttClient;
class GeminiSttClient;
class CloudSttRouter;
class CompositeSttEngine;
class NotificationPresenter;

class ApplicationContext : public QObject {
    Q_OBJECT

public:
    explicit ApplicationContext(QObject* parent = nullptr);
    ~ApplicationContext() override = default;

    void initializeHeadless(bool startServices = true);
    [[nodiscard]] bool isInitialized() const noexcept;
    void reloadSettings();

    [[nodiscard]] DictationCoordinator* dictationCoordinator() const noexcept;
    [[nodiscard]] CompositeSttEngine* compositeSttEngine() const noexcept;
    [[nodiscard]] NotificationPresenter* notificationPresenter() const noexcept;
    [[nodiscard]] SystemHealthMonitor* systemHealthMonitor() const noexcept;
    [[nodiscard]] AudioFeedbackPlayer* audioFeedbackPlayer() const noexcept;
    [[nodiscard]] ApiKeyStore* apiKeyStore() const noexcept;
    [[nodiscard]] WhisperSttClient* whisperSttClient() const noexcept;
    [[nodiscard]] WhisperModelManager* whisperModelManager() const noexcept;
    [[nodiscard]] AbstractLlmClient* llmClient() const noexcept;
    [[nodiscard]] GroqSttClient* groqSttClient() const noexcept;
    [[nodiscard]] GeminiSttClient* geminiSttClient() const noexcept;
    [[nodiscard]] CloudSttRouter* cloudSttRouter() const noexcept;
    [[nodiscard]] AudioRecorder* audioRecorder() const noexcept;
    [[nodiscard]] GlobalShortcutManager* shortcutManager() const noexcept;
    [[nodiscard]] TextInjector* textInjector() const noexcept;
    [[nodiscard]] DaemonConnector* daemonConnector() const noexcept;
    [[nodiscard]] DBusService* dbusService() const noexcept;
    [[nodiscard]] StatusNotifierService* statusNotifierService() const noexcept;
    [[nodiscard]] LinuxNotificationService* notificationService() const noexcept;

private:
    void wireSubsystems();

    GroqSttClient* m_groqSttClient = nullptr;
    GeminiSttClient* m_geminiSttClient = nullptr;
    CloudSttRouter* m_cloudSttRouter = nullptr;
    WhisperSttClient* m_whisperSttClient = nullptr;
    WhisperModelManager* m_whisperModelManager = nullptr;
    CompositeSttEngine* m_compositeSttEngine = nullptr;
    NotificationPresenter* m_notificationPresenter = nullptr;
    GroqLlmClient* m_groqLlmClient = nullptr;
    AudioRecorder* m_audioRecorder = nullptr;
    GlobalShortcutManager* m_shortcutManager = nullptr;
    DaemonConnector* m_daemonConnector = nullptr;
    TextInjector* m_textInjector = nullptr;
    DictationCoordinator* m_dictationCoordinator = nullptr;
    SystemHealthMonitor* m_healthMonitor = nullptr;
    AudioFeedbackPlayer* m_feedbackPlayer = nullptr;
    DBusService* m_dbusService = nullptr;
    StatusNotifierService* m_statusNotifierService = nullptr;
    LinuxNotificationService* m_notificationService = nullptr;

    bool m_initialized = false;
};
