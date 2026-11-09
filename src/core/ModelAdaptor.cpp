#include "ModelAdaptor.h"

#include "ModelService.h"

ModelAdaptor::ModelAdaptor(ModelService* service, QObject* parent)
    : QDBusAbstractAdaptor(parent)
    , m_service(service) {
    setAutoRelaySignals(true);

    if (m_service) {
        connect(m_service, &ModelService::modelStatusChanged, this, &ModelAdaptor::modelStatusChanged);
        connect(m_service, &ModelService::computeDeviceChanged, this, &ModelAdaptor::computeDeviceChanged);
        connect(m_service, &ModelService::modelCatalogChanged, this, &ModelAdaptor::modelCatalogChanged);
    }
}

bool ModelAdaptor::isModelLoaded() const {
    return m_service ? m_service->isModelLoaded() : false;
}

bool ModelAdaptor::isModelBusy() const {
    return m_service ? m_service->isModelBusy() : false;
}

QString ModelAdaptor::loadedModelPath() const {
    return m_service ? m_service->loadedModelPath() : QString();
}

QString ModelAdaptor::modelPath() const {
    return m_service ? m_service->modelPath() : QString();
}

QString ModelAdaptor::modelFileName() const {
    return m_service ? m_service->modelFileName() : QString();
}

QString ModelAdaptor::computeDevice() const {
    return m_service ? m_service->computeDevice() : QString();
}

bool ModelAdaptor::isVulkanSupported() const {
    return m_service ? ModelService::isVulkanSupported() : false;
}

bool ModelAdaptor::isModelInstalled() const {
    return m_service ? m_service->isModelInstalled() : false;
}

QString ModelAdaptor::modelsDirectory() const {
    return m_service ? m_service->modelsDirectory() : QString();
}

QString ModelAdaptor::selectedModelName() const {
    return m_service ? m_service->selectedModelName() : QString();
}

QString ModelAdaptor::modelCatalogJson() const {
    return m_service ? m_service->modelCatalogJson() : QStringLiteral("[]");
}

bool ModelAdaptor::modelDownloadingAny() const {
    return m_service ? m_service->modelDownloadingAny() : false;
}

QString ModelAdaptor::modelDownloadBytes() const {
    return m_service ? m_service->modelDownloadBytes() : QString();
}

QString ModelAdaptor::modelCatalogLastError() const {
    return m_service ? m_service->modelCatalogLastError() : QString();
}

QString ModelAdaptor::modelDiskSpace() const {
    return m_service ? m_service->modelDiskSpace() : QString();
}

void ModelAdaptor::loadModel(const QString& customPath) {
    if (m_service) {
        m_service->loadModel(customPath);
    }
}

void ModelAdaptor::unloadModel() {
    if (m_service) {
        m_service->unloadModel();
    }
}

void ModelAdaptor::checkModelStatus() {
    if (m_service) {
        m_service->checkModelStatus();
    }
}

void ModelAdaptor::selectModel(const QString& modelId) {
    if (m_service) {
        m_service->selectModel(modelId);
    }
}

bool ModelAdaptor::startModelDownload(const QString& modelId) {
    return m_service && m_service->startModelDownload(modelId);
}

void ModelAdaptor::cancelModelDownload(const QString& modelId) {
    if (m_service) {
        m_service->cancelModelDownload(modelId);
    }
}

bool ModelAdaptor::deleteModel(const QString& modelId) {
    return m_service && m_service->deleteModel(modelId);
}

void ModelAdaptor::refreshModelCatalog() {
    if (m_service) {
        m_service->refreshModelCatalog();
    }
}

void ModelAdaptor::checkModelDiskSpace() {
    if (m_service) {
        m_service->checkModelDiskSpace();
    }
}
