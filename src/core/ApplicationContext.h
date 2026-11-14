#pragma once

#include <QObject>

class AudioRecorder;
class ApiKeyStore;
class AudioFeedbackPlayer;
class DBusService;
class DictationCoordinator;
class HistoryRepository;
class GlobalShortcutManager;
class GroqLlmClient;
class GroqSttClient;
class AbstractLlmClient;
class LinuxNotificationService;
class ModelService;
class StatusNotifierService;
class SystemHealthMonitor;
class SystemService;
class AbstractTextInjector;
class TextInjector;
class DaemonConnector;
class WhisperModelManager;
class WhisperSttClient;
class GeminiSttClient;
class MetaMusaSttClient;
class CloudSttRouter;
class CompositeSttEngine;
class NotificationPresenter;
class SettingsManager;
class SettingsService;

class ApplicationContext : public QObject {
    Q_OBJECT

public:
    explicit ApplicationContext(QObject* parent = nullptr);
    ~ApplicationContext() override = default;

    void initializeHeadless(bool startServices = true);
    [[nodiscard]] bool isInitialized() const noexcept;
    void reloadSettings();

    [[nodiscard]] DictationCoordinator* dictationCoordinator() const noexcept;
    [[nodiscard]] HistoryRepository* historyRepository() const noexcept;
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
    [[nodiscard]] MetaMusaSttClient* metaMusaSttClient() const noexcept;
    [[nodiscard]] CloudSttRouter* cloudSttRouter() const noexcept;
    [[nodiscard]] AudioRecorder* audioRecorder() const noexcept;
    [[nodiscard]] GlobalShortcutManager* shortcutManager() const noexcept;
    [[nodiscard]] TextInjector* textInjector() const noexcept;
    [[nodiscard]] DaemonConnector* daemonConnector() const noexcept;
    [[nodiscard]] DBusService* dbusService() const noexcept;
    [[nodiscard]] StatusNotifierService* statusNotifierService() const noexcept;
    [[nodiscard]] LinuxNotificationService* notificationService() const noexcept;
    [[nodiscard]] ModelService* modelService() const noexcept;
    [[nodiscard]] SystemService* systemService() const noexcept;
    [[nodiscard]] SettingsManager* settingsManager() const noexcept;
    [[nodiscard]] SettingsService* settingsService() const noexcept;

private:
    void wireSubsystems();

    GroqSttClient* m_groqSttClient = nullptr;
    GeminiSttClient* m_geminiSttClient = nullptr;
    MetaMusaSttClient* m_metaMusaSttClient = nullptr;
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
    HistoryRepository* m_historyRepository = nullptr;
    SystemHealthMonitor* m_healthMonitor = nullptr;
    AudioFeedbackPlayer* m_feedbackPlayer = nullptr;
    DBusService* m_dbusService = nullptr;
    StatusNotifierService* m_statusNotifierService = nullptr;
    LinuxNotificationService* m_notificationService = nullptr;
    ModelService* m_modelService = nullptr;
    SystemService* m_systemService = nullptr;
    SettingsManager* m_settingsManager = nullptr;
    SettingsService* m_settingsService = nullptr;

    bool m_initialized = false;
};
