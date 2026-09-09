#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>

class AudioRecorder;
class CompositeSttEngine;
class DictationCoordinator;
class LinuxNotificationService;

class NotificationPresenter : public QObject {
    Q_OBJECT

public:
    explicit NotificationPresenter(QObject* parent = nullptr);
    ~NotificationPresenter() override = default;

    void setNotificationService(LinuxNotificationService* service);
    [[nodiscard]] LinuxNotificationService* notificationService() const noexcept;

    void setDictationCoordinator(DictationCoordinator* coordinator);
    [[nodiscard]] DictationCoordinator* dictationCoordinator() const noexcept;

    void setAudioRecorder(AudioRecorder* recorder);
    [[nodiscard]] AudioRecorder* audioRecorder() const noexcept;

    void setCompositeSttEngine(CompositeSttEngine* engine);
    [[nodiscard]] CompositeSttEngine* compositeSttEngine() const noexcept;

signals:
    void requestShowWindow();
    void requestNavigate(const QString& section);
    void requestQuitApp();

public slots:
    void onNotificationActionInvoked(uint id, const QString& actionKey);

    void onCoordinatorErrorOccurred(const QString& error);
    void onCoordinatorTranscriptionFinished(const QString& text);
    void onCoordinatorMaxDurationWarning();
    void onCoordinatorLlmFallbackWarning(const QString& warning);

    void onAudioDeviceDisconnected(const QString& deviceName, const QString& fallbackDeviceName);
    void onAudioDeviceAvailabilityChanged();

    void onFailoverTriggered();
    void onOnlineRestored();
    void onOfflineModelRequired();
    void onNetworkDisconnectedDuringSession();
    void onModelLoadFailed(const QString& modelPath, const QString& error);

private:
    void sendNotification(const QString& title, const QString& body, int urgency = 1,
                          const QString& icon = QStringLiteral("dialog-information"));
    void sendCategoryNotification(const QString& categoryKey, const QString& title, const QString& body,
                                  int urgency = 1, const QString& icon = QStringLiteral("dialog-information"),
                                  const QStringList& actions = {});

    QPointer<LinuxNotificationService> m_notificationService;
    QPointer<DictationCoordinator> m_dictationCoordinator;
    QPointer<AudioRecorder> m_audioRecorder;
    QPointer<CompositeSttEngine> m_sttEngine;
};
