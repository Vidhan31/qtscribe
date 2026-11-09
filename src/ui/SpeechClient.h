#pragma once

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

class SpeechClient : public QObject {
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
    Q_PROPERTY(QString historyJson READ historyJson NOTIFY historyChanged FINAL)

    Q_PROPERTY(bool cloudApiKeySet READ cloudApiKeySet NOTIFY cloudStatusChanged FINAL)
    Q_PROPERTY(bool cloudApiKeyInvalid READ cloudApiKeyInvalid NOTIFY cloudStatusChanged FINAL)
    Q_PROPERTY(bool cloudRateLimited READ cloudRateLimited NOTIFY cloudStatusChanged FINAL)
    Q_PROPERTY(int cloudRetrySecondsRemaining READ cloudRetrySecondsRemaining NOTIFY cloudStatusChanged FINAL)
    Q_PROPERTY(QString cloudLastError READ cloudLastError NOTIFY cloudStatusChanged FINAL)

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

    explicit SpeechClient(QObject* parent = nullptr);
    ~SpeechClient() override;

    static SpeechClient* instance();
    static SpeechClient* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

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

    [[nodiscard]] bool cloudApiKeySet() const noexcept;
    [[nodiscard]] bool cloudApiKeyInvalid() const noexcept;
    [[nodiscard]] bool cloudRateLimited() const noexcept;
    [[nodiscard]] int cloudRetrySecondsRemaining() const noexcept;
    [[nodiscard]] QString cloudLastError() const;
    [[nodiscard]] QString historyJson() const;

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
    void cloudStatusChanged();

    void transcriptionFinished(const QString& text);
    void historyCleared();
    void historyChanged();
    void errorOccurred(const QString& error);

public slots:
    void setActiveBackend(int backend, bool persistSettings = true);
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

private:
    void setupDBusConnections();
    Q_INVOKABLE void syncFromDaemon();
    Q_INVOKABLE void onServiceRegistered(const QString& serviceName);
    Q_INVOKABLE void onServiceUnregistered(const QString& serviceName);
    Q_INVOKABLE void onPropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties,
                                         const QStringList& invalidatedProperties);
    Q_INVOKABLE void onAudioLevelChanged(double level);

    static inline SpeechClient* s_instance = nullptr;

    QDBusInterface* m_interface = nullptr;
    QDBusServiceWatcher* m_watcher = nullptr;

    bool m_connected = false;
    int m_activeBackend = 0;
    int m_recordingMode = 0;
    int m_dictationState = 0;
    bool m_canRecord = false;
    qreal m_audioLevel = 0.0;
    QString m_statusMessage;
    QString m_lastError;
    QString m_lastTranscription;
    bool m_autoSwitchOffline = true;
    bool m_clientWindowFocused = false;

    bool m_cloudApiKeySet = false;
    bool m_cloudApiKeyInvalid = false;
    bool m_cloudRateLimited = false;
    int m_cloudRetrySecondsRemaining = 0;
    QString m_cloudLastError;
    QString m_historyJson = QStringLiteral("[]");
};
