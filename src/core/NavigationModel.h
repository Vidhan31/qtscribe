#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QQmlEngine>
#include <QString>

#include <cstdint>

struct NavigationItem {
    QString section;
    QString title;
    QString iconSource;
    QString sectionId;
    bool isFirstInSection {false};
};

class NavigationModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool isCloud READ isCloud WRITE setIsCloud NOTIFY isCloudChanged FINAL)
    Q_PROPERTY(bool isOnline READ isCloud WRITE setIsCloud NOTIFY isCloudChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum Roles { SectionRole = Qt::UserRole + 1, TitleRole, IconSourceRole, SectionIdRole, IsFirstInSectionRole };
    Q_ENUM(Roles)

    explicit NavigationModel(QObject* parent = nullptr);
    ~NavigationModel() override = default;

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] bool isCloud() const;
    void setIsCloud(bool cloud);

    [[nodiscard]] bool isOnline() const;
    void setIsOnline(bool online);

    [[nodiscard]] int count() const;

signals:
    void isCloudChanged();
    void isOnlineChanged();
    void countChanged();

private:
    void rebuildItems();

    bool m_isCloud {false};
    QList<NavigationItem> m_items;
};
