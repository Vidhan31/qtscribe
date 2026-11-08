#pragma once

#include "DaemonClient.h"

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

class GuiDictationCoordinator : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(DictationCoordinator)
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
    Q_PROPERTY(bool clientWindowFocused READ clientWindowFocused WRITE setClientWindowFocused NOTIFY clientWindowFocusedChanged FINAL)

    Q_PROPERTY(bool pushToTalkSupported READ pushToTalkSupported NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(bool shortcutAvailable READ shortcutAvailable NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(bool shortcutSupported READ shortcutSupported NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(QString shortcutStatusMessage READ shortcutStatusMessage NOTIFY shortcutStatusChanged FINAL)

    Q_PROPERTY(bool directTypingConnected READ directTypingConnected NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(bool directTypingHasFatalError READ directTypingHasFatalError NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(QString directTypingFatalErrorMessage READ directTypingFatalErrorMessage NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(QString directTypingStatus READ directTypingStatus NOTIFY directTypingStatusChanged FINAL)

public:
    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum class DictationState { Idle = 0, Recording, Transcribing, Enhancing, Error };
    Q_ENUM(DictationState)

    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum class TranscriptionBackend { Cloud = 0, WhisperCpp };
    Q_ENUM(TranscriptionBackend)

    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum class RecordingMode { Toggle = 0, PushToTalk };
    Q_ENUM(RecordingMode)

    explicit GuiDictationCoordinator(QObject* parent = nullptr);
    ~GuiDictationCoordinator() override = default;

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

    [[nodiscard]] bool directTypingConnected() const noexcept;
    [[nodiscard]] bool directTypingHasFatalError() const noexcept;
    [[nodiscard]] QString directTypingFatalErrorMessage() const;
    [[nodiscard]] QString directTypingStatus() const;

public slots:
    void setActiveBackend(TranscriptionBackend backend, bool persistSettings = true);
    void setRecordingMode(RecordingMode mode);
    void setAutoSwitchOffline(bool enable);
    void setClientWindowFocused(bool focused);

    void startRecording();
    void stopRecording();
    void toggleRecording();
    void cancelDictation();
    void retryTranscription();
    void clearLastTranscription();
    void clearError();
    void reloadSettings();

    void testDirectTyping();
    void restartDirectTyping();
    void stopDirectTyping();

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

    void transcriptionFinished(const QString& text);
    void errorOccurred(const QString& error);
    void requestShowWindow();
    void requestNavigate(const QString& section);
    void requestQuitApp();
};
