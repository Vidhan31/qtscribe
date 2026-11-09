#pragma once

#include <QDBusAbstractAdaptor>
#include <QString>

class SystemService;

class SystemAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.qtscribe.SystemService")

    Q_PROPERTY(bool pushToTalkSupported READ pushToTalkSupported NOTIFY shortcutStatusChanged)
    Q_PROPERTY(bool shortcutAvailable READ shortcutAvailable NOTIFY shortcutStatusChanged)
    Q_PROPERTY(bool shortcutSupported READ shortcutSupported NOTIFY shortcutStatusChanged)
    Q_PROPERTY(QString shortcutStatusMessage READ shortcutStatusMessage NOTIFY shortcutStatusChanged)

    Q_PROPERTY(bool textInjectionConnected READ textInjectionConnected NOTIFY textInjectionStatusChanged)
    Q_PROPERTY(bool textInjectionHasFatalError READ textInjectionHasFatalError NOTIFY textInjectionStatusChanged)
    Q_PROPERTY(QString textInjectionFatalErrorMessage READ textInjectionFatalErrorMessage NOTIFY textInjectionStatusChanged)
    Q_PROPERTY(QString textInjectionStatus READ textInjectionStatus NOTIFY textInjectionStatusChanged)

    Q_PROPERTY(bool directTypingConnected READ directTypingConnected NOTIFY directTypingStatusChanged)
    Q_PROPERTY(bool directTypingHasFatalError READ directTypingHasFatalError NOTIFY directTypingStatusChanged)
    Q_PROPERTY(QString directTypingFatalErrorMessage READ directTypingFatalErrorMessage NOTIFY directTypingStatusChanged)
    Q_PROPERTY(QString directTypingStatus READ directTypingStatus NOTIFY directTypingStatusChanged)

    Q_PROPERTY(bool microphoneAvailable READ microphoneAvailable NOTIFY microphoneStatusChanged)
    Q_PROPERTY(bool microphoneTestActive READ microphoneTestActive NOTIFY microphoneTestChanged)
    Q_PROPERTY(double microphoneTestLevel READ microphoneTestLevel NOTIFY microphoneTestLevelChanged)

public:
    explicit SystemAdaptor(SystemService* service, QObject* parent);
    ~SystemAdaptor() override = default;

    [[nodiscard]] bool pushToTalkSupported() const;
    [[nodiscard]] bool shortcutAvailable() const;
    [[nodiscard]] bool shortcutSupported() const;
    [[nodiscard]] QString shortcutStatusMessage() const;

    [[nodiscard]] bool textInjectionConnected() const;
    [[nodiscard]] bool textInjectionHasFatalError() const;
    [[nodiscard]] QString textInjectionFatalErrorMessage() const;
    [[nodiscard]] QString textInjectionStatus() const;

    [[nodiscard]] bool directTypingConnected() const;
    [[nodiscard]] bool directTypingHasFatalError() const;
    [[nodiscard]] QString directTypingFatalErrorMessage() const;
    [[nodiscard]] QString directTypingStatus() const;

    [[nodiscard]] bool microphoneAvailable() const;
    [[nodiscard]] bool microphoneTestActive() const;
    [[nodiscard]] double microphoneTestLevel() const;

signals:
    void shortcutStatusChanged();
    void textInjectionStatusChanged();
    void directTypingStatusChanged();
    void microphoneStatusChanged();
    void microphoneTestChanged();
    void microphoneTestLevelChanged(double level);
    void requestShowWindow();
    void requestNavigate(const QString& section);
    void requestQuitApp();

public slots:
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

private:
    SystemService* m_service;
};
