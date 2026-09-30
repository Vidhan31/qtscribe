#include "CloudResponseParser.h"

#include <QHttpHeaders>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

#include <cmath>

using namespace Qt::StringLiterals;

namespace CloudResponseParser {

int parseRetryAfterSeconds(const QByteArray& raw) {
    if (raw.isEmpty()) {
        return 0;
    }
    QString str = QString::fromUtf8(raw).trimmed();
    if (str.endsWith(u's', Qt::CaseInsensitive)) {
        str.chop(1);
    }
    bool ok = false;
    const double val = str.toDouble(&ok);
    if (ok && val > 0.0) {
        return static_cast<int>(std::ceil(val));
    }
    return 0;
}

QString extractApiErrorMessage(const QByteArray& responseBody, const QString& defaultError) {
    if (responseBody.isEmpty()) {
        return defaultError;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(responseBody);
    if (doc.isObject()) {
        const QJsonObject rootObj = doc.object();
        if (rootObj.contains(u"error"_s)) {
            const QJsonValue errVal = rootObj.value(u"error"_s);
            if (errVal.isObject()) {
                const QString msg = errVal.toObject().value(u"message"_s).toString();
                if (!msg.isEmpty()) {
                    return u"Cloud API error: %1"_s.arg(msg);
                }
            } else if (errVal.isString()) {
                return u"Cloud API error: %1"_s.arg(errVal.toString());
            }
        }
    }

    return defaultError.isEmpty() ? u"Invalid response from cloud API"_s : defaultError;
}

CloudApiResponse parseReply(QNetworkReply* reply, qint64 latencyMs) {
    if (!reply) {
        return CloudApiResponse {.errorMessage = u"Null network reply"_s};
    }

    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QNetworkReply::NetworkError netErr = reply->error();
    const QString errorString = reply->errorString();
    const QByteArray responseBody = reply->readAll();

    QByteArray retryAfterRaw = reply->headers().value(QHttpHeaders::WellKnownHeader::RetryAfter).toByteArray();
    if (retryAfterRaw.isEmpty()) {
        retryAfterRaw = reply->rawHeader("retry-after");
    }

    const bool isSuccess = (netErr == QNetworkReply::NoError && httpStatus >= 200 && httpStatus < 300);
    QJsonObject json;
    QString errorMessage;

    if (isSuccess) {
        const QJsonDocument doc = QJsonDocument::fromJson(responseBody);
        if (doc.isObject()) {
            json = doc.object();
        }
    } else {
        errorMessage = extractApiErrorMessage(responseBody, u"Network error: %1"_s.arg(errorString));
    }

    return CloudApiResponse {.httpStatus = httpStatus,
                             .latencyMs = latencyMs,
                             .rawBody = responseBody,
                             .json = json,
                             .errorMessage = errorMessage,
                             .isSuccess = isSuccess,
                             .isRateLimited = (httpStatus == 429),
                             .retryAfterSeconds = parseRetryAfterSeconds(retryAfterRaw),
                             .networkError = netErr};
}

} // namespace CloudResponseParser
