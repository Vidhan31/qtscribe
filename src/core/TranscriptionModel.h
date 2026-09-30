#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QQmlEngine>
#include <QSortFilterProxyModel>
#include <QString>

#include <cstdint>

struct TranscriptionRecord {
    QString id;
    QDateTime timestamp;
    QString text;
};

class TranscriptionSourceModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum class Roles : std::uint16_t { IdRole = Qt::UserRole + 1, TimestampRole, FormattedTimestampRole, TextRole };
    Q_ENUM(Roles)

    explicit TranscriptionSourceModel(QObject* parent = nullptr);
    ~TranscriptionSourceModel() override = default;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    void addRecord(const QString& text);
    void clearAll();

signals:
    void countChanged();

private:
    static constexpr int kMaxHistoryRecords = 200;
    QList<TranscriptionRecord> m_records;
};

class TranscriptionModel : public QSortFilterProxyModel {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged FINAL)
    Q_PROPERTY(int filteredCount READ filteredCount NOTIFY countChanged FINAL)
    Q_PROPERTY(QString searchFilter READ searchFilter WRITE setSearchFilter NOTIFY searchFilterChanged FINAL)

public:
    using Roles = TranscriptionSourceModel::Roles;

    explicit TranscriptionModel(QObject* parent = nullptr);
    ~TranscriptionModel() override = default;

    int count() const;
    int totalCount() const;
    int filteredCount() const;
    QString searchFilter() const;

    Q_INVOKABLE void setSearchFilter(const QString& filter);
    Q_INVOKABLE void addRecord(const QString& text);
    Q_INVOKABLE void clearAll();
    Q_INVOKABLE void copyToClipboard(const QString& text);

signals:
    void countChanged();
    void totalCountChanged();
    void searchFilterChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    TranscriptionSourceModel m_sourceModel;
    QString m_searchFilter;
};
