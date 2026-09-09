#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>

class AbstractTextInjector;
class DaemonConnector;

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

    void setDaemonConnector(DaemonConnector* connector);
    [[nodiscard]] DaemonConnector* daemonConnector() const noexcept;

    void setTextInjector(AbstractTextInjector* injector);
    [[nodiscard]] AbstractTextInjector* textInjector() const noexcept;

    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] bool hasFatalError() const;
    [[nodiscard]] QString fatalErrorMessage() const;
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] QString statusMessage() const;

    [[nodiscard]] bool preventClipboardHistory() const;
    void setPreventClipboardHistory(bool prevent);

    [[nodiscard]] int injectionDelay() const;
    void setInjectionDelay(int delayMs);

    [[nodiscard]] bool isKde() const;
    [[nodiscard]] bool isDevBuild() const;
    [[nodiscard]] bool clipboardWarningAcknowledged() const;
    void setClipboardWarningAcknowledged(bool acknowledged);
    [[nodiscard]] bool clipboardWarningRequired() const;
    [[nodiscard]] bool clipboardBannerDismissed() const;
    void setClipboardBannerDismissed(bool dismissed);

    Q_INVOKABLE void connectToServer();
    Q_INVOKABLE void disconnectFromServer();
    Q_INVOKABLE void stopDaemon();
    Q_INVOKABLE void restartService();
    Q_INVOKABLE bool testTyping(const QString& text = QString());

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
    DaemonConnector* m_connector = nullptr;
    AbstractTextInjector* m_injector = nullptr;
    QString m_lastError;
    QString m_statusMessage;
    int m_injectionDelay = 200;
    bool m_preventClipboardHistory = true;
    bool m_clipboardWarningAcknowledged = false;
    bool m_clipboardBannerDismissed = false;
};
