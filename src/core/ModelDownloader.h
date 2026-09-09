#pragma once

#include <QElapsedTimer>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QString>

#include <memory>

class QFile;
class QNetworkReply;

class ModelDownloader : public QObject {
    Q_OBJECT

public:
    explicit ModelDownloader(QObject* parent = nullptr);
    ~ModelDownloader() override;

    bool startDownload(const QString& modelId, const QString& modelName, const QString& downloadUrl,
                       const QString& destinationPath, qint64 expectedSizeBytes = 0);
    void cancelDownload(const QString& modelId = QString());

    bool isDownloadingAny() const;
    QString downloadingModelId() const;
    qreal downloadProgress() const;
    qint64 downloadBytesReceived() const;
    qint64 downloadTotalBytes() const;
    QString downloadSpeedFormatted() const;
    QString downloadBytesFormatted() const;
    QString lastError() const;

    static QString formatBytes(qint64 bytes);

signals:
    void isDownloadingAnyChanged();
    void downloadProgressChanged(const QString& modelId, qreal progress, qint64 bytesReceived, qint64 totalBytes,
                                 const QString& speedFormatted);
    void downloadFinished(const QString& modelId, bool success, const QString& error);
    void lastErrorChanged(const QString& error);

private slots:
    void onDownloadReadyRead();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();

private:
    void setLastError(const QString& error);
    void cleanupActiveDownload(bool removePartFile);

    QNetworkAccessManager m_nam;
    QPointer<QNetworkReply> m_currentReply;
    std::unique_ptr<QFile> m_partFile;
    QString m_downloadingModelId;
    QString m_downloadingModelName;
    QString m_destinationPath;
    QString m_partFilePath;
    qint64 m_expectedSizeBytes = 0;
    QString m_downloadAbortReason;
    QString m_lastError;

    qreal m_currentProgress = 0.0;
    qint64 m_currentBytesReceived = 0;
    qint64 m_currentTotalBytes = 0;
    QString m_currentSpeedFormatted;
    QElapsedTimer m_downloadTimer;
    qint64 m_lastSpeedBytes = 0;
    qint64 m_lastSpeedTimeMs = 0;
};
