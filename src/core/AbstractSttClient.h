#pragma once

#include <QByteArray>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariantMap>

#include <cstdint>

class AbstractSttClient : public QObject {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged FINAL)
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged FINAL)
    Q_PROPERTY(bool apiKeySet READ isApiKeySet NOTIFY apiKeySetChanged FINAL)
    Q_PROPERTY(bool isApiKeyInvalid READ isApiKeyInvalid NOTIFY isApiKeyInvalidChanged FINAL)
    Q_PROPERTY(bool isRateLimited READ isRateLimited NOTIFY isRateLimitedChanged FINAL)
    Q_PROPERTY(int retrySecondsRemaining READ retrySecondsRemaining NOTIFY retrySecondsRemainingChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(AbstractSttClient::ErrorCategory errorCategory READ errorCategory NOTIFY errorCategoryChanged FINAL)

public:
    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum class ErrorCategory { None, InvalidApiKey, NetworkOffline, RateLimited, GeneralError };
    Q_ENUM(ErrorCategory)

    explicit AbstractSttClient(QObject* parent = nullptr);
    ~AbstractSttClient() override = default;

    virtual void transcribe(const QByteArray& wavData) = 0;
    virtual void cancel() = 0;
    virtual void retryLast();
    virtual bool isReady() const = 0;
    virtual bool isBusy() const = 0;

    virtual void activate();
    virtual void deactivate();

    virtual QString lastError() const;
    virtual bool handlesSmartFormatting() const;

    virtual bool isApiKeySet() const;
    virtual bool isApiKeyInvalid() const;
    virtual bool isRateLimited() const;
    virtual int retrySecondsRemaining() const;
    virtual ErrorCategory errorCategory() const;

signals:
    void transcriptionReady(const QString& text);
    void errorOccurred(const QString& error);
    void busyChanged();
    void readyChanged();
    void apiKeySetChanged();
    void isApiKeyInvalidChanged();
    void isRateLimitedChanged();
    void retrySecondsRemainingChanged();
    void errorCategoryChanged();
    void lastErrorChanged();
};
