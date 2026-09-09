#pragma once

#include "keyinjectord/protocol.h"

#include <QLocalSocket>
#include <QObject>
#include <QProcess>
#include <QString>

class DaemonConnector : public QObject {
    Q_OBJECT

public:
    explicit DaemonConnector(QObject* parent = nullptr);
    ~DaemonConnector() override;

    bool isConnected() const;
    bool hasFatalError() const;
    QString fatalErrorMessage() const;
    QString lastError() const;
    QString statusMessage() const;

    bool connectToServer();
    void disconnectFromServer();
    void stopDaemon();
    void restartService();

    bool sendCommand(keyinjectord::Opcode opcode, int timeoutMs = 2000);
    bool ensureDaemonRunning();

signals:
    void connectedChanged();
    void hasFatalErrorChanged();
    void fatalErrorMessageChanged();
    void lastErrorChanged();
    void statusMessageChanged();

private slots:
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QLocalSocket::LocalSocketError error);

private:
    void setLastError(const QString& error);
    void setStatusMessage(const QString& message);
    void setFatalError(bool fatal, const QString& msg = QString());

    QLocalSocket* m_socket = nullptr;
    QProcess* m_daemonProcess = nullptr;
    bool m_hasFatalError = false;
    QString m_fatalErrorMessage;
    QString m_lastError;
    QString m_statusMessage;
};
