#pragma once

#include "AbstractSttClient.h"

#include <QByteArray>
#include <QObject>
#include <QQmlEngine>
#include <QString>

#include <cstdint>

class AudioRecorder;
class AbstractLlmClient;
class AbstractTextInjector;
class GlobalShortcutManager;

class DictationCoordinator : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

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

    [[nodiscard]] TranscriptionBackend activeBackend() const;
    void setActiveBackend(TranscriptionBackend backend, bool persistSettings = true);

    [[nodiscard]] bool autoSwitchOffline() const;
    void setAutoSwitchOffline(bool enable);

    [[nodiscard]] RecordingMode recordingMode() const;
    void setRecordingMode(RecordingMode mode);

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

public slots:
    void initialize();
    void toggleRecording();
    void startRecording();
    void stopRecording();
    void cancelDictation();
    void retryTranscription();

    void clearLastTranscription();
    void clearError();

    void showWindow();
    void quitApp();
    void navigateToSection(const QString& section);

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

    void transcriptionFinished(const QString& text);
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

    AudioRecorder* m_recorder = nullptr;
    AbstractSttClient* m_sttEngine = nullptr;
    AbstractLlmClient* m_llmClient = nullptr;
    GlobalShortcutManager* m_shortcutMgr = nullptr;
    AbstractTextInjector* m_injector = nullptr;

    TranscriptionBackend m_activeBackend = TranscriptionBackend::WhisperCpp;
    DictationState m_state = DictationState::Idle;
    RecordingMode m_recordingMode = RecordingMode::Toggle;
    QString m_statusMessage;
    QString m_lastError;
    QString m_lastTranscription;
    QByteArray m_lastWavData;

    bool m_autoSwitchOffline = true;
    bool m_initialized = false;

    friend class TestDictationCoordinator;
};
