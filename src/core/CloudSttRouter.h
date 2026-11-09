#pragma once

#include "AbstractCloudSttClient.h"

#include <QHash>
#include <QString>

class SettingsManager;

class CloudSttRouter : public AbstractCloudSttClient {
    Q_OBJECT

    Q_PROPERTY(QString activeProviderId READ activeProviderId WRITE setActiveProviderId NOTIFY activeProviderChanged FINAL)

public:
    explicit CloudSttRouter(QObject* parent = nullptr);
    ~CloudSttRouter() override;

    static CloudSttRouter* instance();

    void registerProvider(const QString& providerId, AbstractCloudSttClient* client);
    void unregisterProvider(const QString& providerId);

    void setSettingsManager(SettingsManager* settings);
    [[nodiscard]] SettingsManager* settingsManager() const noexcept;

    [[nodiscard]] QString activeProviderId() const;
    void setActiveProviderId(const QString& providerId);
    void loadSettings();

    AbstractCloudSttClient* activeCloudClient() const;
    AbstractCloudSttClient* activeClient() const;

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
    QHash<QString, AbstractCloudSttClient*> m_clients;
    AbstractCloudSttClient* m_activeClient = nullptr;
    SettingsManager* m_settingsManager = nullptr;
};
