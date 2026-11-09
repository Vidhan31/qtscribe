#pragma once

#include <QObject>
#include <QString>

class AudioFeedbackPlayer;
class AudioRecorder;
class AbstractTextInjector;
class DaemonConnector;
class GlobalShortcutManager;
class DictationCoordinator;

class SystemService : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool pushToTalkSupported READ pushToTalkSupported NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(bool shortcutAvailable READ shortcutAvailable NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(bool shortcutSupported READ shortcutSupported NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(QString shortcutStatusMessage READ shortcutStatusMessage NOTIFY shortcutStatusChanged FINAL)

    Q_PROPERTY(bool textInjectionConnected READ textInjectionConnected NOTIFY textInjectionStatusChanged FINAL)
    Q_PROPERTY(bool textInjectionHasFatalError READ textInjectionHasFatalError NOTIFY textInjectionStatusChanged FINAL)
    Q_PROPERTY(QString textInjectionFatalErrorMessage READ textInjectionFatalErrorMessage NOTIFY textInjectionStatusChanged FINAL)
    Q_PROPERTY(QString textInjectionStatus READ textInjectionStatus NOTIFY textInjectionStatusChanged FINAL)

    Q_PROPERTY(bool directTypingConnected READ directTypingConnected NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(bool directTypingHasFatalError READ directTypingHasFatalError NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(QString directTypingFatalErrorMessage READ directTypingFatalErrorMessage NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(QString directTypingStatus READ directTypingStatus NOTIFY directTypingStatusChanged FINAL)

    Q_PROPERTY(bool microphoneAvailable READ microphoneAvailable NOTIFY microphoneStatusChanged FINAL)
    Q_PROPERTY(bool microphoneTestActive READ microphoneTestActive NOTIFY microphoneTestChanged FINAL)
    Q_PROPERTY(qreal microphoneTestLevel READ microphoneTestLevel NOTIFY microphoneTestLevelChanged FINAL)

public:
    explicit SystemService(QObject* parent = nullptr);
    ~SystemService() override = default;

    void setShortcutManager(GlobalShortcutManager* mgr);
    [[nodiscard]] GlobalShortcutManager* shortcutManager() const noexcept;

    void setDaemonConnector(DaemonConnector* connector);
    [[nodiscard]] DaemonConnector* daemonConnector() const noexcept;

    void setTextInjector(AbstractTextInjector* injector);
    [[nodiscard]] AbstractTextInjector* textInjector() const noexcept;

    void setAudioRecorder(AudioRecorder* recorder);
    [[nodiscard]] AudioRecorder* audioRecorder() const noexcept;

    void setAudioFeedbackPlayer(AudioFeedbackPlayer* player);
    [[nodiscard]] AudioFeedbackPlayer* audioFeedbackPlayer() const noexcept;

    void setDictationCoordinator(DictationCoordinator* coordinator);
    [[nodiscard]] DictationCoordinator* dictationCoordinator() const noexcept;

    [[nodiscard]] bool pushToTalkSupported() const noexcept;
    [[nodiscard]] bool shortcutAvailable() const noexcept;
    [[nodiscard]] bool shortcutSupported() const noexcept;
    [[nodiscard]] QString shortcutStatusMessage() const;

    [[nodiscard]] bool textInjectionConnected() const noexcept;
    [[nodiscard]] bool textInjectionHasFatalError() const noexcept;
    [[nodiscard]] QString textInjectionFatalErrorMessage() const;
    [[nodiscard]] QString textInjectionStatus() const;

    [[nodiscard]] bool directTypingConnected() const noexcept;
    [[nodiscard]] bool directTypingHasFatalError() const noexcept;
    [[nodiscard]] QString directTypingFatalErrorMessage() const;
    [[nodiscard]] QString directTypingStatus() const;

    [[nodiscard]] bool microphoneAvailable() const noexcept;
    [[nodiscard]] bool microphoneTestActive() const noexcept;
    [[nodiscard]] qreal microphoneTestLevel() const noexcept;

    void testTextInjection();
    void restartTextInjection();
    void stopTextInjection();

    void testDirectTyping();
    void restartDirectTyping();
    void stopDirectTyping();

    void startMicrophoneTest();
    void stopMicrophoneTest();
    void playStartSound();
    void playStopSound();

    void showWindow();
    void quitApp();
    void navigateToSection(const QString& section);

signals:
    void shortcutStatusChanged();
    void textInjectionStatusChanged();
    void directTypingStatusChanged();
    void microphoneStatusChanged();
    void microphoneTestChanged();
    void microphoneTestLevelChanged(qreal level);

    void requestShowWindow();
    void requestNavigate(const QString& section);
    void requestQuitApp();

private:
    void onMicrophoneTestAudioLevel(qreal level);

    GlobalShortcutManager* m_shortcutMgr = nullptr;
    DaemonConnector* m_daemonConnector = nullptr;
    AbstractTextInjector* m_injector = nullptr;
    AudioRecorder* m_recorder = nullptr;
    AudioFeedbackPlayer* m_feedbackPlayer = nullptr;
    DictationCoordinator* m_coordinator = nullptr;

    bool m_microphoneTestActive = false;
    qreal m_microphoneTestLevel = 0.0;
};
