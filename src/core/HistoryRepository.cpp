#include "HistoryRepository.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QStandardPaths>
#include <QUuid>

using namespace Qt::StringLiterals;

HistoryRepository::HistoryRepository(QString customFilePath, QObject* parent)
    : QObject(parent)
    , m_customFilePath(std::move(customFilePath)) { }

QString HistoryRepository::defaultHistoryFilePath() {
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dirPath + u"/history.json"_s;
}

void HistoryRepository::removeHistoryFile() {
    HistoryRepository repo;
    repo.clearHistory();
}

QString HistoryRepository::filePath() const {
    return effectiveFilePath();
}

void HistoryRepository::setFilePath(const QString& path) {
    m_customFilePath = path;
}

QString HistoryRepository::effectiveFilePath() const {
    return m_customFilePath.isEmpty() ? defaultHistoryFilePath() : m_customFilePath;
}

QList<HistoryRecord> HistoryRepository::loadRecords() const {
    const QString path = effectiveFilePath();
    QLockFile lock(path + u".lock"_s);
    lock.setStaleLockTime(5000);
    const bool locked = lock.tryLock(2000);

    QFile file(path);
    if (file.exists()) {
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (locked) {
            lock.unlock();
        }
        return {};
    }

    const QByteArray data = file.readAll();
    file.close();
    if (locked) {
        lock.unlock();
    }

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        return {};
    }

    QList<HistoryRecord> records;
    const QJsonArray arr = doc.array();
    records.reserve(arr.size());
    for (const auto val : arr) {
        if (!val.isObject()) {
            continue;
        }
        const QJsonObject obj = val.toObject();
        const QString text = obj.value(u"text"_s).toString();
        if (text.trimmed().isEmpty()) {
            continue;
        }
        QString id = obj.value(u"id"_s).toString();
        if (id.isEmpty()) {
            id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        }
        const QString tsStr = obj.value(u"timestamp"_s).toString();
        QDateTime ts = QDateTime::fromString(tsStr, Qt::ISODate);
        if (!ts.isValid()) {
            ts = QDateTime::currentDateTimeUtc();
        }
        records.append(HistoryRecord {.id = id, .timestamp = ts, .text = text});
    }

    return records;
}

QString HistoryRepository::loadRecordsJson() const {
    const QString path = effectiveFilePath();
    QLockFile lock(path + u".lock"_s);
    lock.setStaleLockTime(5000);
    const bool locked = lock.tryLock(2000);

    QFile file(path);
    if (file.exists()) {
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (locked) {
            lock.unlock();
        }
        return u"[]"_s;
    }

    const QByteArray data = file.readAll();
    file.close();
    if (locked) {
        lock.unlock();
    }

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        return u"[]"_s;
    }

    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

bool HistoryRepository::appendRecord(const QString& text) {
    if (text.trimmed().isEmpty()) {
        return false;
    }
    const HistoryRecord record {
        .id = QUuid::createUuid().toString(QUuid::WithoutBraces),
        .timestamp = QDateTime::currentDateTimeUtc(),
        .text = text,
    };
    return appendRecord(record);
}

bool HistoryRepository::appendRecord(const HistoryRecord& record) {
    if (record.text.trimmed().isEmpty()) {
        return false;
    }

    QList<HistoryRecord> records = loadRecords();
    records.prepend(record);
    while (records.size() > kMaxHistoryRecords) {
        records.removeLast();
    }

    const bool ok = saveRecords(records);
    if (ok) {
        emit historyChanged();
    }
    return ok;
}

bool HistoryRepository::saveRecords(const QList<HistoryRecord>& records) {
    const QString path = effectiveFilePath();
    const QString dirPath = QFileInfo(path).absolutePath();
    QDir().mkpath(dirPath);
    QFile::setPermissions(dirPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);

    QLockFile lock(path + u".lock"_s);
    lock.setStaleLockTime(5000);
    const bool locked = lock.tryLock(2000);

    QFile file(path);
    if (file.exists()) {
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (locked) {
            lock.unlock();
        }
        return false;
    }
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);

    QJsonArray arr;
    for (const auto& rec : records) {
        QJsonObject obj;
        obj[u"id"_s] = rec.id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : rec.id;
        obj[u"timestamp"_s] = rec.timestamp.isValid() ? rec.timestamp.toString(Qt::ISODate)
                                                      : QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        obj[u"text"_s] = rec.text;
        arr.append(obj);
    }
    file.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    file.close();

    if (locked) {
        lock.unlock();
    }
    return true;
}

void HistoryRepository::clearHistory() {
    const QString path = effectiveFilePath();
    const QString dirPath = QFileInfo(path).absolutePath();
    if (QDir(dirPath).exists()) {
        QLockFile lock(path + u".lock"_s);
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
    emit historyCleared();
    emit historyChanged();
}
