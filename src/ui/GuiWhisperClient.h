#pragma once

#include "DaemonClient.h"

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

class GuiWhisperClient : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(WhisperSttClient)
    QML_SINGLETON

    Q_PROPERTY(bool isModelInstalled READ isModelInstalled NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString modelPath READ modelPath NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString loadedModelPath READ loadedModelPath NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString modelsDirectory READ modelsDirectory CONSTANT FINAL)
    Q_PROPERTY(QString modelFileName READ modelFileName NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString computeDevice READ computeDevice NOTIFY computeDeviceChanged FINAL)
    Q_PROPERTY(bool isVulkanSupported READ isVulkanSupported NOTIFY vulkanSupportedChanged FINAL)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged FINAL)

public:
    explicit GuiWhisperClient(QObject* parent = nullptr);
    ~GuiWhisperClient() override = default;

    static GuiWhisperClient* instance();
    static GuiWhisperClient* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    [[nodiscard]] bool isModelInstalled() const;
    [[nodiscard]] bool isModelLoaded() const;
    [[nodiscard]] QString modelPath() const;
    [[nodiscard]] QString loadedModelPath() const;
    [[nodiscard]] QString modelsDirectory() const;
    [[nodiscard]] QString modelFileName() const;
    [[nodiscard]] QString computeDevice() const;
    [[nodiscard]] bool isVulkanSupported() const;
    [[nodiscard]] bool isBusy() const;

public slots:
    void loadModel(const QString& customPath = QString());
    void unloadModel();
    void checkModelStatus();

signals:
    void modelStatusChanged();
    void computeDeviceChanged();
    void vulkanSupportedChanged();
    void busyChanged();
};
