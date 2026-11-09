#pragma once

#include "ModelClient.h"
#include "SettingsClient.h"
#include "SpeechClient.h"
#include "SystemClient.h"

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

class DaemonClient : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged FINAL)

    Q_PROPERTY(TranscriptionBackend activeBackend READ activeBackend WRITE setActiveBackend NOTIFY activeBackendChanged FINAL)
    Q_PROPERTY(RecordingMode recordingMode READ recordingMode WRITE setRecordingMode NOTIFY recordingModeChanged FINAL)
    Q_PROPERTY(DictationState dictationState READ dictationState NOTIFY dictationStateChanged FINAL)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY dictationStateChanged FINAL)
    Q_PROPERTY(bool busy READ isBusy NOTIFY dictationStateChanged FINAL)
    Q_PROPERTY(bool canRecord READ canRecord NOTIFY canRecordChanged FINAL)
    Q_PROPERTY(qreal audioLevel READ audioLevel NOTIFY audioLevelChanged FINAL)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged FINAL)
    Q_PROPERTY(bool transcribing READ transcribing NOTIFY transcribingChanged FINAL)
    Q_PROPERTY(bool enhancing READ enhancing NOTIFY enhancingChanged FINAL)
    Q_PROPERTY(QString lastTranscription READ lastTranscription NOTIFY lastTranscriptionChanged FINAL)
    Q_PROPERTY(bool autoSwitchOffline READ autoSwitchOffline WRITE setAutoSwitchOffline NOTIFY autoSwitchOfflineChanged FINAL)
    Q_PROPERTY(bool clientWindowFocused READ clientWindowFocused WRITE setClientWindowFocused NOTIFY clientWindowFocusedChanged FINAL)

    Q_PROPERTY(bool pushToTalkSupported READ pushToTalkSupported NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(bool shortcutAvailable READ shortcutAvailable NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(bool shortcutSupported READ shortcutSupported NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(bool systemShortcutSupported READ shortcutSupported NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(QString shortcutStatusMessage READ shortcutStatusMessage NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(QString systemShortcutStatus READ shortcutStatusMessage NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(bool systemShortcutHasIssue READ systemShortcutHasIssue NOTIFY shortcutStatusChanged FINAL)

    Q_PROPERTY(bool textInjectionConnected READ textInjectionConnected NOTIFY textInjectionStatusChanged FINAL)
    Q_PROPERTY(bool textInjectionHasFatalError READ textInjectionHasFatalError NOTIFY textInjectionStatusChanged FINAL)
    Q_PROPERTY(bool textInjectionFatalError READ textInjectionHasFatalError NOTIFY textInjectionStatusChanged FINAL)
    Q_PROPERTY(QString textInjectionFatalErrorMessage READ textInjectionFatalErrorMessage NOTIFY textInjectionStatusChanged FINAL)
    Q_PROPERTY(QString textInjectionStatus READ textInjectionStatus NOTIFY textInjectionStatusChanged FINAL)
    Q_PROPERTY(bool textInjectionHasIssue READ textInjectionHasIssue NOTIFY textInjectionStatusChanged FINAL)

    Q_PROPERTY(bool directTypingConnected READ directTypingConnected NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(bool directTypingHasFatalError READ directTypingHasFatalError NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(bool directTypingFatalError READ directTypingHasFatalError NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(QString directTypingFatalErrorMessage READ directTypingFatalErrorMessage NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(QString directTypingStatus READ directTypingStatus NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(bool directTypingHasIssue READ directTypingHasIssue NOTIFY directTypingStatusChanged FINAL)

    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(bool isModelBusy READ isModelBusy NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString loadedModelPath READ loadedModelPath NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString modelPath READ modelPath NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString modelFileName READ modelFileName NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString computeDevice READ computeDevice NOTIFY computeDeviceChanged FINAL)
    Q_PROPERTY(bool isVulkanSupported READ isVulkanSupported NOTIFY vulkanSupportedChanged FINAL)
    Q_PROPERTY(bool isModelInstalled READ isModelInstalled NOTIFY modelStatusChanged FINAL)

    Q_PROPERTY(bool microphoneAvailable READ microphoneAvailable NOTIFY microphoneStatusChanged FINAL)
    Q_PROPERTY(bool microphoneTestActive READ microphoneTestActive NOTIFY microphoneTestChanged FINAL)
    Q_PROPERTY(qreal microphoneTestLevel READ microphoneTestLevel NOTIFY microphoneTestLevelChanged FINAL)

    Q_PROPERTY(bool cloudApiKeySet READ cloudApiKeySet NOTIFY cloudStatusChanged FINAL)
    Q_PROPERTY(bool cloudApiKeyInvalid READ cloudApiKeyInvalid NOTIFY cloudStatusChanged FINAL)
    Q_PROPERTY(bool cloudRateLimited READ cloudRateLimited NOTIFY cloudStatusChanged FINAL)
    Q_PROPERTY(int cloudRetrySecondsRemaining READ cloudRetrySecondsRemaining NOTIFY cloudStatusChanged FINAL)
    Q_PROPERTY(QString cloudLastError READ cloudLastError NOTIFY cloudStatusChanged FINAL)

    Q_PROPERTY(QString modelCatalogJson READ modelCatalogJson NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(QString modelsDirectory READ modelsDirectory NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(bool modelDownloadingAny READ modelDownloadingAny NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(QString modelDownloadBytes READ modelDownloadBytes NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(QString modelCatalogLastError READ modelCatalogLastError NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(QString modelDiskSpace READ modelDiskSpace NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(QString historyJson READ historyJson NOTIFY historyChanged FINAL)

public:
    // NOLINTNEXTLINE(performance-enum-size)
    enum class DictationState { Idle = 0, Recording, Transcribing, Enhancing, Error };
    Q_ENUM(DictationState)

    // NOLINTNEXTLINE(performance-enum-size)
    enum class TranscriptionBackend { Cloud = 0, WhisperCpp };
    Q_ENUM(TranscriptionBackend)

    // NOLINTNEXTLINE(performance-enum-size)
    enum class RecordingMode { Toggle = 0, PushToTalk };
    Q_ENUM(RecordingMode)

    explicit DaemonClient(QObject* parent = nullptr);
    ~DaemonClient() override;

    static DaemonClient* instance();
    static DaemonClient* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    [[nodiscard]] SpeechClient* speechClient() const noexcept { return m_speechClient; }
    [[nodiscard]] ModelClient* modelClient() const noexcept { return m_modelClient; }
    [[nodiscard]] SystemClient* systemClient() const noexcept { return m_systemClient; }
    [[nodiscard]] SettingsClient* settingsClient() const noexcept { return m_settingsClient; }

    [[nodiscard]] bool isConnected() const noexcept;
    [[nodiscard]] TranscriptionBackend activeBackend() const noexcept;
    [[nodiscard]] RecordingMode recordingMode() const noexcept;
    [[nodiscard]] DictationState dictationState() const noexcept;
    [[nodiscard]] bool isBusy() const noexcept;
    [[nodiscard]] bool canRecord() const noexcept;
    [[nodiscard]] qreal audioLevel() const noexcept;
    [[nodiscard]] QString statusMessage() const;
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] bool recording() const noexcept;
    [[nodiscard]] bool transcribing() const noexcept;
    [[nodiscard]] bool enhancing() const noexcept;
    [[nodiscard]] QString lastTranscription() const;
    [[nodiscard]] bool autoSwitchOffline() const noexcept;
    [[nodiscard]] bool clientWindowFocused() const noexcept;

    [[nodiscard]] bool pushToTalkSupported() const noexcept;
    [[nodiscard]] bool shortcutAvailable() const noexcept;
    [[nodiscard]] bool shortcutSupported() const noexcept;
    [[nodiscard]] QString shortcutStatusMessage() const;
    [[nodiscard]] bool systemShortcutHasIssue() const noexcept;

    [[nodiscard]] bool textInjectionConnected() const noexcept;
    [[nodiscard]] bool textInjectionHasFatalError() const noexcept;
    [[nodiscard]] QString textInjectionFatalErrorMessage() const;
    [[nodiscard]] QString textInjectionStatus() const;
    [[nodiscard]] bool textInjectionHasIssue() const noexcept;

    [[nodiscard]] bool directTypingConnected() const noexcept;
    [[nodiscard]] bool directTypingHasFatalError() const noexcept;
    [[nodiscard]] QString directTypingFatalErrorMessage() const;
    [[nodiscard]] QString directTypingStatus() const;
    [[nodiscard]] bool directTypingHasIssue() const noexcept;

    [[nodiscard]] bool isModelLoaded() const noexcept;
    [[nodiscard]] bool isModelBusy() const noexcept;
    [[nodiscard]] QString loadedModelPath() const;
    [[nodiscard]] QString modelPath() const;
    [[nodiscard]] QString modelFileName() const;
    [[nodiscard]] QString computeDevice() const;
    [[nodiscard]] bool isVulkanSupported() const noexcept;
    [[nodiscard]] bool isModelInstalled() const noexcept;

    [[nodiscard]] bool microphoneAvailable() const noexcept;
    [[nodiscard]] bool microphoneTestActive() const noexcept;
    [[nodiscard]] qreal microphoneTestLevel() const noexcept;

    [[nodiscard]] bool cloudApiKeySet() const noexcept;
    [[nodiscard]] bool cloudApiKeyInvalid() const noexcept;
    [[nodiscard]] bool cloudRateLimited() const noexcept;
    [[nodiscard]] int cloudRetrySecondsRemaining() const noexcept;
    [[nodiscard]] QString cloudLastError() const;

    [[nodiscard]] QString modelCatalogJson() const;
    [[nodiscard]] QString modelsDirectory() const;
    [[nodiscard]] bool modelDownloadingAny() const noexcept;
    [[nodiscard]] QString modelDownloadBytes() const;
    [[nodiscard]] QString modelCatalogLastError() const;
    [[nodiscard]] QString modelDiskSpace() const;
    [[nodiscard]] QString historyJson() const;

    static bool isServiceRunning();
    static bool sendRemoteCommand(const QString& command);

signals:
    void connectedChanged();
    void activeBackendChanged();
    void recordingModeChanged();
    void dictationStateChanged();
    void canRecordChanged();
    void audioLevelChanged(qreal level);
    void statusMessageChanged();
    void lastErrorChanged();
    void recordingChanged();
    void transcribingChanged();
    void enhancingChanged();
    void lastTranscriptionChanged();
    void autoSwitchOfflineChanged();
    void clientWindowFocusedChanged();
    void shortcutStatusChanged();
    void textInjectionStatusChanged();
    void directTypingStatusChanged();
    void modelStatusChanged();
    void computeDeviceChanged();
    void vulkanSupportedChanged();
    void microphoneStatusChanged();
    void microphoneTestChanged();
    void microphoneTestLevelChanged(qreal level);
    void cloudStatusChanged();
    void modelCatalogChanged();

    void transcriptionFinished(const QString& text);
    void historyCleared();
    void historyChanged();
    void errorOccurred(const QString& error);
    void requestShowWindow();
    void requestNavigate(const QString& section);
    void requestQuitApp();

public slots:
    void setActiveBackend(TranscriptionBackend backend, bool persistSettings = true);
    void setActiveBackend(int backend, bool persistSettings = true);
    void setRecordingMode(RecordingMode mode);
    void setRecordingMode(int mode);
    void setAutoSwitchOffline(bool enable);
    void setClientWindowFocused(bool focused);
    void toggleRecording();
    void startRecording();
    void stopRecording();
    void cancelDictation();
    void retryTranscription();
    void clearLastTranscription();
    void clearError();
    void clearHistory();
    void reloadSettings();

    void testTextInjection();
    void restartTextInjection();
    void stopTextInjection();

    void testDirectTyping();
    void restartDirectTyping();
    void stopDirectTyping();

    void showWindow();
    void quitApp();
    void navigateToSection(const QString& section);

    void loadModel(const QString& customPath = QString());
    void unloadModel();
    void checkModelStatus();

    void startMicrophoneTest();
    void stopMicrophoneTest();
    void playStartSound();
    void playStopSound();

    void selectModel(const QString& modelId);
    void startModelDownload(const QString& modelId);
    void cancelModelDownload(const QString& modelId);
    void deleteModel(const QString& modelId);
    void refreshModelCatalog();
    void checkModelDiskSpace();

private:
    Q_INVOKABLE void onServiceRegistered(const QString& serviceName);
    Q_INVOKABLE void onServiceUnregistered(const QString& serviceName);

    static inline DaemonClient* s_instance = nullptr;

    SpeechClient* m_speechClient = nullptr;
    ModelClient* m_modelClient = nullptr;
    SystemClient* m_systemClient = nullptr;
    SettingsClient* m_settingsClient = nullptr;
};
