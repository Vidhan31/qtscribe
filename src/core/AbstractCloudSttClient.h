#pragma once

#include "AbstractSttClient.h"

class AbstractCloudSttClient : public AbstractSttClient {
    Q_OBJECT

    Q_PROPERTY(bool apiKeySet READ isApiKeySet NOTIFY apiKeySetChanged FINAL)
    Q_PROPERTY(bool isApiKeyInvalid READ isApiKeyInvalid NOTIFY isApiKeyInvalidChanged FINAL)
    Q_PROPERTY(bool isRateLimited READ isRateLimited NOTIFY isRateLimitedChanged FINAL)
    Q_PROPERTY(int retrySecondsRemaining READ retrySecondsRemaining NOTIFY retrySecondsRemainingChanged FINAL)
    Q_PROPERTY(AbstractCloudSttClient::ErrorCategory errorCategory READ errorCategory NOTIFY errorCategoryChanged FINAL)

public:
    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum class ErrorCategory { None, InvalidApiKey, NetworkOffline, RateLimited, BillingError, GeneralError };
    Q_ENUM(ErrorCategory)

    explicit AbstractCloudSttClient(QObject* parent = nullptr);
    ~AbstractCloudSttClient() override = default;

    virtual bool isApiKeySet() const;
    virtual bool isApiKeyInvalid() const;
    virtual bool isRateLimited() const;
    virtual int retrySecondsRemaining() const;
    virtual ErrorCategory errorCategory() const;

signals:
    void apiKeySetChanged();
    void isApiKeyInvalidChanged();
    void isRateLimitedChanged();
    void retrySecondsRemainingChanged();
    void errorCategoryChanged();
};
