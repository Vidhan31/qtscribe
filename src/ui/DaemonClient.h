#pragma once

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusServiceWatcher>
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

    Q_PROPERTY(int activeBackend READ activeBackend WRITE setActiveBackend NOTIFY activeBackendChanged FINAL)
    Q_PROPERTY(int recordingMode READ recordingMode WRITE setRecordingMode NOTIFY recordingModeChanged FINAL)
    Q_PROPERTY(int dictationState READ dictationState NOTIFY dictationStateChanged FINAL)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY dictationStateChanged FINAL)
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
    Q_PROPERTY(QString shortcutStatusMessage READ shortcutStatusMessage NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(bool systemShortcutHasIssue READ systemShortcutHasIssue NOTIFY shortcutStatusChanged FINAL)

    Q_PROPERTY(bool directTypingConnected READ directTypingConnected NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(bool directTypingHasFatalError READ directTypingHasFatalError NOTIFY directTypingStatusChanged FINAL)
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

    [[nodiscard]] bool isConnected() const noexcept;

    [[nodiscard]] int activeBackend() const noexcept;
    [[nodiscard]] int recordingMode() const noexcept;
    [[nodiscard]] int dictationState() const noexcept;
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

public slots:
    void setActiveBackend(int backend, bool persistSettings = true);
    void setRecordingMode(int mode);
    void setAutoSwitchOffline(bool enable);
    void setClientWindowFocused(bool focused);

    void startRecording();
    void stopRecording();
    void toggleRecording();
    void cancelDictation();
    void retryTranscription();
    void clearLastTranscription();
    void clearError();
    void clearHistory();
    void reloadSettings();

    void testDirectTyping();
    void restartDirectTyping();
    void stopDirectTyping();

    void showWindow();
    void quitApp();
    void navigateToSection(const QString& section);

    void loadModel(const QString& customPath = QString());
    void unloadModel();
    void checkModelStatus();

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
    void directTypingStatusChanged();
    void modelStatusChanged();
    void computeDeviceChanged();
    void vulkanSupportedChanged();

    void transcriptionFinished(const QString& text);
    void historyCleared();
    void errorOccurred(const QString& error);
    void requestShowWindow();
    void requestNavigate(const QString& section);
    void requestQuitApp();

private slots:
    void onServiceRegistered(const QString& serviceName);
    void onServiceUnregistered(const QString& serviceName);
    void syncFromDaemon();
    void onAudioLevelChanged(double level);
    void onTranscriptionFinished(const QString& text);

private:
    void setupDBusConnections();

    static inline DaemonClient* s_instance = nullptr;

    QDBusServiceWatcher* m_watcher = nullptr;
    QDBusInterface* m_interface = nullptr;
    bool m_connected = false;

    int m_activeBackend = 0;
    int m_recordingMode = 0;
    int m_dictationState = 0;
    bool m_canRecord = false;
    bool m_autoSwitchOffline = true;
    bool m_clientWindowFocused = false;
    qreal m_audioLevel = 0.0;
    QString m_statusMessage;
    QString m_lastError;
    QString m_lastTranscription;

    bool m_pushToTalkSupported = true;
    bool m_shortcutAvailable = true;
    bool m_shortcutSupported = true;
    QString m_shortcutStatusMessage;

    bool m_directTypingConnected = false;
    bool m_directTypingHasFatalError = false;
    QString m_directTypingFatalErrorMessage;
    QString m_directTypingStatus;

    bool m_isModelLoaded = false;
    bool m_isModelBusy = false;
    QString m_loadedModelPath;
    QString m_modelPath;
    QString m_modelFileName;
    QString m_computeDevice;
    bool m_isVulkanSupported = false;
    bool m_isModelInstalled = false;
};
