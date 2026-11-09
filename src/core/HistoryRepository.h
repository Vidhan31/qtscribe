#pragma once

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>

struct HistoryRecord {
    QString id;
    QDateTime timestamp;
    QString text;

    bool operator==(const HistoryRecord& other) const noexcept {
        return id == other.id && timestamp == other.timestamp && text == other.text;
    }
};

class HistoryRepository : public QObject {
    Q_OBJECT

public:
    static constexpr int kMaxHistoryRecords = 200;

    explicit HistoryRepository(QString customFilePath = QString(), QObject* parent = nullptr);
    ~HistoryRepository() override = default;

    [[nodiscard]] static QString defaultHistoryFilePath();
    static void removeHistoryFile();

    [[nodiscard]] QString filePath() const;
    void setFilePath(const QString& path);

    [[nodiscard]] QList<HistoryRecord> loadRecords() const;
    [[nodiscard]] QString loadRecordsJson() const;

    bool appendRecord(const QString& text);
    bool appendRecord(const HistoryRecord& record);
    bool saveRecords(const QList<HistoryRecord>& records);
    void clearHistory();

signals:
    void historyChanged();
    void historyCleared();

private:
    [[nodiscard]] QString effectiveFilePath() const;

    QString m_customFilePath;
};
