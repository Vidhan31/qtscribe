#include "ModelDownloader.h"

#include "LoggingCategories.h"

#include <QFile>
#include <QFileInfo>
#include <QHttpHeaders>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include <cstring>

using namespace Qt::StringLiterals;

ModelDownloader::ModelDownloader(QObject* parent)
    : QObject(parent) { }

ModelDownloader::~ModelDownloader() {
    cleanupActiveDownload(true);
}

bool ModelDownloader::startDownload(const QString& modelId, const QString& modelName, const QString& downloadUrl,
                                    const QString& destinationPath, qint64 expectedSizeBytes) {
    if (isDownloadingAny()) {
        setLastError(tr("Another model download is currently in progress."));
        return false;
    }

    setLastError({});
    m_downloadingModelId = modelId;
    m_downloadingModelName = modelName;
    m_destinationPath = destinationPath;
    m_partFilePath = destinationPath + u".part"_s;
    m_expectedSizeBytes = expectedSizeBytes;
    m_downloadAbortReason.clear();
    m_currentProgress = 0.0;
    m_currentBytesReceived = 0;
    m_currentTotalBytes = expectedSizeBytes;
    m_currentSpeedFormatted.clear();
    m_lastSpeedBytes = 0;
    m_lastSpeedTimeMs = 0;
    m_downloadTimer.restart();

    m_partFile = std::make_unique<QFile>(m_partFilePath);
    if (!m_partFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setLastError(tr("Failed to create temporary file for download: %1").arg(m_partFile->errorString()));
        cleanupActiveDownload(false);
        return false;
    }

    emit isDownloadingAnyChanged();
    emit downloadProgressChanged(modelId, 0.0, 0, expectedSizeBytes, QString());

    QNetworkRequest request {QUrl(downloadUrl)};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QHttpHeaders headers;
    headers.append(QHttpHeaders::WellKnownHeader::UserAgent, u"QtScribe/1.0 (Linux; Wayland)"_s);
    request.setHeaders(headers);

    qCDebug(lcSpeech) << "ModelDownloader: Starting download for" << modelId << "from" << downloadUrl;

    m_currentReply = m_nam.get(request);
    connect(m_currentReply, &QNetworkReply::readyRead, this, &ModelDownloader::onDownloadReadyRead);
    connect(m_currentReply, &QNetworkReply::downloadProgress, this, &ModelDownloader::onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::finished, this, &ModelDownloader::onDownloadFinished);

    return true;
}

void ModelDownloader::cancelDownload(const QString& modelId) {
    if (!isDownloadingAny()) {
        return;
    }

    if (!modelId.isEmpty() && modelId != m_downloadingModelId) {
        return;
    }

    const QString cancelledId = m_downloadingModelId;
    qCDebug(lcSpeech) << "ModelDownloader: Cancelling download for" << cancelledId;

    cleanupActiveDownload(true);

    emit downloadProgressChanged(cancelledId, 0.0, 0, 0, QString());
    emit isDownloadingAnyChanged();
}

bool ModelDownloader::isDownloadingAny() const {
    return !m_downloadingModelId.isEmpty();
}

QString ModelDownloader::downloadingModelId() const {
    return m_downloadingModelId;
}

qreal ModelDownloader::downloadProgress() const {
    return m_currentProgress;
}

qint64 ModelDownloader::downloadBytesReceived() const {
    return m_currentBytesReceived;
}

qint64 ModelDownloader::downloadTotalBytes() const {
    return m_currentTotalBytes;
}

QString ModelDownloader::downloadSpeedFormatted() const {
    return m_currentSpeedFormatted;
}

QString ModelDownloader::downloadBytesFormatted() const {
    if (m_currentTotalBytes > 0) {
        return tr("%1 / %2").arg(formatBytes(m_currentBytesReceived), formatBytes(m_currentTotalBytes));
    }
    if (m_currentBytesReceived > 0) {
        return formatBytes(m_currentBytesReceived);
    }
    return {};
}

QString ModelDownloader::lastError() const {
    return m_lastError;
}

void ModelDownloader::onDownloadReadyRead() {
    if (!m_currentReply || !m_partFile || !m_partFile->isOpen() || m_downloadingModelId.isEmpty()) {
        return;
    }

    const QByteArray chunk = m_currentReply->readAll();
    if (chunk.isEmpty()) {
        return;
    }

    const qint64 currentSize = m_partFile->size();
    if (m_expectedSizeBytes > 0 && (currentSize + chunk.size() > m_expectedSizeBytes)) {
        m_downloadAbortReason = tr("Download for %1 exceeded catalogued size of %2.")
                                    .arg(m_downloadingModelName, formatBytes(m_expectedSizeBytes));
        qCWarning(lcSpeech) << "ModelDownloader: Download for" << m_downloadingModelId << "exceeded catalogued size ("
                            << (currentSize + chunk.size()) << ">" << m_expectedSizeBytes << "bytes). Aborting.";
        setLastError(m_downloadAbortReason);
        m_currentReply->abort();
        return;
    }

    const qint64 bytesWritten = m_partFile->write(chunk);
    if (bytesWritten != chunk.size() || m_partFile->error() != QFile::NoError) {
        const QString errStr = m_partFile->errorString();
        m_downloadAbortReason =
            tr("Failed to write download data to disk for %1: %2")
                .arg(m_downloadingModelName, errStr.isEmpty() ? tr("Short write or disk error") : errStr);
        qCWarning(lcSpeech) << "ModelDownloader: Short write or disk error while downloading" << m_downloadingModelId
                            << ":" << m_downloadAbortReason;
        setLastError(m_downloadAbortReason);
        m_currentReply->abort();
        return;
    }
}

void ModelDownloader::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (!m_currentReply || m_downloadingModelId.isEmpty()) {
        return;
    }

    if (m_expectedSizeBytes > 0 && (bytesTotal > m_expectedSizeBytes || bytesReceived > m_expectedSizeBytes)) {
        m_downloadAbortReason = tr("Download size for %1 exceeds catalogued size of %2.")
                                    .arg(m_downloadingModelName, formatBytes(m_expectedSizeBytes));
        qCWarning(lcSpeech) << "ModelDownloader: Reported download size (" << qMax(bytesTotal, bytesReceived)
                            << "bytes) exceeds catalogued size (" << m_expectedSizeBytes << "bytes) for"
                            << m_downloadingModelId << ". Aborting.";
        setLastError(m_downloadAbortReason);
        m_currentReply->abort();
        return;
    }

    m_currentBytesReceived = bytesReceived;
    if (bytesTotal > 0) {
        m_currentTotalBytes = bytesTotal;
        m_currentProgress = static_cast<qreal>(bytesReceived) / static_cast<qreal>(bytesTotal);
    }

    const qint64 elapsedMs = m_downloadTimer.elapsed();
    if (elapsedMs - m_lastSpeedTimeMs >= 250 && elapsedMs > 0) {
        const qint64 bytesDelta = bytesReceived - m_lastSpeedBytes;
        const qint64 timeDeltaMs = elapsedMs - m_lastSpeedTimeMs;
        if (timeDeltaMs > 0) {
            const qreal speedBytesPerSec = (static_cast<qreal>(bytesDelta) * 1000.0) / static_cast<qreal>(timeDeltaMs);
            m_currentSpeedFormatted = tr("%1/s").arg(formatBytes(static_cast<qint64>(speedBytesPerSec)));
        }
        m_lastSpeedBytes = bytesReceived;
        m_lastSpeedTimeMs = elapsedMs;
    }

    emit downloadProgressChanged(m_downloadingModelId, m_currentProgress, bytesReceived, m_currentTotalBytes,
                                 m_currentSpeedFormatted);
}

void ModelDownloader::onDownloadFinished() {
    if (!m_currentReply) {
        return;
    }

    const QString modelId = m_downloadingModelId;
    const QString modelName = m_downloadingModelName;
    const QString partPath = m_partFilePath;
    const QString finalPath = m_destinationPath;
    const qint64 expectedSize = m_expectedSizeBytes;

    const QNetworkReply::NetworkError error = m_currentReply->error();
    const int httpStatus = m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString errorString = m_currentReply->errorString();
    const QString abortReason = m_downloadAbortReason;

    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    if (m_partFile) {
        m_partFile->flush();
        m_partFile->close();
        m_partFile.reset();
    }

    bool success = false;
    QString failureMessage;

    if (!abortReason.isEmpty()) {
        failureMessage = abortReason;
    } else if (error != QNetworkReply::NoError || httpStatus < 200 || httpStatus >= 300) {
        const QString fullErr = errorString.isEmpty() ? tr("HTTP error %1").arg(httpStatus) : errorString;
        failureMessage = tr("Download failed for %1: %2").arg(modelName, fullErr);
    } else {
        const QFileInfo partInfo(partPath);
        if (!partInfo.exists()) {
            failureMessage = tr("Downloaded temporary file not found: %1").arg(partPath);
        } else {
            const qint64 actualSize = partInfo.size();
            if (actualSize == 0) {
                failureMessage = tr("Download failed for %1: received empty file.").arg(modelName);
            } else if (expectedSize > 0 && actualSize != expectedSize) {
                failureMessage =
                    tr("Download size mismatch for %1: expected %2 (%3 bytes), but received %4 (%5 bytes).")
                        .arg(modelName, formatBytes(expectedSize), QString::number(expectedSize),
                             formatBytes(actualSize), QString::number(actualSize));
            } else {
                QFile verifyFile(partPath);
                if (!verifyFile.open(QIODevice::ReadOnly)) {
                    failureMessage =
                        tr("Failed to open downloaded file for validation: %1").arg(verifyFile.errorString());
                } else {
                    const QByteArray header = verifyFile.read(4);
                    verifyFile.close();

                    constexpr quint32 kGgmlMagic = 0x67676d6c;
                    quint32 magicVal = 0;
                    if (header.size() == 4) {
                        std::memcpy(&magicVal, header.constData(), 4);
                    }

                    if (magicVal != kGgmlMagic) {
                        failureMessage =
                            tr("Download integrity check failed for %1: invalid GGML model format.").arg(modelName);
                    } else {
                        QFile::remove(finalPath);
                        if (QFile::rename(partPath, finalPath)) {
                            success = true;
                        } else {
                            failureMessage =
                                tr("Failed to rename temporary download to destination: %1").arg(finalPath);
                        }
                    }
                }
            }
        }
    }

    cleanupActiveDownload(!success);

    if (success) {
        qCDebug(lcSpeech) << "ModelDownloader: Successfully validated, downloaded, and finalized" << finalPath;
        emit isDownloadingAnyChanged();
        emit downloadFinished(modelId, true, QString());
        return;
    }

    qCWarning(lcSpeech) << "ModelDownloader: Download failed for" << modelId << ":" << failureMessage;
    setLastError(failureMessage);
    emit downloadProgressChanged(modelId, 0.0, 0, 0, QString());
    emit isDownloadingAnyChanged();
    emit downloadFinished(modelId, false, failureMessage);
}

void ModelDownloader::setLastError(const QString& error) {
    if (m_lastError != error) {
        m_lastError = error;
        emit lastErrorChanged(m_lastError);
    }
}

void ModelDownloader::cleanupActiveDownload(bool removePartFile) {
    if (m_currentReply) {
        m_currentReply->disconnect(this);
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    if (m_partFile) {
        m_partFile->close();
        m_partFile.reset();
    }

    if (removePartFile && !m_partFilePath.isEmpty()) {
        QFile::remove(m_partFilePath);
    }

    m_downloadingModelId.clear();
    m_downloadingModelName.clear();
    m_destinationPath.clear();
    m_partFilePath.clear();
    m_expectedSizeBytes = 0;
    m_downloadAbortReason.clear();
    m_currentProgress = 0.0;
    m_currentBytesReceived = 0;
    m_currentTotalBytes = 0;
    m_currentSpeedFormatted.clear();
}

QString ModelDownloader::formatBytes(qint64 bytes) {
    if (bytes < 0) {
        return u"0 B"_s;
    }
    const qreal b = static_cast<qreal>(bytes);
    if (b < 1024.0) {
        return QString::number(bytes) + u" B"_s;
    }
    if (b < 1024.0 * 1024.0) {
        return QString::number(b / 1024.0, 'f', 1) + u" KiB"_s;
    }
    if (b < 1024.0 * 1024.0 * 1024.0) {
        return QString::number(b / (1024.0 * 1024.0), 'f', 1) + u" MiB"_s;
    }
    return QString::number(b / (1024.0 * 1024.0 * 1024.0), 'f', 2) + u" GiB"_s;
}
