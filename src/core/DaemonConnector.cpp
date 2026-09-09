#include "DaemonConnector.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QThread>

#include <cerrno>
#include <cstring>

#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

DaemonConnector::DaemonConnector(QObject* parent)
    : QObject(parent)
    , m_socket(new QLocalSocket(this)) {
    connect(m_socket, &QLocalSocket::connected, this, &DaemonConnector::onConnected);
    connect(m_socket, &QLocalSocket::disconnected, this, &DaemonConnector::onDisconnected);
    connect(m_socket, &QLocalSocket::errorOccurred, this, &DaemonConnector::onErrorOccurred);

    connect(qApp, &QCoreApplication::aboutToQuit, this, &DaemonConnector::stopDaemon);
}

DaemonConnector::~DaemonConnector() {
    stopDaemon();
}

bool DaemonConnector::isConnected() const {
    return m_socket && m_socket->state() == QLocalSocket::ConnectedState;
}

bool DaemonConnector::hasFatalError() const {
    return m_hasFatalError;
}

QString DaemonConnector::fatalErrorMessage() const {
    return m_fatalErrorMessage;
}

QString DaemonConnector::lastError() const {
    return m_lastError;
}

QString DaemonConnector::statusMessage() const {
    return m_statusMessage;
}

bool DaemonConnector::ensureDaemonRunning() {
    if (isConnected()) {
        return true;
    }

    if (m_daemonProcess && m_daemonProcess->state() != QProcess::NotRunning && m_socket &&
        m_socket->state() == QLocalSocket::ConnectedState) {
        return true;
    }

    if (m_socket->state() != QLocalSocket::UnconnectedState) {
        m_socket->close();
    }
    if (m_daemonProcess && m_daemonProcess->state() != QProcess::NotRunning) {
        m_daemonProcess->terminate();
        if (!m_daemonProcess->waitForFinished(500)) {
            m_daemonProcess->kill();
        }
        delete m_daemonProcess;
        m_daemonProcess = nullptr;
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidatePaths = {appDir + u"/keyinjectord"_s, QStandardPaths::findExecutable(u"keyinjectord"_s)};

    QString lastCapturedError;

    for (const QString& daemonExecutable : candidatePaths) {
        if (daemonExecutable.isEmpty() || !QFile::exists(daemonExecutable)) {
            continue;
        }

        setStatusMessage(u"Starting keyinjectord (%1)..."_s.arg(daemonExecutable));

        int fds[2];
        if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds) != 0) {
            lastCapturedError =
                u"Failed to create IPC socketpair: %1"_s.arg(QString::fromLocal8Bit(std::strerror(errno)));
            setLastError(lastCapturedError);
            setFatalError(true, lastCapturedError);
            return false;
        }

        int parentFd = fds[0];
        int childFd = fds[1];

        if (!m_daemonProcess) {
            m_daemonProcess = new QProcess(this);
        }

        m_daemonProcess->setProcessChannelMode(QProcess::MergedChannels);

        m_daemonProcess->setChildProcessModifier([childFd, parentFd]() {
            ::close(parentFd);
            int flags = fcntl(childFd, F_GETFD);
            if (flags >= 0) {
                fcntl(childFd, F_SETFD, flags & ~FD_CLOEXEC);
            }
        });

        m_daemonProcess->start(daemonExecutable, {u"--socket-fd"_s, QString::number(childFd)});
        ::close(childFd);

        if (!m_daemonProcess->waitForStarted(2000)) {
            ::close(parentFd);
            lastCapturedError = u"Failed to launch %1: %2"_s.arg(daemonExecutable, m_daemonProcess->errorString());
            delete m_daemonProcess;
            m_daemonProcess = nullptr;
            continue;
        }

        if (!m_socket->setSocketDescriptor(parentFd, QLocalSocket::ConnectedState, QIODevice::ReadWrite)) {
            ::close(parentFd);
            lastCapturedError = u"Failed to attach socket descriptor in QLocalSocket"_s;
            m_daemonProcess->terminate();
            delete m_daemonProcess;
            m_daemonProcess = nullptr;
            continue;
        }
        if (m_daemonProcess->waitForFinished(50)) {
            QByteArray output = m_daemonProcess->readAll();
            lastCapturedError = QString::fromUtf8(output).trimmed();
            if (lastCapturedError.isEmpty()) {
                lastCapturedError = daemonExecutable + u" exited immediately with exit code "_s +
                                    QString::number(m_daemonProcess->exitCode());
            }
            m_socket->close();
            delete m_daemonProcess;
            m_daemonProcess = nullptr;
            continue;
        }

        onConnected();
        return true;
    }

    if (!lastCapturedError.isEmpty()) {
        setLastError(lastCapturedError);
        setFatalError(true, lastCapturedError);
    } else {
        const QString notFoundErr = u"keyinjectord binary not found or failed to start."_s;
        setLastError(notFoundErr);
        setFatalError(true, notFoundErr);
    }
    return false;
}

bool DaemonConnector::connectToServer() {
    if (isConnected()) {
        setStatusMessage(u"Already connected"_s);
        return true;
    }

    return ensureDaemonRunning();
}

void DaemonConnector::disconnectFromServer() {
    if (m_socket && m_socket->state() != QLocalSocket::UnconnectedState) {
        m_socket->close();
    }
}

void DaemonConnector::stopDaemon() {
    if (m_socket && m_socket->state() != QLocalSocket::UnconnectedState) {
        m_socket->close();
    }
    if (m_daemonProcess && m_daemonProcess->state() != QProcess::NotRunning) {
        setStatusMessage(u"Stopping keyinjectord daemon..."_s);
        m_daemonProcess->terminate();
        if (!m_daemonProcess->waitForFinished(1000)) {
            m_daemonProcess->kill();
        }
        delete m_daemonProcess;
        m_daemonProcess = nullptr;
    }
}

void DaemonConnector::restartService() {
    stopDaemon();
    connectToServer();
}

bool DaemonConnector::sendCommand(keyinjectord::Opcode opcode, int timeoutMs) {
    if (!m_socket || m_socket->state() != QLocalSocket::ConnectedState) {
        setLastError(u"Socket not connected"_s);
        return false;
    }

    const char byte = static_cast<char>(opcode);
    qint64 written = m_socket->write(&byte, 1);
    if (written != 1) {
        setLastError(u"Failed to write opcode to socket"_s);
        return false;
    }
    m_socket->flush();

    if (m_socket->bytesAvailable() == 0) {
        if (!m_socket->waitForReadyRead(timeoutMs)) {
            setLastError(u"Timed out waiting for response from keyinjectord"_s);
            return false;
        }
    }

    char respByte = 0;
    if (m_socket->read(&respByte, 1) != 1) {
        setLastError(u"Failed to read response code from keyinjectord"_s);
        return false;
    }

    auto status = static_cast<keyinjectord::ResponseStatus>(static_cast<uint8_t>(respByte));
    switch (status) {
        case keyinjectord::ResponseStatus::Ok:
            setStatusMessage(u"Command succeeded"_s);
            setLastError({});
            return true;
        case keyinjectord::ResponseStatus::UnknownCmd:
            setLastError(u"Server rejected command: unknown opcode"_s);
            return false;
        case keyinjectord::ResponseStatus::DeviceError:
            setLastError(u"Key injection failed on device"_s);
            return false;
    }

    setLastError(u"Unknown response code from keyinjectord"_s);
    return false;
}

void DaemonConnector::onConnected() {
    setStatusMessage(u"Connected to keyinjectord"_s);
    setLastError({});
    setFatalError(false);
    emit connectedChanged();
}

void DaemonConnector::onDisconnected() {
    setStatusMessage(u"Disconnected from keyinjectord"_s);
    emit connectedChanged();
}

void DaemonConnector::onErrorOccurred(QLocalSocket::LocalSocketError error) {
    Q_UNUSED(error)
    QString msg = m_socket->errorString();
    if (m_daemonProcess && m_daemonProcess->state() == QProcess::NotRunning) {
        QByteArray output = m_daemonProcess->readAll();
        QString procErr = QString::fromUtf8(output).trimmed();
        if (!procErr.isEmpty()) {
            msg = procErr;
            setFatalError(true, msg);
        }
    }
    setLastError(msg);
    emit connectedChanged();
}

void DaemonConnector::setFatalError(bool fatal, const QString& msg) {
    bool changed = false;
    if (m_hasFatalError != fatal) {
        m_hasFatalError = fatal;
        emit hasFatalErrorChanged();
        changed = true;
    }
    if (m_fatalErrorMessage != msg) {
        m_fatalErrorMessage = msg;
        emit fatalErrorMessageChanged();
        changed = true;
    }
    if (changed) {
        emit connectedChanged();
    }
}

void DaemonConnector::setLastError(const QString& error) {
    if (m_lastError != error) {
        m_lastError = error;
        emit lastErrorChanged();
    }
}

void DaemonConnector::setStatusMessage(const QString& message) {
    if (m_statusMessage != message) {
        m_statusMessage = message;
        emit statusMessageChanged();
    }
}
