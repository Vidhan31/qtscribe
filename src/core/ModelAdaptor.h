#pragma once

#include <QDBusAbstractAdaptor>
#include <QString>

class ModelService;

class ModelAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.qtscribe.ModelService")

    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY modelStatusChanged)
    Q_PROPERTY(bool isModelBusy READ isModelBusy NOTIFY modelStatusChanged)
    Q_PROPERTY(QString loadedModelPath READ loadedModelPath NOTIFY modelStatusChanged)
    Q_PROPERTY(QString modelPath READ modelPath NOTIFY modelStatusChanged)
    Q_PROPERTY(QString modelFileName READ modelFileName NOTIFY modelStatusChanged)
    Q_PROPERTY(QString computeDevice READ computeDevice NOTIFY computeDeviceChanged)
    Q_PROPERTY(bool isVulkanSupported READ isVulkanSupported CONSTANT)
    Q_PROPERTY(bool isModelInstalled READ isModelInstalled NOTIFY modelStatusChanged)

    Q_PROPERTY(QString modelsDirectory READ modelsDirectory NOTIFY modelCatalogChanged)
    Q_PROPERTY(QString selectedModelName READ selectedModelName NOTIFY modelCatalogChanged)
    Q_PROPERTY(QString modelCatalogJson READ modelCatalogJson NOTIFY modelCatalogChanged)
    Q_PROPERTY(bool modelDownloadingAny READ modelDownloadingAny NOTIFY modelCatalogChanged)
    Q_PROPERTY(QString modelDownloadBytes READ modelDownloadBytes NOTIFY modelCatalogChanged)
    Q_PROPERTY(QString modelCatalogLastError READ modelCatalogLastError NOTIFY modelCatalogChanged)
    Q_PROPERTY(QString modelDiskSpace READ modelDiskSpace NOTIFY modelCatalogChanged)

public:
    explicit ModelAdaptor(ModelService* service, QObject* parent);
    ~ModelAdaptor() override = default;

    [[nodiscard]] bool isModelLoaded() const;
    [[nodiscard]] bool isModelBusy() const;
    [[nodiscard]] QString loadedModelPath() const;
    [[nodiscard]] QString modelPath() const;
    [[nodiscard]] QString modelFileName() const;
    [[nodiscard]] QString computeDevice() const;
    [[nodiscard]] bool isVulkanSupported() const;
    [[nodiscard]] bool isModelInstalled() const;

    [[nodiscard]] QString modelsDirectory() const;
    [[nodiscard]] QString selectedModelName() const;
    [[nodiscard]] QString modelCatalogJson() const;
    [[nodiscard]] bool modelDownloadingAny() const;
    [[nodiscard]] QString modelDownloadBytes() const;
    [[nodiscard]] QString modelCatalogLastError() const;
    [[nodiscard]] QString modelDiskSpace() const;

signals:
    void modelStatusChanged();
    void computeDeviceChanged();
    void modelCatalogChanged();

public slots:
    void loadModel(const QString& customPath = QString());
    void unloadModel();
    void checkModelStatus();
    void selectModel(const QString& modelId);
    bool startModelDownload(const QString& modelId);
    void cancelModelDownload(const QString& modelId);
    bool deleteModel(const QString& modelId);
    void refreshModelCatalog();
    void checkModelDiskSpace();

private:
    ModelService* m_service;
};
