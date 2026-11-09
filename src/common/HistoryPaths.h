#pragma once

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QStandardPaths>
#include <QString>

// Shared history-file location. The daemon writes the transcription history and
// the GUI only reads or clears it, so both sides must agree on one path.
namespace HistoryPaths {

inline QString historyFilePath() {
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dirPath + QStringLiteral("/history.json");
}

inline void removeHistoryFile() {
    const QString path = historyFilePath();
    const QString dirPath = QFileInfo(path).absolutePath();
    if (QDir(dirPath).exists()) {
        QLockFile lock(path + QStringLiteral(".lock"));
        lock.setStaleLockTime(5000);
        const bool locked = lock.tryLock(2000);
        if (QFile::exists(path)) {
            QFile::remove(path);
        }
        if (locked) {
            lock.unlock();
        }
    } else if (QFile::exists(path)) {
        QFile::remove(path);
    }
}

} // namespace HistoryPaths
