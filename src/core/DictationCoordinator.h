#pragma once

#include "AbstractSttClient.h"

#include <QByteArray>
#include <QObject>
#include <QString>

#include <cstdint>

class AudioRecorder;
class AbstractLlmClient;
class AbstractTextInjector;
class DaemonConnector;
class GlobalShortcutManager;
class WhisperSttClient;

class DictationCoordinator : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.qtscribe.SpeechService")

    Q_PROPERTY(TranscriptionBackend activeBackend READ activeBackend WRITE setActiveBackend NOTIFY activeBackendChanged FINAL)
    Q_PROPERTY(RecordingMode recordingMode READ recordingMode WRITE setRecordingMode NOTIFY recordingModeChanged FINAL)
    Q_PROPERTY(DictationState dictationState READ dictationState NOTIFY dictationStateChanged FINAL)
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

    Q_PROPERTY(bool directTypingConnected READ directTypingConnected NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(bool directTypingHasFatalError READ directTypingHasFatalError NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(QString directTypingFatalErrorMessage READ directTypingFatalErrorMessage NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(QString directTypingStatus READ directTypingStatus NOTIFY directTypingStatusChanged FINAL)

    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(bool isModelBusy READ isModelBusy NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString loadedModelPath READ loadedModelPath NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString modelPath READ modelPath NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString modelFileName READ modelFileName NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString computeDevice READ computeDevice NOTIFY computeDeviceChanged FINAL)
    Q_PROPERTY(bool isVulkanSupported READ isVulkanSupported CONSTANT FINAL)
    Q_PROPERTY(bool isModelInstalled READ isModelInstalled NOTIFY modelStatusChanged FINAL)

public:
    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum class DictationState { Idle, Recording, Transcribing, Enhancing, Error };
    Q_ENUM(DictationState)

    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum class TranscriptionBackend { Cloud, WhisperCpp };
    Q_ENUM(TranscriptionBackend)

    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum class RecordingMode { Toggle, PushToTalk };
    Q_ENUM(RecordingMode)

public:
    explicit DictationCoordinator(QObject* parent = nullptr);
    ~DictationCoordinator() override = default;

    void setAudioRecorder(AudioRecorder* recorder);
    void setSttEngine(AbstractSttClient* engine);
    [[nodiscard]] AbstractSttClient* sttEngine() const noexcept;
    [[nodiscard]] AbstractSttClient* activeSttClient() const noexcept;

    void setLlmClient(AbstractLlmClient* llmClient);
    void setShortcutManager(GlobalShortcutManager* mgr);
    void setTextInjector(AbstractTextInjector* injector);
    [[nodiscard]] AbstractTextInjector* textInjector() const noexcept;
    void setDaemonConnector(DaemonConnector* connector);
    [[nodiscard]] DaemonConnector* daemonConnector() const noexcept;

    [[nodiscard]] TranscriptionBackend activeBackend() const;
    [[nodiscard]] bool autoSwitchOffline() const;
    [[nodiscard]] RecordingMode recordingMode() const;

    [[nodiscard]] DictationState dictationState() const;
    [[nodiscard]] bool isBusy() const;
    [[nodiscard]] bool canRecord() const;
    [[nodiscard]] qreal audioLevel() const;
    [[nodiscard]] QString statusMessage() const;
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] bool recording() const;
    [[nodiscard]] bool transcribing() const;
    [[nodiscard]] bool enhancing() const;
    [[nodiscard]] QString lastTranscription() const;
    [[nodiscard]] bool clientWindowFocused() const noexcept;

    [[nodiscard]] bool pushToTalkSupported() const noexcept;
    [[nodiscard]] bool shortcutAvailable() const noexcept;
    [[nodiscard]] bool shortcutSupported() const noexcept;
    [[nodiscard]] QString shortcutStatusMessage() const;

    [[nodiscard]] bool directTypingConnected() const noexcept;
    [[nodiscard]] bool directTypingHasFatalError() const noexcept;
    [[nodiscard]] QString directTypingFatalErrorMessage() const;
    [[nodiscard]] QString directTypingStatus() const;

    [[nodiscard]] bool isModelLoaded() const;
    [[nodiscard]] bool isModelBusy() const;
    [[nodiscard]] QString loadedModelPath() const;
    [[nodiscard]] QString modelPath() const;
    [[nodiscard]] QString modelFileName() const;
    [[nodiscard]] QString computeDevice() const;
    [[nodiscard]] bool isVulkanSupported() const;
    [[nodiscard]] bool isModelInstalled() const;

    [[nodiscard]] static QString historyFilePath();
    static void removeHistoryFile();

    void loadSettings();

public slots:
    void initialize();
    void reloadSettings();
    void setActiveBackend(TranscriptionBackend backend, bool persistSettings = true);
    void setActiveBackend(int backend, bool persistSettings = true);
    void setAutoSwitchOffline(bool enable);
    void setRecordingMode(RecordingMode mode);
    void setRecordingMode(int mode);
    void setClientWindowFocused(bool focused);
    void toggleRecording();
    void startRecording();
    void stopRecording();
    void cancelDictation();
    void retryTranscription();

    void clearLastTranscription();
    void clearError();
    void clearHistory();

    void showWindow();
    void quitApp();
    void navigateToSection(const QString& section);

    void testDirectTyping();
    void restartDirectTyping();
    void stopDirectTyping();

    void loadModel(const QString& customPath = QString());
    void unloadModel();
    void checkModelStatus();

signals:
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

    void transcriptionFinished(const QString& text);
    void historyCleared();
    void errorOccurred(const QString& error);
    void requestShowWindow();
    void requestNavigate(const QString& section);
    void requestQuitApp();
    void maxDurationWarningTriggered();
    void llmFallbackWarningTriggered(const QString& warning);

private slots:
    void onRecordingFinished(const QByteArray& wavData);
    void onMaxDurationReached();
    void onSttTranscriptionReady(const QString& text);
    void onSttError(const QString& error);
    void onLlmEnhancementReady(const QString& enhancedText);
    void onLlmError(const QString& error, const QString& fallbackRawText);
    void onShortcutActivated(const QString& shortcutId);
    void onShortcutDeactivated(const QString& shortcutId);
    void updateCoordinatorHealth();

private:
    void setState(DictationState state);
    void setStatusMessage(const QString& msg);
    void setLastError(const QString& error);
    void completeTranscription(const QString& text);
    void finishTranscriptionAndInject(const QString& text);

    [[nodiscard]] WhisperSttClient* whisperClient() const noexcept;

    AudioRecorder* m_recorder = nullptr;
    AbstractSttClient* m_sttEngine = nullptr;
    AbstractLlmClient* m_llmClient = nullptr;
    GlobalShortcutManager* m_shortcutMgr = nullptr;
    AbstractTextInjector* m_injector = nullptr;
    DaemonConnector* m_daemonConnector = nullptr;

    TranscriptionBackend m_activeBackend = TranscriptionBackend::WhisperCpp;
    DictationState m_state = DictationState::Idle;
    RecordingMode m_recordingMode = RecordingMode::Toggle;
    QString m_statusMessage;
    QString m_lastError;
    QString m_lastTranscription;
    QByteArray m_lastWavData;

    bool m_autoSwitchOffline = true;
    bool m_clientWindowFocused = false;
    bool m_initialized = false;

    friend class TestDictationCoordinator;
};

Q_DECLARE_METATYPE(DictationCoordinator::TranscriptionBackend)
Q_DECLARE_METATYPE(DictationCoordinator::RecordingMode)
Q_DECLARE_METATYPE(DictationCoordinator::DictationState)
