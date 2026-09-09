#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

#include <atomic>
#include <cstdint>

struct whisper_context;

class WhisperWorker : public QObject {
    Q_OBJECT

public:
    explicit WhisperWorker(QObject* parent = nullptr);
    ~WhisperWorker() override;
    void cancel(uint64_t requestId = 0);
    void cancelLoad(uint64_t loadRequestId = 0);
    void resetAbort();
    bool isAborted(uint64_t requestId = 0) const;
    bool isLoadAborted(uint64_t loadRequestId = 0) const;

public slots:
    void loadModel(uint64_t loadRequestId, const QString& modelPath, bool useGpu = true);
    void unloadModel();
    void transcribe(uint64_t requestId, const QByteArray& wavData, const QString& language = QString(),
                    const QString& prompt = QString());

signals:
    void modelLoaded(uint64_t loadRequestId, const QString& modelPath, bool success, const QString& error,
                     const QString& activeDevice);
    void modelUnloaded();
    void transcriptionFinished(uint64_t requestId, const QString& text);
    void transcriptionFailed(uint64_t requestId, const QString& error);

private:
    whisper_context* m_ctx = nullptr;
    std::atomic<bool> m_abortRequested {false};
    std::atomic<uint64_t> m_cancelledRequestId {0};
    std::atomic<uint64_t> m_cancelledLoadRequestId {0};
    QString m_activeDevice;
};
