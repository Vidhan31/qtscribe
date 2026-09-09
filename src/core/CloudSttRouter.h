#pragma once

#include "AbstractSttClient.h"

#include <QHash>
#include <QQmlEngine>
#include <QString>

class CloudProviderModel;

class CloudSttRouter : public AbstractSttClient {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit CloudSttRouter(QObject* parent = nullptr);
    ~CloudSttRouter() override = default;

    void setCloudProviderModel(CloudProviderModel* model);
    CloudProviderModel* cloudProviderModel() const;

    void registerProvider(const QString& providerId, AbstractSttClient* client);
    void unregisterProvider(const QString& providerId);

    AbstractSttClient* activeCloudClient() const;
    AbstractSttClient* activeClient() const;

    bool isApiKeySet() const override;
    bool isApiKeyInvalid() const override;
    bool isRateLimited() const override;
    int retrySecondsRemaining() const override;
    ErrorCategory errorCategory() const override;

    void transcribe(const QByteArray& wavData) override;
    void cancel() override;
    void retryLast() override;
    bool isReady() const override;
    bool isBusy() const override;
    bool handlesSmartFormatting() const override;
    QString lastError() const override;

    void activate() override;
    void deactivate() override;

private slots:
    void onActiveProviderChanged();

private:
    void updateActiveClient();

    CloudProviderModel* m_providerModel = nullptr;
    QHash<QString, AbstractSttClient*> m_clients;
    AbstractSttClient* m_activeClient = nullptr;
};
