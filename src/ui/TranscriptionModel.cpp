#include "TranscriptionModel.h"

#include "ClipboardManager.h"
#include "DictationCoordinator.h"

#include "DaemonClient.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QStandardPaths>
#include <QUuid>

using namespace Qt::StringLiterals;

TranscriptionSourceModel::TranscriptionSourceModel(QObject* parent)
    : QAbstractListModel(parent) { }

int TranscriptionSourceModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_records.size());
}

QVariant TranscriptionSourceModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_records.size()) {
        return QVariant();
    }

    const auto& record = m_records.at(index.row());
    switch (static_cast<Roles>(role)) {
        case Roles::IdRole:
            return record.id;
        case Roles::TimestampRole:
            return record.timestamp;
        case Roles::FormattedTimestampRole:
            return record.timestamp.toLocalTime().toString(u"MMM d, hh:mm:ss AP"_s);
        case Roles::TextRole:
            return record.text;
    }
    return QVariant();
}

QHash<int, QByteArray> TranscriptionSourceModel::roleNames() const {
    return {{static_cast<int>(Roles::IdRole), "id"},
            {static_cast<int>(Roles::TimestampRole), "timestamp"},
            {static_cast<int>(Roles::FormattedTimestampRole), "formattedTimestamp"},
            {static_cast<int>(Roles::TextRole), "text"}};
}

int TranscriptionSourceModel::count() const {
    return static_cast<int>(m_records.size());
}

void TranscriptionSourceModel::addRecord(const TranscriptionRecord& record) {
    if (record.text.trimmed().isEmpty()) {
        return;
    }

    beginInsertRows(QModelIndex(), 0, 0);
    m_records.prepend(record);
    endInsertRows();

    if (m_records.size() > kMaxHistoryRecords) {
        int removeCount = m_records.size() - kMaxHistoryRecords;
        beginRemoveRows(QModelIndex(), kMaxHistoryRecords, m_records.size() - 1);
        while (removeCount-- > 0) {
            m_records.removeLast();
        }
        endRemoveRows();
    }

    emit countChanged();
}

void TranscriptionSourceModel::addRecord(const QString& text) {
    if (text.trimmed().isEmpty()) {
        return;
    }

    addRecord(TranscriptionRecord {.id = QUuid::createUuid().toString(QUuid::WithoutBraces),
                                   .timestamp = QDateTime::currentDateTimeUtc(),
                                   .text = text});
}

void TranscriptionSourceModel::setRecords(const QList<TranscriptionRecord>& records) {
    beginResetModel();
    m_records = records;
    if (m_records.size() > kMaxHistoryRecords) {
        m_records = m_records.sliced(0, kMaxHistoryRecords);
    }
    endResetModel();
    emit countChanged();
}

void TranscriptionSourceModel::clearAll() {
    if (m_records.isEmpty()) {
        return;
    }
    beginResetModel();
    m_records.clear();
    endResetModel();
    emit countChanged();
}

TranscriptionModel::TranscriptionModel(QObject* parent)
    : QSortFilterProxyModel(parent)
    , m_sourceModel(this) {
    setSourceModel(&m_sourceModel);
    setFilterCaseSensitivity(Qt::CaseInsensitive);

    connect(&m_sourceModel, &TranscriptionSourceModel::countChanged, this, [this]() {
        emit countChanged();
        emit totalCountChanged();
    });
    connect(&m_sourceModel, &TranscriptionSourceModel::rowsInserted, this, &TranscriptionModel::countChanged);
    connect(&m_sourceModel, &TranscriptionSourceModel::rowsRemoved, this, &TranscriptionModel::countChanged);
    connect(&m_sourceModel, &TranscriptionSourceModel::modelReset, this, &TranscriptionModel::countChanged);

    loadHistory();

    if (auto* client = DaemonClient::instance()) {
        connect(client, &DaemonClient::transcriptionFinished, this, &TranscriptionModel::addRecord);
        connect(client, &DaemonClient::historyCleared, this, [this]() { m_sourceModel.clearAll(); });
    }
}

void TranscriptionModel::loadHistory(const QString& filePath) {
    QString resolvedPath = filePath;
    if (resolvedPath.isEmpty()) {
        if (QStandardPaths::isTestModeEnabled()) {
            return;
        }
        resolvedPath = DictationCoordinator::historyFilePath();
    }

    QLockFile lock(resolvedPath + u".lock"_s);
    lock.setStaleLockTime(5000);
    const bool locked = lock.tryLock(2000);

    QFile file(resolvedPath);
    if (file.exists()) {
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (locked) {
            lock.unlock();
        }
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (locked) {
        lock.unlock();
    }
    if (!doc.isArray()) {
        return;
    }

    QList<TranscriptionRecord> loadedRecords;
    const QJsonArray arr = doc.array();
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
        loadedRecords.append(TranscriptionRecord {.id = id, .timestamp = ts, .text = text});
    }

    m_sourceModel.setRecords(loadedRecords);
}

int TranscriptionModel::count() const {
    return rowCount();
}

int TranscriptionModel::totalCount() const {
    return m_sourceModel.rowCount();
}

int TranscriptionModel::filteredCount() const {
    return rowCount();
}

QString TranscriptionModel::searchFilter() const {
    return m_searchFilter;
}

void TranscriptionModel::setSearchFilter(const QString& filter) {
    if (m_searchFilter != filter) {
        beginFilterChange();
        m_searchFilter = filter;
        endFilterChange();
        emit searchFilterChanged();
        emit countChanged();
    }
}

void TranscriptionModel::addRecord(const QString& text) {
    m_sourceModel.addRecord(text);
}

void TranscriptionModel::clearAll() {
    m_sourceModel.clearAll();
    auto* client = DaemonClient::instance();
    if (client && client->isConnected()) {
        client->clearHistory();
    } else {
        DictationCoordinator::removeHistoryFile();
    }
}

void TranscriptionModel::copyToClipboard(const QString& text) {
    ClipboardManager clipboardManager;
    clipboardManager.setText(text, false);
}

bool TranscriptionModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    if (m_searchFilter.isEmpty()) {
        return true;
    }
    QModelIndex index = m_sourceModel.index(sourceRow, 0, sourceParent);
    QString text = m_sourceModel.data(index, static_cast<int>(Roles::TextRole)).toString();
    return text.contains(m_searchFilter, Qt::CaseInsensitive);
}
