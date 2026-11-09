#pragma once

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

class ModelClient : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(bool isModelBusy READ isModelBusy NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString loadedModelPath READ loadedModelPath NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString modelPath READ modelPath NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString modelFileName READ modelFileName NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString computeDevice READ computeDevice NOTIFY computeDeviceChanged FINAL)
    Q_PROPERTY(bool isVulkanSupported READ isVulkanSupported NOTIFY vulkanSupportedChanged FINAL)
    Q_PROPERTY(bool isModelInstalled READ isModelInstalled NOTIFY modelStatusChanged FINAL)

    Q_PROPERTY(QString modelCatalogJson READ modelCatalogJson NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(QString modelsDirectory READ modelsDirectory NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(bool modelDownloadingAny READ modelDownloadingAny NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(QString modelDownloadBytes READ modelDownloadBytes NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(QString modelCatalogLastError READ modelCatalogLastError NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(QString modelDiskSpace READ modelDiskSpace NOTIFY modelCatalogChanged FINAL)

public:
    explicit ModelClient(QObject* parent = nullptr);
    ~ModelClient() override;

    static ModelClient* instance();
    static ModelClient* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    [[nodiscard]] bool isConnected() const noexcept;
    [[nodiscard]] bool isModelLoaded() const noexcept;
    [[nodiscard]] bool isModelBusy() const noexcept;
    [[nodiscard]] QString loadedModelPath() const;
    [[nodiscard]] QString modelPath() const;
    [[nodiscard]] QString modelFileName() const;
    [[nodiscard]] QString computeDevice() const;
    [[nodiscard]] bool isVulkanSupported() const noexcept;
    [[nodiscard]] bool isModelInstalled() const noexcept;

    [[nodiscard]] QString modelCatalogJson() const;
    [[nodiscard]] QString modelsDirectory() const;
    [[nodiscard]] bool modelDownloadingAny() const noexcept;
    [[nodiscard]] QString modelDownloadBytes() const;
    [[nodiscard]] QString modelCatalogLastError() const;
    [[nodiscard]] QString modelDiskSpace() const;

signals:
    void connectedChanged();
    void modelStatusChanged();
    void computeDeviceChanged();
    void vulkanSupportedChanged();
    void modelCatalogChanged();

public slots:
    void loadModel(const QString& customPath = QString());
    void unloadModel();
    void checkModelStatus();
    void selectModel(const QString& modelId);
    void startModelDownload(const QString& modelId);
    void cancelModelDownload(const QString& modelId);
    void deleteModel(const QString& modelId);
    void refreshModelCatalog();
    void checkModelDiskSpace();

private:
    void setupDBusConnections();
    Q_INVOKABLE void syncFromDaemon();
    Q_INVOKABLE void syncModelCatalog();
    Q_INVOKABLE void onServiceRegistered(const QString& serviceName);
    Q_INVOKABLE void onServiceUnregistered(const QString& serviceName);
    Q_INVOKABLE void onPropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties,
                                         const QStringList& invalidatedProperties);

    static inline ModelClient* s_instance = nullptr;

    QDBusInterface* m_interface = nullptr;
    QDBusServiceWatcher* m_watcher = nullptr;

    bool m_connected = false;
    bool m_isModelLoaded = false;
    bool m_isModelBusy = false;
    QString m_loadedModelPath;
    QString m_modelPath;
    QString m_modelFileName;
    QString m_computeDevice;
    bool m_isVulkanSupported = false;
    bool m_isModelInstalled = false;

    QString m_modelCatalogJson;
    QString m_modelsDirectory;
    bool m_modelDownloadingAny = false;
    QString m_modelDownloadBytes;
    QString m_modelCatalogLastError;
    QString m_modelDiskSpace;
};
