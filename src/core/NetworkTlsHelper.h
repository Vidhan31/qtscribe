#pragma once

#include <QByteArray>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QtTypes>

namespace NetworkTlsHelper {

void ensureSystemProxyConfigured();
[[nodiscard]] QSslConfiguration secureTlsConfiguration();
void applySecureTls(QNetworkRequest& request);
[[nodiscard]] QByteArray redactedBodyPreview(const QByteArray& body, qsizetype maxBytes = 512);

} // namespace NetworkTlsHelper
