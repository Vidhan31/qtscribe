#pragma once

#include "ModelDownloader.h"

#include <QAbstractListModel>
#include <QList>
#include <QQmlEngine>
#include <QString>

#include <cstdint>
#include <memory>
#include <optional>

struct WhisperModelItem {
    QString id = {};
    QString name = {};
    QString fileName = {};
    QString downloadUrl = {};
    qint64 sizeBytes = 0;
    QString sizeFormatted = {};
    QString memoryFormatted = {};
    QString description = {};
    bool isInstalled = false;
    qint64 installedSizeBytes = 0;
    QString installedSizeFormatted = {};

    bool isDownloading = false;
    qreal progress = 0.0;
    qint64 bytesReceived = 0;
    qint64 totalBytes = 0;
    QString speedFormatted = {};
};

class WhisperModelManager : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString modelsDirectory READ modelsDirectory CONSTANT FINAL)
    Q_PROPERTY(QString selectedModelId READ selectedModelId WRITE setSelectedModelId NOTIFY selectedModelChanged FINAL)
    Q_PROPERTY(QString selectedModelPath READ selectedModelPath NOTIFY selectedModelChanged FINAL)
    Q_PROPERTY(QString selectedModelName READ selectedModelName NOTIFY selectedModelChanged FINAL)
    Q_PROPERTY(bool isSelectedModelInstalled READ isSelectedModelInstalled NOTIFY selectedModelChanged FINAL)
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

    explicit WhisperModelManager(QObject* parent = nullptr);
    explicit WhisperModelManager(std::unique_ptr<ModelDownloader> downloader, QObject* parent = nullptr);
    ~WhisperModelManager() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString modelsDirectory() const;
    void setModelsDirectory(const QString& path);

    const QList<WhisperModelItem>& models() const;
    std::optional<WhisperModelItem> model(const QString& modelId) const;
    int modelCount() const;
    int findModelIndex(const QString& modelId) const;

    QString selectedModelId() const;
    QString selectedModelPath() const;
    QString selectedModelName() const;
    virtual bool isSelectedModelInstalled() const;
    bool hasAnyModelInstalled() const;
    QString firstInstalledModelId() const;

    bool isDownloadingAny() const;
    QString downloadingModelId() const;
    qreal downloadProgress() const;
    QString downloadSpeedFormatted() const;
    QString downloadBytesFormatted() const;
    QString lastError() const;
    qint64 availableDiskSpace() const;
    QString availableDiskSpaceFormatted() const;

    bool isModelInstalled(const QString& modelId) const;
    QString getModelPath(const QString& modelId) const;

    static QString formatBytes(qint64 bytes);

    Q_INVOKABLE void setSelectedModelId(const QString& id);
    Q_INVOKABLE bool startDownload(const QString& modelId);
    Q_INVOKABLE void cancelDownload(const QString& modelId = QString());
    Q_INVOKABLE bool deleteModel(const QString& modelId);
    Q_INVOKABLE void refreshModelList();
    Q_INVOKABLE void checkDiskSpace();
    void scanInstalledModels();
    void cleanupOrphanedPartFiles();

signals:
    void modelsChanged();
    void selectedModelChanged();
    void modelStatusChanged();
    void isDownloadingAnyChanged();
    void downloadProgressChanged();
    void lastErrorChanged();
    void diskSpaceChanged();
    void modelDownloadFinished(const QString& modelId, bool success, const QString& error);

private:
    void initPresets();
    void setupDownloaderConnections();
    void setLastError(const QString& error);

    std::unique_ptr<ModelDownloader> m_downloader;
    QList<WhisperModelItem> m_models;
    QString m_customModelsDirectory;
    QString m_selectedModelId;
    QString m_lastError;
    qint64 m_availableDiskSpace = 0;
};
