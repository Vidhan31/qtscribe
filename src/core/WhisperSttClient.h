#pragma once

#include "AbstractOfflineSttClient.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <QThread>

#include <cstdint>

class WhisperWorker;
class WhisperModelManager;

class WhisperSttClient : public AbstractOfflineSttClient {
    Q_OBJECT

    Q_PROPERTY(bool isModelInstalled READ isModelInstalled NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString modelPath READ modelPath NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString loadedModelPath READ loadedModelPath NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString modelsDirectory READ modelsDirectory CONSTANT FINAL)
    Q_PROPERTY(QString modelFileName READ modelFileName NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString computeDevice READ computeDevice NOTIFY computeDeviceChanged FINAL)
    Q_PROPERTY(bool isVulkanSupported READ isVulkanSupported CONSTANT FINAL)

public:
    explicit WhisperSttClient(QObject* parent = nullptr);
    ~WhisperSttClient() override;

    void setModelManager(WhisperModelManager* manager);
    WhisperModelManager* modelManager() const;

    bool isModelInstalled() const override;
    bool isModelLoaded() const override;
    QString modelPath() const override;
    QString loadedModelPath() const override;
    QString modelsDirectory() const;
    QString modelFileName() const;
    QString computeDevice() const override;
    QString lastError() const override;
    static bool isVulkanSupported();

    bool isReady() const override;
    bool isBusy() const override;
    [[nodiscard]] bool isActive() const noexcept { return m_active; }
    [[nodiscard]] bool isExplicitlyUnloaded() const noexcept override { return m_explicitlyUnloaded; }

    void activate() override;
    void deactivate() override;

    Q_INVOKABLE void transcribe(const QByteArray& wavData) override;
    Q_INVOKABLE void cancel() override;
    Q_INVOKABLE void loadModel(const QString& customPath = QString()) override;
    Q_INVOKABLE void unloadModel() override;
    Q_INVOKABLE void checkModelStatus() override;
    void loadSettings() override;

signals:
    void requestLoadModel(uint64_t loadRequestId, const QString& path, bool useGpu);
    void requestUnloadModel(uint64_t unloadRequestId);
    void requestTranscribe(uint64_t requestId, const QByteArray& wavData, const QString& language,
                           const QString& prompt);

private:
    Q_INVOKABLE void onWorkerModelLoaded(uint64_t loadRequestId, const QString& modelPath, bool success,
                                         const QString& error, const QString& activeDevice);
    Q_INVOKABLE void onWorkerModelUnloaded(uint64_t unloadRequestId = 0);
    Q_INVOKABLE void onWorkerTranscriptionFinished(uint64_t requestId, const QString& text);
    Q_INVOKABLE void onWorkerTranscriptionFailed(uint64_t requestId, const QString& error);

    void setLastError(const QString& error);
    void setBusy(bool busy);
    QString resolveModelPath() const;

    QThread m_workerThread;
    WhisperWorker* m_worker = nullptr;
    QPointer<WhisperModelManager> m_modelManager;

    uint64_t m_nextLoadRequestId = 1;
    uint64_t m_activeLoadRequestId = 0;
    uint64_t m_nextUnloadRequestId = 1;
    uint64_t m_activeUnloadRequestId = 0;
    uint64_t m_nextRequestId = 1;
    uint64_t m_activeRequestId = 0;
    QString m_pendingLoadPath;
    bool m_active = false;
    bool m_modelLoaded = false;
    bool m_busy = false;
    bool m_explicitlyUnloaded = false;
    QString m_loadedModelPath;
    QString m_computeDevice;
    QString m_lastError;
};
