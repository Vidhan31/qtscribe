#pragma once

#include "AbstractSttClient.h"

#include <QHash>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

class CloudSttRouter : public AbstractSttClient {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString activeProviderId READ activeProviderId WRITE setActiveProviderId NOTIFY activeProviderChanged FINAL)

public:
    explicit CloudSttRouter(QObject* parent = nullptr);
    ~CloudSttRouter() override;

    static CloudSttRouter* instance();
    static CloudSttRouter* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    void registerProvider(const QString& providerId, AbstractSttClient* client);
    void unregisterProvider(const QString& providerId);

    [[nodiscard]] QString activeProviderId() const;
    void setActiveProviderId(const QString& providerId);
    void loadSettings();

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

signals:
    void activeProviderChanged(const QString& providerId);

private:
    void initDefaultProviders();
    void updateActiveClient();

    QString m_activeProviderId {QStringLiteral("groq")};
    QHash<QString, AbstractSttClient*> m_clients;
    AbstractSttClient* m_activeClient = nullptr;
};
