#include "ModelCatalogModel.h"

#include "DaemonClient.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <utility>

using namespace Qt::StringLiterals;

ModelCatalogModel::ModelCatalogModel(QObject* parent)
    : QAbstractListModel(parent) {
    s_instance = this;

    if (auto* client = DaemonClient::instance()) {
        connect(client, &DaemonClient::modelCatalogChanged, this, &ModelCatalogModel::reloadFromDaemon);
        connect(client, &DaemonClient::connectedChanged, this, &ModelCatalogModel::reloadFromDaemon);
    }

    reloadFromDaemon();
}

ModelCatalogModel* ModelCatalogModel::instance() {
    if (!s_instance) {
        s_instance = new ModelCatalogModel(QCoreApplication::instance());
    }
    return s_instance;
}

ModelCatalogModel::~ModelCatalogModel() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

ModelCatalogModel* ModelCatalogModel::create(QQmlEngine* /*qmlEngine*/, QJSEngine* /*jsEngine*/) {
    return instance();
}

int ModelCatalogModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_items.size());
}

QVariant ModelCatalogModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_items.size())) {
        return {};
    }

    const auto& item = m_items.at(index.row());
    switch (role) {
        case IdRole:
            return item.id;
        case NameRole:
            return item.name;
        case FileNameRole:
            return item.fileName;
        case DownloadUrlRole:
            return item.downloadUrl;
        case SizeBytesRole:
            return item.sizeBytes;
        case SizeFormattedRole:
            return item.sizeFormatted;
        case MemoryFormattedRole:
            return item.memoryFormatted;
        case DescriptionRole:
            return item.description;
        case IsInstalledRole:
            return item.isInstalled;
        case IsSelectedRole:
            return item.isSelected;
        case IsDownloadingRole:
            return item.isDownloading;
        case ProgressRole:
            return item.progress;
        case BytesReceivedRole:
            return item.bytesReceived;
        case TotalBytesRole:
            return item.totalBytes;
        case SpeedFormattedRole:
            return item.speedFormatted;
        case InstalledSizeFormattedRole:
            return item.installedSizeFormatted;
        case CanDeleteRole:
            return item.canDelete;
        default:
            return {};
    }
}

QHash<int, QByteArray> ModelCatalogModel::roleNames() const {
    return {{IdRole, "modelId"},
            {NameRole, "name"},
            {FileNameRole, "fileName"},
            {DownloadUrlRole, "downloadUrl"},
            {SizeBytesRole, "sizeBytes"},
            {SizeFormattedRole, "sizeFormatted"},
            {MemoryFormattedRole, "memoryFormatted"},
            {DescriptionRole, "description"},
            {IsInstalledRole, "isInstalled"},
            {IsSelectedRole, "isSelected"},
            {IsDownloadingRole, "isDownloading"},
            {ProgressRole, "progress"},
            {BytesReceivedRole, "bytesReceived"},
            {TotalBytesRole, "totalBytes"},
            {SpeedFormattedRole, "speedFormatted"},
            {InstalledSizeFormattedRole, "installedSizeFormatted"},
            {CanDeleteRole, "canDelete"}};
}

QString ModelCatalogModel::modelsDirectory() {
    if (auto* client = DaemonClient::instance()) {
        const QString directory = client->modelsDirectory();
        if (!directory.isEmpty()) {
            return directory;
        }
    }
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/qtscribe/models"_s;
}

QString ModelCatalogModel::selectedModelName() const {
    for (const auto& item : m_items) {
        if (item.isSelected) {
            return item.name;
        }
    }
    return {};
}

bool ModelCatalogModel::isSelectedModelInstalled() const {
    return std::ranges::any_of(m_items, [](const auto& item) { return item.isSelected; });
}

bool ModelCatalogModel::isDownloadingAny() const {
    return std::ranges::any_of(m_items, [](const auto& item) { return item.isDownloading; });
}

QString ModelCatalogModel::downloadingModelId() const {
    for (const auto& item : m_items) {
        if (item.isDownloading) {
            return item.id;
        }
    }
    return {};
}

qreal ModelCatalogModel::downloadProgress() const {
    for (const auto& item : m_items) {
        if (item.isDownloading) {
            return item.progress;
        }
    }
    return 0.0;
}

QString ModelCatalogModel::downloadSpeedFormatted() const {
    for (const auto& item : m_items) {
        if (item.isDownloading) {
            return item.speedFormatted;
        }
    }
    return {};
}

QString ModelCatalogModel::downloadBytesFormatted() {
    if (auto* client = DaemonClient::instance()) {
        return client->modelDownloadBytes();
    }
    return {};
}

QString ModelCatalogModel::lastError() {
    if (auto* client = DaemonClient::instance()) {
        return client->modelCatalogLastError();
    }
    return {};
}

QString ModelCatalogModel::availableDiskSpaceFormatted() {
    if (auto* client = DaemonClient::instance()) {
        return client->modelDiskSpace();
    }
    return {};
}

void ModelCatalogModel::setSelectedModelId(const QString& id) {
    if (auto* client = DaemonClient::instance()) {
        client->selectModel(id);
    }
}

void ModelCatalogModel::startDownload(const QString& modelId) {
    if (auto* client = DaemonClient::instance()) {
        client->startModelDownload(modelId);
    }
}

void ModelCatalogModel::cancelDownload(const QString& modelId) {
    if (auto* client = DaemonClient::instance()) {
        client->cancelModelDownload(modelId);
    }
}

void ModelCatalogModel::deleteModel(const QString& modelId) {
    if (auto* client = DaemonClient::instance()) {
        client->deleteModel(modelId);
    }
}

void ModelCatalogModel::refreshModelList() {
    if (auto* client = DaemonClient::instance()) {
        client->refreshModelCatalog();
    }
}

void ModelCatalogModel::checkDiskSpace() {
    if (auto* client = DaemonClient::instance()) {
        client->checkModelDiskSpace();
    }
}

// TODO: Stream active download progress via a dedicated D-Bus signal/property
// instead of re-serializing and re-parsing the full model catalog JSON on every tick.
void ModelCatalogModel::reloadFromDaemon() {
    auto* client = DaemonClient::instance();
    applyCatalogJson(client ? client->modelCatalogJson() : QString());
}

void ModelCatalogModel::applyCatalogJson(const QString& json) {
    QList<CatalogItem> items;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8());
    if (document.isArray()) {
        const QJsonArray catalog = document.array();
        items.reserve(catalog.size());
        for (const auto& value : catalog) {
            if (!value.isObject()) {
                continue;
            }
            const QJsonObject object = value.toObject();
            CatalogItem item;
            item.id = object.value(u"modelId"_s).toString();
            if (item.id.isEmpty()) {
                continue;
            }
            item.name = object.value(u"name"_s).toString();
            item.fileName = object.value(u"fileName"_s).toString();
            item.downloadUrl = object.value(u"downloadUrl"_s).toString();
            item.sizeBytes = static_cast<qint64>(object.value(u"sizeBytes"_s).toDouble());
            item.sizeFormatted = object.value(u"sizeFormatted"_s).toString();
            item.memoryFormatted = object.value(u"memoryFormatted"_s).toString();
            item.description = object.value(u"description"_s).toString();
            item.isInstalled = object.value(u"isInstalled"_s).toBool();
            item.isSelected = object.value(u"isSelected"_s).toBool();
            item.isDownloading = object.value(u"isDownloading"_s).toBool();
            item.progress = object.value(u"progress"_s).toDouble();
            item.bytesReceived = static_cast<qint64>(object.value(u"bytesReceived"_s).toDouble());
            item.totalBytes = static_cast<qint64>(object.value(u"totalBytes"_s).toDouble());
            item.speedFormatted = object.value(u"speedFormatted"_s).toString();
            item.installedSizeFormatted = object.value(u"installedSizeFormatted"_s).toString();
            item.canDelete = object.value(u"canDelete"_s).toBool();
            items.append(item);
        }
    }

    const QString previousSelectedName = selectedModelName();
    const bool previousSelectedInstalled = isSelectedModelInstalled();
    const bool previousDownloadingAny = isDownloadingAny();
    const QString previousDownloadingId = downloadingModelId();
    const qreal previousProgress = downloadProgress();
    const QString previousSpeed = downloadSpeedFormatted();

    if (hasSameStructure(items)) {
        applyItems(items);
    } else {
        beginResetModel();
        m_items = std::move(items);
        endResetModel();
    }

    if (previousSelectedName != selectedModelName()) {
        emit selectedModelNameChanged();
    }
    if (previousSelectedInstalled != isSelectedModelInstalled()) {
        emit selectedModelInstalledChanged();
    }

    const bool downloadingAny = isDownloadingAny();
    const QString downloadingId = downloadingModelId();
    if (previousDownloadingAny != downloadingAny || previousDownloadingId != downloadingId) {
        emit isDownloadingAnyChanged();
    }

    const QString downloadBytes = downloadBytesFormatted();
    const bool progressChanged = previousProgress != downloadProgress() || previousSpeed != downloadSpeedFormatted() ||
                                 m_emittedDownloadBytes != downloadBytes;
    if (progressChanged) {
        m_emittedDownloadBytes = downloadBytes;
        emit downloadProgressChanged();
    }

    const QString directory = modelsDirectory();
    if (m_emittedModelsDirectory != directory) {
        m_emittedModelsDirectory = directory;
        emit modelsDirectoryChanged();
    }

    const QString modelError = lastError();
    if (m_emittedLastError != modelError) {
        m_emittedLastError = modelError;
        emit lastErrorChanged();
    }

    const QString diskSpace = availableDiskSpaceFormatted();
    if (m_emittedDiskSpace != diskSpace) {
        m_emittedDiskSpace = diskSpace;
        emit diskSpaceChanged();
    }
}

void ModelCatalogModel::applyItems(const QList<CatalogItem>& items) {
    for (int row = 0; row < static_cast<int>(items.size()); ++row) {
        const CatalogItem& incoming = items.at(row);
        const CatalogItem& existing = m_items.at(row);
        if (existing == incoming) {
            continue;
        }

        const QList<int> roles = changedRoles(existing, incoming);
        m_items[row] = incoming;

        const QModelIndex modelIndex = index(row, 0);
        emit dataChanged(modelIndex, modelIndex, roles);
    }
}

bool ModelCatalogModel::hasSameStructure(const QList<CatalogItem>& items) const {
    if (items.size() != m_items.size()) {
        return false;
    }
    for (int row = 0; row < static_cast<int>(items.size()); ++row) {
        if (items.at(row).id != m_items.at(row).id) {
            return false;
        }
    }
    return true;
}

QList<int> ModelCatalogModel::changedRoles(const CatalogItem& previous, const CatalogItem& current) {
    QList<int> roles;
    if (previous.id != current.id) {
        roles.append(IdRole);
    }
    if (previous.name != current.name) {
        roles.append(NameRole);
    }
    if (previous.fileName != current.fileName) {
        roles.append(FileNameRole);
    }
    if (previous.downloadUrl != current.downloadUrl) {
        roles.append(DownloadUrlRole);
    }
    if (previous.sizeBytes != current.sizeBytes) {
        roles.append(SizeBytesRole);
    }
    if (previous.sizeFormatted != current.sizeFormatted) {
        roles.append(SizeFormattedRole);
    }
    if (previous.memoryFormatted != current.memoryFormatted) {
        roles.append(MemoryFormattedRole);
    }
    if (previous.description != current.description) {
        roles.append(DescriptionRole);
    }
    if (previous.isInstalled != current.isInstalled) {
        roles.append(IsInstalledRole);
    }
    if (previous.isSelected != current.isSelected) {
        roles.append(IsSelectedRole);
    }
    if (previous.isDownloading != current.isDownloading) {
        roles.append(IsDownloadingRole);
    }
    if (previous.progress != current.progress) {
        roles.append(ProgressRole);
    }
    if (previous.bytesReceived != current.bytesReceived) {
        roles.append(BytesReceivedRole);
    }
    if (previous.totalBytes != current.totalBytes) {
        roles.append(TotalBytesRole);
    }
    if (previous.speedFormatted != current.speedFormatted) {
        roles.append(SpeedFormattedRole);
    }
    if (previous.installedSizeFormatted != current.installedSizeFormatted) {
        roles.append(InstalledSizeFormattedRole);
    }
    if (previous.canDelete != current.canDelete) {
        roles.append(CanDeleteRole);
    }
    return roles;
}
