#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QQmlEngine>
#include <QString>

#include <cstdint>

struct CloudProviderItem {
    QString id;
    QString name;
    QString description;
    QString iconSource;
    QString componentSource;
    QString websiteUrl;
    bool isDefault {false};
};

class CloudProviderModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString activeProviderId READ activeProviderId WRITE setActiveProviderId NOTIFY activeProviderChanged FINAL)
    Q_PROPERTY(int activeProviderIndex READ activeProviderIndex WRITE setActiveProviderIndex NOTIFY activeProviderChanged FINAL)
    Q_PROPERTY(QString activeProviderName READ activeProviderName NOTIFY activeProviderChanged FINAL)
    Q_PROPERTY(QString activeProviderDescription READ activeProviderDescription NOTIFY activeProviderChanged FINAL)
    Q_PROPERTY(QString activeProviderComponent READ activeProviderComponent NOTIFY activeProviderChanged FINAL)
    Q_PROPERTY(QString activeProviderWebsiteUrl READ activeProviderWebsiteUrl NOTIFY activeProviderChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        DescriptionRole,
        IconSourceRole,
        ComponentSourceRole,
        WebsiteUrlRole,
        IsActiveRole
    };
    Q_ENUM(Roles)

    explicit CloudProviderModel(QObject* parent = nullptr);
    ~CloudProviderModel() override = default;

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString activeProviderId() const;
    void setActiveProviderId(const QString& id);

    [[nodiscard]] int activeProviderIndex() const;
    void setActiveProviderIndex(int index);

    [[nodiscard]] QString activeProviderName() const;
    [[nodiscard]] QString activeProviderDescription() const;
    [[nodiscard]] QString activeProviderComponent() const;
    [[nodiscard]] QString activeProviderWebsiteUrl() const;
    [[nodiscard]] int count() const;

    Q_INVOKABLE [[nodiscard]] int indexOfProvider(const QString& id) const;
    Q_INVOKABLE [[nodiscard]] QString providerIdAt(int index) const;

signals:
    void activeProviderChanged();
    void countChanged();

private:
    void initializeProviders();
    void loadSettings();
    void saveSettings();

    QString m_activeProviderId {QStringLiteral("groq")};
    QList<CloudProviderItem> m_providers;
};
