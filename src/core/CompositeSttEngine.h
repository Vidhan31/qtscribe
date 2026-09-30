#pragma once

#include "AbstractSttClient.h"

#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QString>

class AudioRecorder;
class WhisperSttClient;
class WhisperModelManager;
class SystemHealthMonitor;

class CompositeSttEngine : public AbstractSttClient {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(TranscriptionBackend activeBackend READ activeBackend WRITE setActiveBackend NOTIFY activeBackendChanged FINAL)
    Q_PROPERTY(bool autoSwitchOffline READ autoSwitchOffline WRITE setAutoSwitchOffline NOTIFY autoSwitchOfflineChanged FINAL)
    Q_PROPERTY(bool isAutoSwitchedToOffline READ isAutoSwitchedToOffline NOTIFY autoSwitchedToOfflineChanged FINAL)

public:
    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum class TranscriptionBackend { Cloud, WhisperCpp };
    Q_ENUM(TranscriptionBackend)

    explicit CompositeSttEngine(QObject* parent = nullptr);
    ~CompositeSttEngine() override = default;

    void setCloudClient(AbstractSttClient* client);
    [[nodiscard]] AbstractSttClient* cloudClient() const noexcept;

    void setWhisperClient(AbstractSttClient* client);
    [[nodiscard]] AbstractSttClient* whisperClient() const noexcept;

    void setModelManager(WhisperModelManager* manager);
    [[nodiscard]] WhisperModelManager* modelManager() const noexcept;

    void setHealthMonitor(SystemHealthMonitor* monitor);
    [[nodiscard]] SystemHealthMonitor* healthMonitor() const noexcept;

    void setAudioRecorder(AudioRecorder* recorder);
    [[nodiscard]] AudioRecorder* audioRecorder() const noexcept;

    [[nodiscard]] TranscriptionBackend activeBackend() const;
    void setActiveBackend(TranscriptionBackend backend, bool persistSettings = true);

    [[nodiscard]] bool autoSwitchOffline() const;
    void setAutoSwitchOffline(bool enable);

    [[nodiscard]] bool isAutoSwitchedToOffline() const;

    [[nodiscard]] AbstractSttClient* activeSttClient() const;

    void transcribe(const QByteArray& wavData) override;
    void cancel() override;
    void retryLast() override;
    [[nodiscard]] bool isReady() const override;
    [[nodiscard]] bool isBusy() const override;
    [[nodiscard]] bool handlesSmartFormatting() const override;
    [[nodiscard]] QString lastError() const override;

    void activate() override;
    void deactivate() override;

    [[nodiscard]] bool isApiKeySet() const override;
    [[nodiscard]] bool isApiKeyInvalid() const override;
    [[nodiscard]] bool isRateLimited() const override;
    [[nodiscard]] int retrySecondsRemaining() const override;
    [[nodiscard]] ErrorCategory errorCategory() const override;

public slots:
    void onNetworkOnlineChanged(bool online);
    void checkPendingOfflineOrOnlineSwitch();

signals:
    void activeBackendChanged();
    void autoSwitchOfflineChanged();
    void autoSwitchedToOfflineChanged();

    void failoverTriggered();
    void onlineRestored();
    void offlineModelRequired();
    void networkDisconnectedDuringSession();
    void modelLoadFailed(const QString& modelPath, const QString& error);

private slots:
    void onActiveClientTranscriptionReady(const QString& text);
    void onActiveClientError(const QString& error);
    void onWhisperModelLoadFailed(const QString& modelPath, const QString& error);

private:
    void applyOfflineSwitch();
    void restoreOnlineBackend();
    [[nodiscard]] bool isOfflineModelAvailable() const;
    void setLastError(const QString& error);

    QPointer<AbstractSttClient> m_cloudClient;
    QPointer<AbstractSttClient> m_whisperClient;
    QPointer<WhisperModelManager> m_modelManager;
    QPointer<SystemHealthMonitor> m_healthMonitor;
    QPointer<AudioRecorder> m_recorder;

    TranscriptionBackend m_activeBackend = TranscriptionBackend::WhisperCpp;
    bool m_autoSwitchOffline = true;
    bool m_pendingOfflineSwitch = false;
    bool m_autoSwitchedToOffline = false;
    bool m_isBusy = false;
    bool m_active = false;

    QByteArray m_lastWavData;
    QString m_lastError;
};
