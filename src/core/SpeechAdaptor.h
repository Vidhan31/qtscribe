#pragma once

#include <QDBusAbstractAdaptor>
#include <QString>

class DictationCoordinator;

class SpeechAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.qtscribe.SpeechService")

    Q_PROPERTY(int activeBackend READ activeBackend WRITE setActiveBackend NOTIFY activeBackendChanged)
    Q_PROPERTY(int recordingMode READ recordingMode WRITE setRecordingMode NOTIFY recordingModeChanged)
    Q_PROPERTY(int dictationState READ dictationState NOTIFY dictationStateChanged)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY dictationStateChanged)
    Q_PROPERTY(bool canRecord READ canRecord NOTIFY canRecordChanged)
    Q_PROPERTY(double audioLevel READ audioLevel NOTIFY audioLevelChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(bool transcribing READ transcribing NOTIFY transcribingChanged)
    Q_PROPERTY(bool enhancing READ enhancing NOTIFY enhancingChanged)
    Q_PROPERTY(QString lastTranscription READ lastTranscription NOTIFY lastTranscriptionChanged)
    Q_PROPERTY(bool autoSwitchOffline READ autoSwitchOffline WRITE setAutoSwitchOffline NOTIFY autoSwitchOfflineChanged)
    Q_PROPERTY(bool clientWindowFocused READ clientWindowFocused WRITE setClientWindowFocused NOTIFY clientWindowFocusedChanged)
    Q_PROPERTY(QString historyJson READ historyJson NOTIFY historyChanged)

    Q_PROPERTY(bool cloudApiKeySet READ cloudApiKeySet NOTIFY cloudStatusChanged)
    Q_PROPERTY(bool cloudApiKeyInvalid READ cloudApiKeyInvalid NOTIFY cloudStatusChanged)
    Q_PROPERTY(bool cloudRateLimited READ cloudRateLimited NOTIFY cloudStatusChanged)
    Q_PROPERTY(int cloudRetrySecondsRemaining READ cloudRetrySecondsRemaining NOTIFY cloudStatusChanged)
    Q_PROPERTY(QString cloudLastError READ cloudLastError NOTIFY cloudStatusChanged)

public slots:
    void setActiveBackend(int backend, bool persistSettings = true);
    void setRecordingMode(int mode);
    void setAutoSwitchOffline(bool enable);
    void setClientWindowFocused(bool focused);
    QString getHistoryJson() const;
    void toggleRecording();
    void startRecording();
    void stopRecording();
    void cancelDictation();
    void retryTranscription();
    void clearLastTranscription();
    void clearError();
    void clearHistory();
    void reloadSettings();

signals:
    void activeBackendChanged();
    void recordingModeChanged();
    void dictationStateChanged();
    void canRecordChanged();
    void audioLevelChanged(double level);
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

public:
    explicit SpeechAdaptor(DictationCoordinator* coordinator, QObject* parent);
    ~SpeechAdaptor() override = default;

    [[nodiscard]] int activeBackend() const;
    [[nodiscard]] int recordingMode() const;
    [[nodiscard]] int dictationState() const;
    [[nodiscard]] bool isBusy() const;
    [[nodiscard]] bool canRecord() const;
    [[nodiscard]] double audioLevel() const;
    [[nodiscard]] QString statusMessage() const;
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] bool recording() const;
    [[nodiscard]] bool transcribing() const;
    [[nodiscard]] bool enhancing() const;
    [[nodiscard]] QString lastTranscription() const;
    [[nodiscard]] bool autoSwitchOffline() const;
    [[nodiscard]] bool clientWindowFocused() const;
    [[nodiscard]] bool cloudApiKeySet() const;
    [[nodiscard]] bool cloudApiKeyInvalid() const;
    [[nodiscard]] bool cloudRateLimited() const;
    [[nodiscard]] int cloudRetrySecondsRemaining() const;
    [[nodiscard]] QString cloudLastError() const;
    [[nodiscard]] QString historyJson() const;

private:
    DictationCoordinator* m_coordinator;
};
