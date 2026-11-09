#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

// Read-only view of the daemon-owned Whisper model catalog.
//
// The daemon discovers installed files, performs downloads, and deletes models.
// The GUI only mirrors the catalog it publishes over D-Bus.
class ModelCatalogModel : public QAbstractListModel {
    Q_OBJECT
    QML_NAMED_ELEMENT(ModelCatalog)
    QML_SINGLETON

    Q_PROPERTY(QString modelsDirectory READ modelsDirectory NOTIFY modelsDirectoryChanged FINAL)
    Q_PROPERTY(QString selectedModelName READ selectedModelName NOTIFY selectedModelNameChanged FINAL)
    Q_PROPERTY(bool isSelectedModelInstalled READ isSelectedModelInstalled NOTIFY selectedModelInstalledChanged FINAL)
    Q_PROPERTY(bool isDownloadingAny READ isDownloadingAny NOTIFY isDownloadingAnyChanged FINAL)
    Q_PROPERTY(QString downloadingModelId READ downloadingModelId NOTIFY isDownloadingAnyChanged FINAL)
    Q_PROPERTY(qreal downloadProgress READ downloadProgress NOTIFY downloadProgressChanged FINAL)
    Q_PROPERTY(QString downloadSpeedFormatted READ downloadSpeedFormatted NOTIFY downloadProgressChanged FINAL)
    Q_PROPERTY(QString downloadBytesFormatted READ downloadBytesFormatted NOTIFY downloadProgressChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(QString availableDiskSpaceFormatted READ availableDiskSpaceFormatted NOTIFY diskSpaceChanged FINAL)

public:
    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        FileNameRole,
        DownloadUrlRole,
        SizeBytesRole,
        SizeFormattedRole,
        MemoryFormattedRole,
        DescriptionRole,
        IsInstalledRole,
        IsSelectedRole,
        IsDownloadingRole,
        ProgressRole,
        BytesReceivedRole,
        TotalBytesRole,
        SpeedFormattedRole,
        InstalledSizeFormattedRole,
        CanDeleteRole
    };
    Q_ENUM(Roles)

    explicit ModelCatalogModel(QObject* parent = nullptr);
    ~ModelCatalogModel() override;

    static ModelCatalogModel* instance();
    static ModelCatalogModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    // Applies an authoritative catalog snapshot.
    //
    // Download progress and selection changes only patch the affected rows so QML
    // delegates survive high-frequency updates. The model is reset only when the
    // catalog structure (row count or model ids) actually changes.
    void applyCatalogJson(const QString& json);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] static QString modelsDirectory();
    [[nodiscard]] QString selectedModelName() const;
    [[nodiscard]] bool isSelectedModelInstalled() const;
    [[nodiscard]] bool isDownloadingAny() const;
    [[nodiscard]] QString downloadingModelId() const;
    [[nodiscard]] qreal downloadProgress() const;
    [[nodiscard]] QString downloadSpeedFormatted() const;
    [[nodiscard]] static QString downloadBytesFormatted();
    [[nodiscard]] static QString lastError();
    [[nodiscard]] static QString availableDiskSpaceFormatted();

    Q_INVOKABLE static void setSelectedModelId(const QString& id);
    Q_INVOKABLE static void startDownload(const QString& modelId);
    Q_INVOKABLE static void cancelDownload(const QString& modelId = QString());
    Q_INVOKABLE static void deleteModel(const QString& modelId);
    Q_INVOKABLE static void refreshModelList();
    Q_INVOKABLE static void checkDiskSpace();

signals:
    void modelsDirectoryChanged();
    void selectedModelNameChanged();
    void selectedModelInstalledChanged();
    void isDownloadingAnyChanged();
    void downloadProgressChanged();
    void lastErrorChanged();
    void diskSpaceChanged();

private:
    struct CatalogItem {
        QString id;
        QString name;
        QString fileName;
        QString downloadUrl;
        qint64 sizeBytes = 0;
        QString sizeFormatted;
        QString memoryFormatted;
        QString description;
        bool isInstalled = false;
        bool isSelected = false;
        bool isDownloading = false;
        qreal progress = 0.0;
        qint64 bytesReceived = 0;
        qint64 totalBytes = 0;
        QString speedFormatted;
        QString installedSizeFormatted;
        bool canDelete = false;

        bool operator==(const CatalogItem&) const = default;
    };

    void reloadFromDaemon();
    void applyItems(const QList<CatalogItem>& items);
    [[nodiscard]] bool hasSameStructure(const QList<CatalogItem>& items) const;
    [[nodiscard]] static QList<int> changedRoles(const CatalogItem& previous, const CatalogItem& current);

    static inline ModelCatalogModel* s_instance = nullptr;
    QList<CatalogItem> m_items;

    QString m_emittedModelsDirectory;
    QString m_emittedDownloadBytes;
    QString m_emittedLastError;
    QString m_emittedDiskSpace;
};
