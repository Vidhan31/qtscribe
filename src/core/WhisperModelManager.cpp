#include "WhisperModelManager.h"

#include "LoggingCategories.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QStorageInfo>

#include <algorithm>
#include <ranges>

using namespace Qt::StringLiterals;

namespace {
const auto kDefaultSelectedModel = u"tiny.en"_s;
const auto kHfBaseUrl = u"https://huggingface.co/ggerganov/whisper.cpp/resolve/main/"_s;
} // namespace

WhisperModelManager::WhisperModelManager(QObject* parent)
    : WhisperModelManager(std::make_unique<ModelDownloader>(), parent) { }

WhisperModelManager::WhisperModelManager(std::unique_ptr<ModelDownloader> downloader, QObject* parent)
    : QAbstractListModel(parent)
    , m_downloader(std::move(downloader)) {
    if (!m_downloader) {
        m_downloader = std::make_unique<ModelDownloader>(this);
    } else {
        m_downloader->setParent(this);
    }

    initPresets();

    QDir().mkpath(modelsDirectory());
    cleanupOrphanedPartFiles();
    scanInstalledModels();
    checkDiskSpace();
    setupDownloaderConnections();

    QSettings settings;
    m_selectedModelId = settings.value(u"Whisper/SelectedModel"_s, QString()).toString();
    if (m_selectedModelId.isEmpty() || !isModelInstalled(m_selectedModelId)) {
        const QString installedId = firstInstalledModelId();
        if (!installedId.isEmpty()) {
            m_selectedModelId = installedId;
        } else if (m_selectedModelId.isEmpty()) {
            m_selectedModelId = kDefaultSelectedModel;
        }
    }
    if (findModelIndex(m_selectedModelId) < 0 && !m_models.isEmpty()) {
        m_selectedModelId = m_models.first().id;
    }
}

WhisperModelManager::~WhisperModelManager() = default;

void WhisperModelManager::initPresets() {
    m_models = {
        WhisperModelItem {.id = u"tiny.en"_s,
                          .name = tr("Tiny (English)"),
                          .fileName = u"ggml-tiny.en.bin"_s,
                          .downloadUrl = kHfBaseUrl + u"ggml-tiny.en.bin"_s,
                          .sizeBytes = 77704715,
                          .sizeFormatted = u"~74 MiB"_s,
                          .memoryFormatted = u"~273 MB RAM/VRAM"_s,
                          .description =
                              tr("Fastest English dictation with lowest resource usage and minimal latency.")},
        WhisperModelItem {.id = u"tiny"_s,
                          .name = tr("Tiny (Multilingual)"),
                          .fileName = u"ggml-tiny.bin"_s,
                          .downloadUrl = kHfBaseUrl + u"ggml-tiny.bin"_s,
                          .sizeBytes = 77691713,
                          .sizeFormatted = u"~74 MiB"_s,
                          .memoryFormatted = u"~273 MB RAM/VRAM"_s,
                          .description =
                              tr("Ultra-fast multilingual dictation across 99+ languages with minimal memory usage.")},
        WhisperModelItem {.id = u"base.en"_s,
                          .name = tr("Base (English)"),
                          .fileName = u"ggml-base.en.bin"_s,
                          .downloadUrl = kHfBaseUrl + u"ggml-base.en.bin"_s,
                          .sizeBytes = 147964211,
                          .sizeFormatted = u"~141 MiB"_s,
                          .memoryFormatted = u"~388 MB RAM/VRAM"_s,
                          .description =
                              tr("Fast English transcription with improved accuracy over Tiny for general speech.")},
        WhisperModelItem {.id = u"base"_s,
                          .name = tr("Base (Multilingual)"),
                          .fileName = u"ggml-base.bin"_s,
                          .downloadUrl = kHfBaseUrl + u"ggml-base.bin"_s,
                          .sizeBytes = 147951465,
                          .sizeFormatted = u"~141 MiB"_s,
                          .memoryFormatted = u"~388 MB RAM/VRAM"_s,
                          .description =
                              tr("Fast multilingual transcription with solid baseline recognition accuracy.")},
        WhisperModelItem {.id = u"small.en"_s,
                          .name = tr("Small (English)"),
                          .fileName = u"ggml-small.en.bin"_s,
                          .downloadUrl = kHfBaseUrl + u"ggml-small.en.bin"_s,
                          .sizeBytes = 487614201,
                          .sizeFormatted = u"~465 MiB"_s,
                          .memoryFormatted = u"~852 MB RAM/VRAM"_s,
                          .description =
                              tr("High accuracy English transcription; recommended sweet spot for desktop dictation.")},
        WhisperModelItem {
            .id = u"small"_s,
            .name = tr("Small (Multilingual)"),
            .fileName = u"ggml-small.bin"_s,
            .downloadUrl = kHfBaseUrl + u"ggml-small.bin"_s,
            .sizeBytes = 487601967,
            .sizeFormatted = u"~465 MiB"_s,
            .memoryFormatted = u"~852 MB RAM/VRAM"_s,
            .description = tr("High accuracy multilingual model; excellent balance of speed and recognition quality.")},
        WhisperModelItem {
            .id = u"medium.en"_s,
            .name = tr("Medium (English)"),
            .fileName = u"ggml-medium.en.bin"_s,
            .downloadUrl = kHfBaseUrl + u"ggml-medium.en.bin"_s,
            .sizeBytes = 1533774781,
            .sizeFormatted = u"~1.4 GiB"_s,
            .memoryFormatted = u"~2.1 GB RAM/VRAM"_s,
            .description =
                tr("Near-professional English accuracy for complex vocabulary, technical terms, and accents.")},
        WhisperModelItem {
            .id = u"medium"_s,
            .name = tr("Medium (Multilingual)"),
            .fileName = u"ggml-medium.bin"_s,
            .downloadUrl = kHfBaseUrl + u"ggml-medium.bin"_s,
            .sizeBytes = 1533763059,
            .sizeFormatted = u"~1.4 GiB"_s,
            .memoryFormatted = u"~2.1 GB RAM/VRAM"_s,
            .description =
                tr("Professional-grade multilingual transcription across diverse accents and audio conditions.")},
        WhisperModelItem {.id = u"large-v3-turbo"_s,
                          .name = tr("Large v3 Turbo (Multilingual)"),
                          .fileName = u"ggml-large-v3-turbo.bin"_s,
                          .downloadUrl = kHfBaseUrl + u"ggml-large-v3-turbo.bin"_s,
                          .sizeBytes = 1624555275,
                          .sizeFormatted = u"~1.5 GiB"_s,
                          .memoryFormatted = u"~2.3 GB RAM/VRAM"_s,
                          .description = tr("State-of-the-art multilingual accuracy with optimized 4-layer fast "
                                            "decoder (up to 8x faster than Large v3).")},
        WhisperModelItem {.id = u"large-v3"_s,
                          .name = tr("Large v3 (Multilingual)"),
                          .fileName = u"ggml-large-v3.bin"_s,
                          .downloadUrl = kHfBaseUrl + u"ggml-large-v3.bin"_s,
                          .sizeBytes = 3095033483,
                          .sizeFormatted = u"~2.9 GiB"_s,
                          .memoryFormatted = u"~3.9 GB RAM/VRAM"_s,
                          .description = tr("Maximum accuracy flagship Whisper model for challenging audio, background "
                                            "noise, and rare dialects.")}};
}

void WhisperModelManager::setupDownloaderConnections() {
    connect(m_downloader.get(), &ModelDownloader::isDownloadingAnyChanged, this,
            &WhisperModelManager::isDownloadingAnyChanged);

    connect(m_downloader.get(), &ModelDownloader::lastErrorChanged, this, &WhisperModelManager::setLastError);

    connect(m_downloader.get(), &ModelDownloader::downloadProgressChanged, this,
            [this](const QString& modelId, qreal progress, qint64 bytesReceived, qint64 totalBytes,
                   const QString& speedFormatted) {
                const int idx = findModelIndex(modelId);
                if (idx >= 0 && idx < m_models.size()) {
                    m_models[idx].isDownloading = (progress < 1.0 && bytesReceived > 0);
                    m_models[idx].progress = progress;
                    m_models[idx].bytesReceived = bytesReceived;
                    m_models[idx].totalBytes = totalBytes;
                    m_models[idx].speedFormatted = speedFormatted;

                    const QModelIndex modelIdx = index(idx);
                    emit dataChanged(
                        modelIdx, modelIdx,
                        {IsDownloadingRole, ProgressRole, BytesReceivedRole, TotalBytesRole, SpeedFormattedRole});
                }
                emit downloadProgressChanged();
            });

    connect(m_downloader.get(), &ModelDownloader::downloadFinished, this,
            [this](const QString& modelId, bool success, const QString& error) {
                const int idx = findModelIndex(modelId);
                if (idx >= 0 && idx < m_models.size()) {
                    m_models[idx].isDownloading = false;
                    if (success) {
                        m_models[idx].isInstalled = true;
                        m_models[idx].progress = 1.0;
                        const QFileInfo fi(getModelPath(modelId));
                        m_models[idx].installedSizeBytes = fi.size();
                        m_models[idx].installedSizeFormatted = formatBytes(fi.size());
                        m_models[idx].bytesReceived = fi.size();
                        m_models[idx].totalBytes = fi.size();
                        m_models[idx].speedFormatted.clear();
                    } else {
                        m_models[idx].progress = 0.0;
                        m_models[idx].bytesReceived = 0;
                        m_models[idx].speedFormatted.clear();
                    }

                    const QModelIndex modelIdx = index(idx);
                    emit dataChanged(modelIdx, modelIdx,
                                     {IsDownloadingRole, IsInstalledRole, IsSelectedRole, ProgressRole,
                                      InstalledSizeFormattedRole, CanDeleteRole});
                }

                if (success && !isSelectedModelInstalled()) {
                    setSelectedModelId(modelId);
                }

                emit downloadProgressChanged();
                emit isDownloadingAnyChanged();
                emit modelStatusChanged();
                emit selectedModelChanged();
                emit modelDownloadFinished(modelId, success, error);
                checkDiskSpace();
            });
}

int WhisperModelManager::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_models.size());
}

QVariant WhisperModelManager::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_models.size()) {
        return {};
    }

    const auto& item = m_models.at(index.row());
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
            return item.isInstalled && (item.id == m_selectedModelId);
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
            return item.isInstalled;
        default:
            return {};
    }
}

QHash<int, QByteArray> WhisperModelManager::roleNames() const {
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

QString WhisperModelManager::modelsDirectory() const {
    if (!m_customModelsDirectory.isEmpty()) {
        return m_customModelsDirectory;
    }
    const QString genericData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return genericData + u"/qtscribe/models"_s;
}

void WhisperModelManager::setModelsDirectory(const QString& path) {
    if (m_customModelsDirectory == path) {
        return;
    }
    m_customModelsDirectory = path;
    if (!m_customModelsDirectory.isEmpty()) {
        QDir().mkpath(m_customModelsDirectory);
    }
    cleanupOrphanedPartFiles();
    scanInstalledModels();
    checkDiskSpace();
    emit modelsChanged();
}

const QList<WhisperModelItem>& WhisperModelManager::models() const {
    return m_models;
}

std::optional<WhisperModelItem> WhisperModelManager::model(const QString& modelId) const {
    const auto it = std::ranges::find_if(m_models, [&](const auto& m) { return m.id == modelId; });
    return it != m_models.end() ? std::optional(*it) : std::nullopt;
}

int WhisperModelManager::modelCount() const {
    return static_cast<int>(m_models.size());
}

int WhisperModelManager::findModelIndex(const QString& modelId) const {
    const auto it = std::ranges::find_if(m_models, [&](const auto& m) { return m.id == modelId; });
    return it != m_models.end() ? static_cast<int>(std::distance(m_models.begin(), it)) : -1;
}

QString WhisperModelManager::selectedModelId() const {
    return m_selectedModelId;
}

void WhisperModelManager::setSelectedModelId(const QString& id) {
    if (m_selectedModelId != id) {
        const QString oldId = m_selectedModelId;
        m_selectedModelId = id;

        QSettings settings;
        settings.setValue(u"Whisper/SelectedModel"_s, m_selectedModelId);

        const int oldIdx = findModelIndex(oldId);
        if (oldIdx >= 0) {
            const QModelIndex modelIdx = index(oldIdx);
            emit dataChanged(modelIdx, modelIdx, {IsSelectedRole});
        }
        const int newIdx = findModelIndex(m_selectedModelId);
        if (newIdx >= 0) {
            const QModelIndex modelIdx = index(newIdx);
            emit dataChanged(modelIdx, modelIdx, {IsSelectedRole});
        }

        emit selectedModelChanged();
        emit modelStatusChanged();
    }
}

QString WhisperModelManager::selectedModelPath() const {
    return getModelPath(m_selectedModelId);
}

QString WhisperModelManager::selectedModelName() const {
    const auto item = model(m_selectedModelId);
    if (item.has_value()) {
        return item->name;
    }
    return m_selectedModelId;
}

bool WhisperModelManager::isSelectedModelInstalled() const {
    return isModelInstalled(m_selectedModelId);
}

bool WhisperModelManager::hasAnyModelInstalled() const {
    for (const auto& item : m_models) {
        if (item.isInstalled) {
            return true;
        }
    }
    return false;
}

QString WhisperModelManager::firstInstalledModelId() const {
    for (const auto& item : m_models) {
        if (item.isInstalled) {
            return item.id;
        }
    }
    return {};
}

bool WhisperModelManager::isDownloadingAny() const {
    return m_downloader ? m_downloader->isDownloadingAny() : false;
}

QString WhisperModelManager::downloadingModelId() const {
    return m_downloader ? m_downloader->downloadingModelId() : QString();
}

qreal WhisperModelManager::downloadProgress() const {
    return m_downloader ? m_downloader->downloadProgress() : 0.0;
}

QString WhisperModelManager::downloadSpeedFormatted() const {
    return m_downloader ? m_downloader->downloadSpeedFormatted() : QString();
}

QString WhisperModelManager::downloadBytesFormatted() const {
    return m_downloader ? m_downloader->downloadBytesFormatted() : QString();
}

QString WhisperModelManager::lastError() const {
    return m_lastError;
}

qint64 WhisperModelManager::availableDiskSpace() const {
    return m_availableDiskSpace;
}

QString WhisperModelManager::availableDiskSpaceFormatted() const {
    return tr("%1 free").arg(formatBytes(m_availableDiskSpace));
}

bool WhisperModelManager::isModelInstalled(const QString& modelId) const {
    const int idx = findModelIndex(modelId);
    if (idx >= 0) {
        return m_models[idx].isInstalled;
    }
    return false;
}

QString WhisperModelManager::getModelPath(const QString& modelId) const {
    const int idx = findModelIndex(modelId);
    if (idx < 0) {
        return modelsDirectory() + u"/"_s + modelId;
    }

    QString primary = modelsDirectory() + u"/"_s + m_models[idx].fileName;
    if (QFile::exists(primary)) {
        return primary;
    }

    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString fallback = appData + u"/models/"_s + m_models[idx].fileName;
    if (QFile::exists(fallback)) {
        return fallback;
    }

    return primary;
}

bool WhisperModelManager::startDownload(const QString& modelId) {
    if (isDownloadingAny()) {
        setLastError(tr("Another model download is currently in progress."));
        return false;
    }

    const int idx = findModelIndex(modelId);
    if (idx < 0) {
        setLastError(tr("Model '%1' not found in catalog.").arg(modelId));
        return false;
    }

    checkDiskSpace();
    const auto& modelItem = m_models[idx];

    const qint64 requiredSpace =
        modelItem.sizeBytes > 0 ? (modelItem.sizeBytes + 50 * 1024 * 1024) : (100 * 1024 * 1024);
    if (m_availableDiskSpace > 0 && m_availableDiskSpace < requiredSpace) {
        setLastError(tr("Insufficient disk space. %1 required, but only %2 available.")
                         .arg(formatBytes(requiredSpace), formatBytes(m_availableDiskSpace)));
        return false;
    }

    setLastError({});

    m_models[idx].isDownloading = true;
    m_models[idx].progress = 0.0;
    m_models[idx].bytesReceived = 0;
    m_models[idx].totalBytes = modelItem.sizeBytes;
    m_models[idx].speedFormatted.clear();

    const QString destinationPath = modelsDirectory() + u"/"_s + modelItem.fileName;
    return m_downloader->startDownload(modelItem.id, modelItem.name, modelItem.downloadUrl, destinationPath,
                                       modelItem.sizeBytes);
}

void WhisperModelManager::cancelDownload(const QString& modelId) {
    if (m_downloader) {
        m_downloader->cancelDownload(modelId);
    }
    checkDiskSpace();
}

bool WhisperModelManager::deleteModel(const QString& modelId) {
    if (isDownloadingAny() && downloadingModelId() == modelId) {
        cancelDownload(modelId);
    }

    const int idx = findModelIndex(modelId);
    if (idx < 0) {
        return false;
    }

    const auto& modelItem = m_models[idx];
    bool failed = false;
    const QString primaryPath = modelsDirectory() + u"/"_s + modelItem.fileName;
    if (QFile::exists(primaryPath) && !QFile::remove(primaryPath)) {
        failed = true;
    }
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString fallbackPath = appData + u"/models/"_s + modelItem.fileName;
    if (QFile::exists(fallbackPath) && !QFile::remove(fallbackPath)) {
        failed = true;
    }

    if (failed) {
        setLastError(tr("Failed to delete model file for '%1'.").arg(modelItem.name));
        scanInstalledModels();
        checkDiskSpace();
        return false;
    }

    m_models[idx].isInstalled = false;
    m_models[idx].installedSizeBytes = 0;
    m_models[idx].installedSizeFormatted.clear();

    const QModelIndex modelIdx = index(idx);
    emit dataChanged(modelIdx, modelIdx, {IsInstalledRole, IsSelectedRole, InstalledSizeFormattedRole, CanDeleteRole});
    emit modelStatusChanged();
    emit selectedModelChanged();

    checkDiskSpace();
    return true;
}

void WhisperModelManager::refreshModelList() {
    scanInstalledModels();
    checkDiskSpace();
}

void WhisperModelManager::scanInstalledModels() {
    const QString primaryDir = modelsDirectory();
    const QString fallbackDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + u"/models"_s;

    for (int i = 0; i < m_models.size(); ++i) {
        auto& item = m_models[i];
        const QString primaryPath = primaryDir + u"/"_s + item.fileName;
        const QString fallbackPath = fallbackDir + u"/"_s + item.fileName;

        QString foundPath;
        if (QFile::exists(primaryPath) && QFileInfo(primaryPath).size() > 0) {
            foundPath = primaryPath;
        } else if (QFile::exists(fallbackPath) && QFileInfo(fallbackPath).size() > 0) {
            foundPath = fallbackPath;
        }

        const bool installed = !foundPath.isEmpty();
        if (item.isInstalled != installed) {
            item.isInstalled = installed;
            if (installed) {
                const QFileInfo fi(foundPath);
                item.installedSizeBytes = fi.size();
                item.installedSizeFormatted = formatBytes(fi.size());
            } else {
                item.installedSizeBytes = 0;
                item.installedSizeFormatted.clear();
            }
            const QModelIndex modelIdx = index(i);
            emit dataChanged(modelIdx, modelIdx,
                             {IsInstalledRole, IsSelectedRole, InstalledSizeFormattedRole, CanDeleteRole});
            emit modelStatusChanged();
            emit selectedModelChanged();
        }
    }
}

void WhisperModelManager::checkDiskSpace() {
    const QStorageInfo storage(modelsDirectory());
    const qint64 bytes = storage.bytesAvailable();
    if (m_availableDiskSpace != bytes) {
        m_availableDiskSpace = bytes;
        emit diskSpaceChanged();
    }
}

void WhisperModelManager::cleanupOrphanedPartFiles() {
    QDir dir(modelsDirectory());
    const QStringList partFiles = dir.entryList({u"*.part"_s}, QDir::Files);
    for (const QString& f : partFiles) {
        dir.remove(f);
        qCDebug(lcSpeech) << "WhisperModelManager: Removed orphaned partial download" << f;
    }
}

void WhisperModelManager::setLastError(const QString& error) {
    if (m_lastError != error) {
        m_lastError = error;
        emit lastErrorChanged();
    }
}

QString WhisperModelManager::formatBytes(qint64 bytes) {
    return ModelDownloader::formatBytes(bytes);
}
