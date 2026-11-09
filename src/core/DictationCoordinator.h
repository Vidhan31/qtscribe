#pragma once

#include "AbstractSttClient.h"

#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QString>

#include <cstdint>
#include <memory>

class AudioRecorder;
class AbstractCloudSttClient;
class AbstractLlmClient;
class AbstractTextInjector;
class GlobalShortcutManager;
class SettingsManager;
class HistoryRepository;
class SystemService;

class DictationCoordinator : public QObject {
    Q_OBJECT

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

    Q_PROPERTY(bool cloudApiKeySet READ cloudApiKeySet NOTIFY cloudStatusChanged FINAL)
    Q_PROPERTY(bool cloudApiKeyInvalid READ cloudApiKeyInvalid NOTIFY cloudStatusChanged FINAL)
    Q_PROPERTY(bool cloudRateLimited READ cloudRateLimited NOTIFY cloudStatusChanged FINAL)
    Q_PROPERTY(int cloudRetrySecondsRemaining READ cloudRetrySecondsRemaining NOTIFY cloudStatusChanged FINAL)
    Q_PROPERTY(QString cloudLastError READ cloudLastError NOTIFY cloudStatusChanged FINAL)

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

    explicit DictationCoordinator(QObject* parent = nullptr);
    ~DictationCoordinator() override;

    void setAudioRecorder(AudioRecorder* recorder);
    void setSttEngine(AbstractSttClient* engine);
    [[nodiscard]] AbstractSttClient* sttEngine() const noexcept;
    [[nodiscard]] AbstractSttClient* activeSttClient() const noexcept;

    void setCloudClient(AbstractCloudSttClient* client);
    [[nodiscard]] AbstractCloudSttClient* cloudClient() const noexcept;

    void setLlmClient(AbstractLlmClient* llmClient);
    void setShortcutManager(GlobalShortcutManager* mgr);
    void setTextInjector(AbstractTextInjector* injector);
    [[nodiscard]] AbstractTextInjector* textInjector() const noexcept;

    void setSystemService(SystemService* systemService);
    [[nodiscard]] SystemService* systemService() const noexcept;

    void setSettingsManager(SettingsManager* settings);
    [[nodiscard]] SettingsManager* settingsManager() const noexcept;

    void setHistoryRepository(HistoryRepository* repo);
    [[nodiscard]] HistoryRepository* historyRepository();

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

    [[nodiscard]] bool cloudApiKeySet() const;
    [[nodiscard]] bool cloudApiKeyInvalid() const;
    [[nodiscard]] bool cloudRateLimited() const;
    [[nodiscard]] int cloudRetrySecondsRemaining() const;
    [[nodiscard]] QString cloudLastError() const;

    [[nodiscard]] static QString historyFilePath();
    static void removeHistoryFile();

    void loadSettings();

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
    void cloudStatusChanged();

    void transcriptionFinished(const QString& text);
    void historyCleared();
    void historyChanged();
    void errorOccurred(const QString& error);
    void maxDurationWarningTriggered();
    void llmFallbackWarningTriggered(const QString& warning);

private:
    void onRecordingFinished(const QByteArray& wavData);
    void onMaxDurationReached();
    void onSttTranscriptionReady(const QString& text);
    void onSttError(const QString& error);
    void onLlmEnhancementReady(const QString& enhancedText);
    void onLlmError(const QString& error, const QString& fallbackRawText);
    void onShortcutActivated(const QString& shortcutId);
    void onShortcutDeactivated(const QString& shortcutId);
    void updateCoordinatorHealth();

    void setState(DictationState state);
    void setStatusMessage(const QString& msg);
    void setLastError(const QString& error);
    void completeTranscription(const QString& text);
    void finishTranscriptionAndInject(const QString& text);

    AudioRecorder* m_recorder = nullptr;
    AbstractSttClient* m_sttEngine = nullptr;
    QPointer<AbstractCloudSttClient> m_cloudClient;
    AbstractLlmClient* m_llmClient = nullptr;
    GlobalShortcutManager* m_shortcutMgr = nullptr;
    AbstractTextInjector* m_injector = nullptr;
    SettingsManager* m_settingsManager = nullptr;
    HistoryRepository* m_historyRepository = nullptr;
    std::unique_ptr<HistoryRepository> m_ownedHistoryRepository;
    SystemService* m_systemService = nullptr;

    TranscriptionBackend m_activeBackend = TranscriptionBackend::WhisperCpp;
    DictationState m_state = DictationState::Idle;
    RecordingMode m_recordingMode = RecordingMode::Toggle;
    QString m_statusMessage;
    QString m_lastError;
    QString m_lastTranscription;
    QString m_pendingRawText;
    QByteArray m_lastWavData;

    bool m_autoSwitchOffline = true;
    bool m_clientWindowFocused = false;
    bool m_initialized = false;

    friend class TestDictationCoordinator;
};

Q_DECLARE_METATYPE(DictationCoordinator::TranscriptionBackend)
Q_DECLARE_METATYPE(DictationCoordinator::RecordingMode)
Q_DECLARE_METATYPE(DictationCoordinator::DictationState)
