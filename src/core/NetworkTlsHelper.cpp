#include "NetworkTlsHelper.h"

#include <QNetworkProxyFactory>
#include <QSslSocket>

#include <mutex>

namespace NetworkTlsHelper {

void ensureSystemProxyConfigured() {
    static std::once_flag proxyFlag;
    std::call_once(proxyFlag, []() { QNetworkProxyFactory::setUseSystemConfiguration(true); });
}

QSslConfiguration secureTlsConfiguration() {
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::TlsV1_2OrLater);
    config.setPeerVerifyMode(QSslSocket::VerifyPeer);
    return config;
}

void applySecureTls(QNetworkRequest& request) {
    request.setSslConfiguration(secureTlsConfiguration());
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
}

QByteArray redactedBodyPreview(const QByteArray& body, qsizetype maxBytes) {
    if (body.size() <= maxBytes) {
        return body;
    }
    QByteArray preview = body.first(maxBytes);
    preview += "...[truncated, " + QByteArray::number(body.size()) + " bytes total]";
    return preview;
}

} // namespace NetworkTlsHelper
