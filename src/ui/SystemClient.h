#pragma once

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

class SystemClient : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged FINAL)

    Q_PROPERTY(bool pushToTalkSupported READ pushToTalkSupported NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(bool shortcutAvailable READ shortcutAvailable NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(bool shortcutSupported READ shortcutSupported NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(QString shortcutStatusMessage READ shortcutStatusMessage NOTIFY shortcutStatusChanged FINAL)
    Q_PROPERTY(bool systemShortcutHasIssue READ systemShortcutHasIssue NOTIFY shortcutStatusChanged FINAL)

    Q_PROPERTY(bool textInjectionConnected READ textInjectionConnected NOTIFY textInjectionStatusChanged FINAL)
    Q_PROPERTY(bool textInjectionHasFatalError READ textInjectionHasFatalError NOTIFY textInjectionStatusChanged FINAL)
    Q_PROPERTY(QString textInjectionFatalErrorMessage READ textInjectionFatalErrorMessage NOTIFY textInjectionStatusChanged FINAL)
    Q_PROPERTY(QString textInjectionStatus READ textInjectionStatus NOTIFY textInjectionStatusChanged FINAL)
    Q_PROPERTY(bool textInjectionHasIssue READ textInjectionHasIssue NOTIFY textInjectionStatusChanged FINAL)

    Q_PROPERTY(bool directTypingConnected READ directTypingConnected NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(bool directTypingHasFatalError READ directTypingHasFatalError NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(QString directTypingFatalErrorMessage READ directTypingFatalErrorMessage NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(QString directTypingStatus READ directTypingStatus NOTIFY directTypingStatusChanged FINAL)
    Q_PROPERTY(bool directTypingHasIssue READ directTypingHasIssue NOTIFY directTypingStatusChanged FINAL)

    Q_PROPERTY(bool microphoneAvailable READ microphoneAvailable NOTIFY microphoneStatusChanged FINAL)
    Q_PROPERTY(bool microphoneTestActive READ microphoneTestActive NOTIFY microphoneTestChanged FINAL)
    Q_PROPERTY(qreal microphoneTestLevel READ microphoneTestLevel NOTIFY microphoneTestLevelChanged FINAL)

public:
    explicit SystemClient(QObject* parent = nullptr);
    ~SystemClient() override;

    static SystemClient* instance();
    static SystemClient* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    [[nodiscard]] bool isConnected() const noexcept;

    [[nodiscard]] bool pushToTalkSupported() const noexcept;
    [[nodiscard]] bool shortcutAvailable() const noexcept;
    [[nodiscard]] bool shortcutSupported() const noexcept;
    [[nodiscard]] QString shortcutStatusMessage() const;
    [[nodiscard]] bool systemShortcutHasIssue() const noexcept;

    [[nodiscard]] bool textInjectionConnected() const noexcept;
    [[nodiscard]] bool textInjectionHasFatalError() const noexcept;
    [[nodiscard]] QString textInjectionFatalErrorMessage() const;
    [[nodiscard]] QString textInjectionStatus() const;
    [[nodiscard]] bool textInjectionHasIssue() const noexcept;

    [[nodiscard]] bool directTypingConnected() const noexcept;
    [[nodiscard]] bool directTypingHasFatalError() const noexcept;
    [[nodiscard]] QString directTypingFatalErrorMessage() const;
    [[nodiscard]] QString directTypingStatus() const;
    [[nodiscard]] bool directTypingHasIssue() const noexcept;

    [[nodiscard]] bool microphoneAvailable() const noexcept;
    [[nodiscard]] bool microphoneTestActive() const noexcept;
    [[nodiscard]] qreal microphoneTestLevel() const noexcept;

    static bool sendRemoteCommand(const QString& command);

signals:
    void connectedChanged();
    void shortcutStatusChanged();
    void textInjectionStatusChanged();
    void directTypingStatusChanged();
    void microphoneStatusChanged();
    void microphoneTestChanged();
    void microphoneTestLevelChanged(qreal level);

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
    void setupDBusConnections();
    Q_INVOKABLE void syncFromDaemon();
    Q_INVOKABLE void onServiceRegistered(const QString& serviceName);
    Q_INVOKABLE void onServiceUnregistered(const QString& serviceName);
    Q_INVOKABLE void onPropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties,
                                         const QStringList& invalidatedProperties);
    Q_INVOKABLE void onMicrophoneTestLevelChanged(double level);

    static inline SystemClient* s_instance = nullptr;

    QDBusInterface* m_interface = nullptr;
    QDBusServiceWatcher* m_watcher = nullptr;

    bool m_connected = false;
    bool m_pushToTalkSupported = false;
    bool m_shortcutAvailable = false;
    bool m_shortcutSupported = false;
    QString m_shortcutStatusMessage;

    bool m_directTypingConnected = false;
    bool m_directTypingHasFatalError = false;
    QString m_directTypingFatalErrorMessage;
    QString m_directTypingStatus;

    bool m_microphoneAvailable = false;
    bool m_microphoneTestActive = false;
    qreal m_microphoneTestLevel = 0.0;
};
