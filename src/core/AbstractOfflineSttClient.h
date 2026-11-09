#pragma once

#include "AbstractSttClient.h"

class AbstractOfflineSttClient : public AbstractSttClient {
    Q_OBJECT

    Q_PROPERTY(bool isModelInstalled READ isModelInstalled NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString modelPath READ modelPath NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString loadedModelPath READ loadedModelPath NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString computeDevice READ computeDevice NOTIFY computeDeviceChanged FINAL)

public:
    explicit AbstractOfflineSttClient(QObject* parent = nullptr);
    ~AbstractOfflineSttClient() override = default;

    virtual bool isModelInstalled() const = 0;
    virtual bool isModelLoaded() const = 0;
    virtual QString modelPath() const = 0;
    virtual QString loadedModelPath() const = 0;
    virtual QString computeDevice() const = 0;

    virtual void loadModel(const QString& customPath = QString()) = 0;
    virtual void unloadModel() = 0;
    virtual void checkModelStatus() = 0;
    virtual void loadSettings() { }
    virtual bool isExplicitlyUnloaded() const noexcept { return false; }

signals:
    void modelStatusChanged();
    void computeDeviceChanged();
    void modelLoadFailed(const QString& modelPath, const QString& error);
};
