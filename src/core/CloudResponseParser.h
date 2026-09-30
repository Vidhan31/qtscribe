#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QString>

struct CloudApiResponse {
    int httpStatus = 0;
    qint64 latencyMs = 0;
    QByteArray rawBody = {};
    QJsonObject json = {};
    QString errorMessage = {};
    bool isSuccess = false;
    bool isRateLimited = false;
    int retryAfterSeconds = 0;
    QNetworkReply::NetworkError networkError = QNetworkReply::NoError;
};

namespace CloudResponseParser {
/**
 * @brief Parses an HTTP response from cloud API without taking ownership of the QNetworkReply.
 * The caller remains responsible for the lifetime and deletion (e.g. via deleteLater()) of reply.
 */
CloudApiResponse parseReply(QNetworkReply* reply, qint64 latencyMs);
int parseRetryAfterSeconds(const QByteArray& raw);
QString extractApiErrorMessage(const QByteArray& responseBody, const QString& defaultError = QString());

} // namespace CloudResponseParser
