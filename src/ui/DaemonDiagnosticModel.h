#pragma once

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

// Read-mostly diagnostics view over the daemon's text-injection state.
//
// The GUI never owns the injection connector; it only reflects and commands the
// daemon over D-Bus.
class DaemonDiagnosticModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged FINAL)
    Q_PROPERTY(bool hasFatalError READ hasFatalError NOTIFY hasFatalErrorChanged FINAL)
    Q_PROPERTY(QString fatalErrorMessage READ fatalErrorMessage NOTIFY fatalErrorMessageChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged FINAL)

    Q_PROPERTY(bool preventClipboardHistory READ preventClipboardHistory WRITE setPreventClipboardHistory NOTIFY preventClipboardHistoryChanged FINAL)
    Q_PROPERTY(int injectionDelay READ injectionDelay WRITE setInjectionDelay NOTIFY injectionDelayChanged FINAL)

    Q_PROPERTY(bool isKde READ isKde CONSTANT FINAL)
    Q_PROPERTY(bool isDevBuild READ isDevBuild CONSTANT FINAL)
    Q_PROPERTY(bool clipboardWarningAcknowledged READ clipboardWarningAcknowledged WRITE setClipboardWarningAcknowledged NOTIFY clipboardWarningAcknowledgedChanged FINAL)
    Q_PROPERTY(bool clipboardWarningRequired READ clipboardWarningRequired NOTIFY clipboardWarningRequiredChanged FINAL)
    Q_PROPERTY(bool clipboardBannerDismissed READ clipboardBannerDismissed WRITE setClipboardBannerDismissed NOTIFY clipboardBannerDismissedChanged FINAL)

public:
    explicit DaemonDiagnosticModel(QObject* parent = nullptr);
    ~DaemonDiagnosticModel() override = default;

    [[nodiscard]] static bool isConnected();
    [[nodiscard]] static bool hasFatalError();
    [[nodiscard]] static QString fatalErrorMessage();
    [[nodiscard]] static QString lastError();
    [[nodiscard]] static QString statusMessage();

    [[nodiscard]] bool preventClipboardHistory() const;
    void setPreventClipboardHistory(bool prevent);

    [[nodiscard]] int injectionDelay() const;
    void setInjectionDelay(int delayMs);

    [[nodiscard]] static bool isKde();
    [[nodiscard]] static bool isDevBuild();
    [[nodiscard]] bool clipboardWarningAcknowledged() const;
    void setClipboardWarningAcknowledged(bool acknowledged);
    [[nodiscard]] bool clipboardWarningRequired() const;
    [[nodiscard]] bool clipboardBannerDismissed() const;
    void setClipboardBannerDismissed(bool dismissed);

    Q_INVOKABLE static void connectToServer();
    Q_INVOKABLE static void disconnectFromServer();
    Q_INVOKABLE static void stopDaemon();
    Q_INVOKABLE static void restartService();
    Q_INVOKABLE static bool testTextInjection(const QString& text = QString());
    Q_INVOKABLE static bool testTyping(const QString& text = QString());

signals:
    void connectedChanged();
    void hasFatalErrorChanged();
    void fatalErrorMessageChanged();
    void lastErrorChanged();
    void statusMessageChanged();
    void preventClipboardHistoryChanged();
    void injectionDelayChanged();
    void clipboardWarningAcknowledgedChanged();
    void clipboardWarningRequiredChanged();
    void clipboardBannerDismissedChanged();

private:
    static void reloadDaemonSettings();

    int m_injectionDelay = 200;
    bool m_preventClipboardHistory = true;
    bool m_clipboardWarningAcknowledged = false;
    bool m_clipboardBannerDismissed = false;
};
