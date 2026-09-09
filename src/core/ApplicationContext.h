#pragma once

#include <QObject>

class AudioRecorder;
class ApiKeyStore;
class AudioFeedbackPlayer;
class CloudProviderModel;
class DBusService;
class DictationCoordinator;
class DictationPadModel;
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
class DaemonDiagnosticModel;
class TranscriptionModel;
class WhisperModelManager;
class WhisperSttClient;
class GeminiSttClient;
class CloudSttRouter;
class CompositeSttEngine;
class NotificationPresenter;
class QQmlApplicationEngine;

class ApplicationContext : public QObject {
    Q_OBJECT

public:
    explicit ApplicationContext(QObject* parent = nullptr);
    ~ApplicationContext() override = default;

    void initialize(QQmlApplicationEngine& engine);
    void initializeHeadless();
    [[nodiscard]] bool isInitialized() const noexcept;

    [[nodiscard]] DictationCoordinator* dictationCoordinator() const noexcept;
    [[nodiscard]] CompositeSttEngine* compositeSttEngine() const noexcept;
    [[nodiscard]] NotificationPresenter* notificationPresenter() const noexcept;
    [[nodiscard]] DictationPadModel* dictationPadModel() const noexcept;
    [[nodiscard]] SystemHealthMonitor* systemHealthMonitor() const noexcept;
    [[nodiscard]] AudioFeedbackPlayer* audioFeedbackPlayer() const noexcept;
    [[nodiscard]] ApiKeyStore* apiKeyStore() const noexcept;
    [[nodiscard]] WhisperSttClient* whisperSttClient() const noexcept;
    [[nodiscard]] WhisperModelManager* whisperModelManager() const noexcept;
    [[nodiscard]] AbstractLlmClient* llmClient() const noexcept;
    [[nodiscard]] CloudProviderModel* cloudProviderModel() const noexcept;
    [[nodiscard]] GroqSttClient* groqSttClient() const noexcept;
    [[nodiscard]] GeminiSttClient* geminiSttClient() const noexcept;
    [[nodiscard]] CloudSttRouter* cloudSttRouter() const noexcept;
    [[nodiscard]] AudioRecorder* audioRecorder() const noexcept;
    [[nodiscard]] GlobalShortcutManager* shortcutManager() const noexcept;
    [[nodiscard]] TextInjector* textInjector() const noexcept;
    [[nodiscard]] DaemonConnector* daemonConnector() const noexcept;
    [[nodiscard]] DaemonDiagnosticModel* daemonDiagnosticModel() const noexcept;
    [[nodiscard]] TranscriptionModel* historyModel() const noexcept;
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
    CloudProviderModel* m_cloudProviderModel = nullptr;
    AudioRecorder* m_audioRecorder = nullptr;
    GlobalShortcutManager* m_shortcutManager = nullptr;
    DaemonConnector* m_daemonConnector = nullptr;
    TextInjector* m_textInjector = nullptr;
    DaemonDiagnosticModel* m_daemonDiagnosticModel = nullptr;
    TranscriptionModel* m_historyModel = nullptr;
    DictationCoordinator* m_dictationCoordinator = nullptr;
    DictationPadModel* m_padModel = nullptr;
    SystemHealthMonitor* m_healthMonitor = nullptr;
    AudioFeedbackPlayer* m_feedbackPlayer = nullptr;
    DBusService* m_dbusService = nullptr;
    StatusNotifierService* m_statusNotifierService = nullptr;
    LinuxNotificationService* m_notificationService = nullptr;

    bool m_initialized = false;
};
