#include "ClipboardManager.h"

#include <QCoreApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

#include <utility>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

ClipboardManager::ClipboardManager(QObject* parent)
    : QObject(parent)
    , m_restoreTimer(new QTimer(this)) {
    m_restoreTimer->setSingleShot(true);
    connect(m_restoreTimer, &QTimer::timeout, this, [this]() {
        if (m_hasActiveBackup) {
            restore(m_savedClipboardText, m_hadClipboardContent);
            m_hasActiveBackup = false;
            m_savedClipboardText.clear();
        }
    });
}

ClipboardManager::~ClipboardManager() {
    if (m_hasActiveBackup) {
        m_restoreTimer->stop();
        restore(m_savedClipboardText, m_hadClipboardContent);
    }
}

QString ClipboardManager::lastError() const {
    return m_lastError;
}

void ClipboardManager::setLastError(const QString& error) const {
    if (m_lastError != error) {
        m_lastError = error;
        emit const_cast<ClipboardManager*>(this)->lastErrorChanged(m_lastError);
    }
}

bool ClipboardManager::hasActiveBackup() const {
    return m_hasActiveBackup;
}

void ClipboardManager::cancelPendingRestore() {
    if (m_restoreTimer->isActive()) {
        m_restoreTimer->stop();
    }
    m_hasActiveBackup = false;
    m_savedClipboardText.clear();
}

void ClipboardManager::scheduleRestore(const QString& backupText, bool hadContent, std::chrono::milliseconds delay) {
    m_savedClipboardText = backupText;
    m_hadClipboardContent = hadContent;
    m_hasActiveBackup = true;
    m_restoreTimer->start(delay);
}

QString ClipboardManager::wlToolPath(const QString& toolName) const {
    return QStandardPaths::findExecutable(toolName);
}

QString ClipboardManager::backupText() const {
    const QString wlPaste = wlToolPath(u"wl-paste"_s);
    if (wlPaste.isEmpty()) {
        setLastError(
            u"wl-paste not found in PATH. Please install the 'wl-clipboard' package from your system package manager."_s);
        return {};
    }
    QProcess proc;
    proc.start(wlPaste, {u"--no-newline"_s, u"--type"_s, u"text/plain"_s});
    if (!proc.waitForFinished(500) || proc.exitCode() != 0) {
        return {};
    }
    return QString::fromUtf8(proc.readAllStandardOutput());
}

bool ClipboardManager::setText(const QString& text, bool sensitive) {
    const QString wlCopy = wlToolPath(u"wl-copy"_s);
    if (wlCopy.isEmpty()) {
        setLastError(
            u"wl-copy not found in PATH. Please install the 'wl-clipboard' package from your system package manager."_s);
        return false;
    }

    auto runWlCopy = [&](bool withSensitive) -> std::pair<bool, int> {
        QProcess proc;
        QStringList args;
        if (withSensitive) {
            args.append(u"--sensitive"_s);
        }
        args.append(u"--type"_s);
        args.append(u"text/plain"_s);
        proc.start(wlCopy, args);
        if (!proc.waitForStarted(1000)) {
            setLastError(u"Failed to start wl-copy (%1). Is wl-clipboard built/installed?"_s.arg(wlCopy));
            return {false, -1};
        }
        proc.write(text.toUtf8());
        proc.closeWriteChannel();
        if (!proc.waitForFinished(1000)) {
            setLastError(u"wl-copy operation timed out"_s);
            return {false, -1};
        }
        if (proc.exitCode() != 0) {
            return {false, proc.exitCode()};
        }
        return {true, 0};
    };

    auto [ok, exitCode] = runWlCopy(sensitive);
    if (!ok) {
        // Fallback robustness for --sensitive:
        // If wl-copy --sensitive exits with a non-zero exit code (e.g. wl-clipboard < 2.2.1), retry without --sensitive
        if (sensitive && exitCode != -1) {
            auto [retryOk, retryExitCode] = runWlCopy(false);
            if (retryOk) {
                setLastError({});
                return true;
            }
            setLastError(u"wl-copy failed with exit code %1"_s.arg(retryExitCode));
            return false;
        }
        if (exitCode != -1) {
            setLastError(u"wl-copy failed with exit code %1"_s.arg(exitCode));
        }
        return false;
    }

    setLastError({});
    return true;
}

void ClipboardManager::restore(const QString& backup, bool hadContent) {
    if (!hadContent) {
        const QString wlCopy = wlToolPath(u"wl-copy"_s);
        if (wlCopy.isEmpty()) {
            setLastError(
                u"wl-copy not found in PATH. Please install the 'wl-clipboard' package from your system package manager."_s);
            return;
        }
        QProcess proc;
        proc.start(wlCopy, {u"--clear"_s});
        proc.waitForFinished(500);
        return;
    }
    setText(backup, /*sensitive=*/false);
}
