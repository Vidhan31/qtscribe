#include "WhisperSttClient.h"

#include "LoggingCategories.h"

#include "WhisperModelManager.h"
#include "WhisperWorker.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

WhisperSttClient::WhisperSttClient(QObject* parent)
    : AbstractSttClient(parent)
    , m_worker(new WhisperWorker()) {
    m_worker->moveToThread(&m_workerThread);

    connect(this, &WhisperSttClient::requestLoadModel, m_worker, &WhisperWorker::loadModel, Qt::QueuedConnection);
    connect(this, &WhisperSttClient::requestUnloadModel, m_worker, &WhisperWorker::unloadModel, Qt::QueuedConnection);
    connect(this, &WhisperSttClient::requestTranscribe, m_worker, &WhisperWorker::transcribe, Qt::QueuedConnection);

    connect(m_worker, &WhisperWorker::modelLoaded, this, &WhisperSttClient::onWorkerModelLoaded, Qt::QueuedConnection);
    connect(m_worker, &WhisperWorker::modelUnloaded, this, &WhisperSttClient::onWorkerModelUnloaded,
            Qt::QueuedConnection);
    connect(m_worker, &WhisperWorker::transcriptionFinished, this, &WhisperSttClient::onWorkerTranscriptionFinished,
            Qt::QueuedConnection);
    connect(m_worker, &WhisperWorker::transcriptionFailed, this, &WhisperSttClient::onWorkerTranscriptionFailed,
            Qt::QueuedConnection);

    if (auto* app = QCoreApplication::instance()) {
        connect(app, &QCoreApplication::aboutToQuit, this, [this]() {
            cancel();
            if (m_worker) {
                m_worker->cancel(0);
            }
        });
    }

    m_workerThread.setObjectName(u"WhisperInferenceThread"_s);
    m_workerThread.start();

    QDir().mkpath(modelsDirectory());
}

WhisperSttClient::~WhisperSttClient() {
    cancel();
    if (m_worker) {
        m_worker->cancel(0);
    }
    m_workerThread.requestInterruption();
    m_workerThread.quit();
    if (!m_workerThread.wait()) {
        qCWarning(lcSpeech) << "WhisperSttClient: Failed waiting for worker thread to exit";
    }
    delete m_worker;
    m_worker = nullptr;
}

void WhisperSttClient::setModelManager(WhisperModelManager* manager) {
    if (m_modelManager == manager) {
        return;
    }

    if (m_modelManager) {
        disconnect(m_modelManager, &WhisperModelManager::selectedModelChanged, this,
                   &WhisperSttClient::checkModelStatus);
        disconnect(m_modelManager, &WhisperModelManager::modelStatusChanged, this, &WhisperSttClient::checkModelStatus);
    }

    m_modelManager = manager;

    if (m_modelManager) {
        connect(m_modelManager, &WhisperModelManager::selectedModelChanged, this, &WhisperSttClient::checkModelStatus);
        connect(m_modelManager, &WhisperModelManager::modelStatusChanged, this, &WhisperSttClient::checkModelStatus);
    }

    checkModelStatus();
}

WhisperModelManager* WhisperSttClient::modelManager() const {
    return m_modelManager;
}

QString WhisperSttClient::modelsDirectory() const {
    if (m_modelManager) {
        return m_modelManager->modelsDirectory();
    }
    const QString genericData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return genericData + u"/qtscribe/models"_s;
}

QString WhisperSttClient::modelFileName() const {
    const QString path = resolveModelPath();
    return QFileInfo(path).fileName();
}

QString WhisperSttClient::resolveModelPath() const {
    if (m_modelManager) {
        return m_modelManager->selectedModelPath();
    }

    QString primary = modelsDirectory() + u"/ggml-tiny.en.bin"_s;
    if (QFile::exists(primary)) {
        return primary;
    }

    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString fallback = appData + u"/models/ggml-tiny.en.bin"_s;
    if (QFile::exists(fallback)) {
        return fallback;
    }

    return primary;
}

bool WhisperSttClient::isModelInstalled() const {
    return QFile::exists(resolveModelPath());
}

bool WhisperSttClient::isModelLoaded() const {
    return m_modelLoaded;
}

QString WhisperSttClient::modelPath() const {
    return resolveModelPath();
}

QString WhisperSttClient::loadedModelPath() const {
    return m_loadedModelPath;
}

QString WhisperSttClient::computeDevice() const {
    return m_computeDevice.isEmpty() ? u"None (Model Unloaded)"_s : m_computeDevice;
}

QString WhisperSttClient::lastError() const {
    return m_lastError;
}

bool WhisperSttClient::isVulkanSupported() const {
#if defined(GGML_USE_VULKAN)
    return true;
#else
    return false;
#endif
}

bool WhisperSttClient::isReady() const {
    return m_modelLoaded && !m_busy && (m_loadedModelPath == resolveModelPath()) && QFile::exists(m_loadedModelPath);
}

bool WhisperSttClient::isBusy() const {
    return m_busy;
}

void WhisperSttClient::activate() {
    if (isModelInstalled() && (!m_modelLoaded || m_loadedModelPath != resolveModelPath())) {
        loadModel();
    }
    emit readyChanged();
}

void WhisperSttClient::deactivate() {
    cancel();
    unloadModel();
    emit readyChanged();
}

void WhisperSttClient::checkModelStatus() {
    const QString targetPath = resolveModelPath();

    if (m_modelLoaded) {
        if (!m_loadedModelPath.isEmpty() && !QFile::exists(m_loadedModelPath)) {
            qCDebug(lcSpeech) << "WhisperSttClient: Loaded model file no longer exists, unloading:"
                              << m_loadedModelPath;
            unloadModel();
        } else if (m_loadedModelPath != targetPath) {
            if (isModelInstalled()) {
                qCDebug(lcSpeech) << "WhisperSttClient: Model selection changed, reloading to target:" << targetPath;
                loadModel(targetPath);
            } else {
                qCDebug(lcSpeech) << "WhisperSttClient: Selected model not installed, unloading current model";
                unloadModel();
            }
        }
    }

    emit modelStatusChanged();
    emit readyChanged();
}

void WhisperSttClient::loadModel(const QString& customPath) {
    const QString path = customPath.isEmpty() ? resolveModelPath() : customPath;
    if (m_worker && m_activeLoadRequestId > 0) {
        m_worker->cancelLoad(m_activeLoadRequestId);
    }
    const uint64_t loadId = m_nextLoadRequestId++;
    m_activeLoadRequestId = loadId;

    if (m_worker) {
        m_worker->resetAbort();
    }

    if (!QFile::exists(path)) {
        const QString err = tr("Whisper model file not found at %1. Please download it in Model Settings.").arg(path);
        setLastError(err);
        m_modelLoaded = false;
        m_loadedModelPath.clear();
        m_computeDevice.clear();
        emit modelStatusChanged();
        emit readyChanged();
        emit modelLoadFailed(path, err);
        return;
    }

    setLastError({});
    emit requestLoadModel(loadId, path, true);
}

void WhisperSttClient::unloadModel() {
    if (m_worker && m_activeLoadRequestId > 0) {
        m_worker->cancelLoad(m_activeLoadRequestId);
    }
    m_activeLoadRequestId = m_nextLoadRequestId++;
    m_modelLoaded = false;
    m_loadedModelPath.clear();
    m_computeDevice.clear();
    cancel();
    if (m_worker) {
        m_worker->cancel(0);
    }
    emit requestUnloadModel();
}

void WhisperSttClient::transcribe(const QByteArray& wavData) {
    if (m_busy) {
        qCDebug(lcSpeech) << "WhisperSttClient: transcribe ignored — inference already in progress";
        return;
    }

    if (!isReady()) {
        qWarning() << "WhisperSttClient: transcribe called when not ready (loaded:" << m_modelLoaded << ")";
        const QString err = tr("Offline Whisper model is not loaded or ready");
        setLastError(err);
        emit errorOccurred(err);
        return;
    }

    if (wavData.isEmpty()) {
        qWarning() << "WhisperSttClient: Empty audio payload passed to transcribe";
        const QString err = tr("No audio data to transcribe");
        setLastError(err);
        emit errorOccurred(err);
        return;
    }

    const uint64_t reqId = m_nextRequestId++;
    m_activeRequestId = reqId;
    setBusy(true);
    setLastError({});

    if (m_worker) {
        m_worker->resetAbort();
    }

    emit requestTranscribe(reqId, wavData, QString(), QString());
}

void WhisperSttClient::cancel() {
    if (m_busy || m_activeRequestId != 0) {
        const uint64_t cancelledId = m_activeRequestId;
        m_activeRequestId = 0;
        if (m_worker) {
            m_worker->cancel(cancelledId);
        }
        setBusy(false);
    }
}

void WhisperSttClient::onWorkerModelLoaded(uint64_t loadRequestId, const QString& modelPath, bool success,
                                           const QString& error, const QString& activeDevice) {
    if (loadRequestId == 0 || loadRequestId != m_activeLoadRequestId) {
        qCDebug(lcSpeech) << "WhisperSttClient: Discarding stale model load result for request" << loadRequestId
                          << "path:" << modelPath << "(active load request:" << m_activeLoadRequestId << ")";
        return;
    }

    m_modelLoaded = success;
    m_computeDevice = activeDevice;
    if (!success) {
        m_loadedModelPath.clear();
        const QString err = error.isEmpty() ? tr("Failed to load whisper.cpp model") : error;
        setLastError(err);
        emit modelLoadFailed(modelPath, err);
    } else {
        m_loadedModelPath = modelPath;
        setLastError({});
    }

    emit modelStatusChanged();
    emit computeDeviceChanged();
    emit readyChanged();
}

void WhisperSttClient::onWorkerModelUnloaded() {
    m_modelLoaded = false;
    m_loadedModelPath.clear();
    m_computeDevice.clear();
    emit modelStatusChanged();
    emit computeDeviceChanged();
    emit readyChanged();
}

void WhisperSttClient::onWorkerTranscriptionFinished(uint64_t requestId, const QString& text) {
    if (requestId == 0 || requestId != m_activeRequestId) {
        qCDebug(lcSpeech) << "WhisperSttClient: Discarding stale transcription finished for request" << requestId
                          << "(active request:" << m_activeRequestId << ")";
        return;
    }

    m_activeRequestId = 0;
    setBusy(false);

    if (text.isEmpty()) {
        const QString err = tr("Whisper returned empty transcription");
        setLastError(err);
        emit errorOccurred(err);
        return;
    }

    setLastError({});
    emit transcriptionReady(text);
}

void WhisperSttClient::onWorkerTranscriptionFailed(uint64_t requestId, const QString& error) {
    if (requestId == 0 || requestId != m_activeRequestId) {
        qCDebug(lcSpeech) << "WhisperSttClient: Discarding stale transcription failure for request" << requestId
                          << "(active request:" << m_activeRequestId << ")";
        return;
    }

    m_activeRequestId = 0;
    setBusy(false);
    setLastError(error);
    emit errorOccurred(error);
}

void WhisperSttClient::setBusy(bool busy) {
    if (m_busy != busy) {
        m_busy = busy;
        emit busyChanged();
        emit readyChanged();
    }
}

void WhisperSttClient::setLastError(const QString& error) {
    if (m_lastError != error) {
        m_lastError = error;
        emit lastErrorChanged();
    }
}
