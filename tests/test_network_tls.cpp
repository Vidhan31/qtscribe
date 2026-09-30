#include "GeminiSttClient.h"
#include "GroqSttClient.h"
#include "NetworkTlsHelper.h"

#include <QCoreApplication>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QTest>
#include <QUrl>

using namespace Qt::StringLiterals;

class TestNetworkTls : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName(u"QTranscribeTest"_s);
        QCoreApplication::setApplicationName(u"QTranscribeTest"_s);
    }

    void testDefaultSslConfigurationUntouched() {
        const QSslConfiguration before = QSslConfiguration::defaultConfiguration();

        {
            GroqSttClient groqClient;
            GeminiSttClient geminiClient;
            NetworkTlsHelper::ensureSystemProxyConfigured();
        }

        const QSslConfiguration after = QSslConfiguration::defaultConfiguration();
        QCOMPARE(after.protocol(), before.protocol());
        QCOMPARE(after.peerVerifyMode(), before.peerVerifyMode());
        QCOMPARE(after.allowedNextProtocols(), before.allowedNextProtocols());
    }

    void testPerRequestTlsKeepsHttpFallback() {
        QNetworkRequest request(QUrl(u"https://example.invalid"_s));
        NetworkTlsHelper::applySecureTls(request);

        const QSslConfiguration cfg = request.sslConfiguration();
        QCOMPARE(cfg.protocol(), QSsl::TlsV1_2OrLater);
        QCOMPARE(cfg.peerVerifyMode(), QSslSocket::VerifyPeer);
        QVERIFY(cfg.allowedNextProtocols()
                != QList<QByteArray>({QSslConfiguration::ALPNProtocolHTTP2}));
        QCOMPARE(request.attribute(QNetworkRequest::Http2AllowedAttribute).toBool(), true);
    }

    void testRedactedBodyPreview() {
        const QByteArray shortBody = R"({"error":{"message":"boom"}})";
        QCOMPARE(NetworkTlsHelper::redactedBodyPreview(shortBody), shortBody);

        const QByteArray longBody(1024, 'x');
        const QByteArray preview = NetworkTlsHelper::redactedBodyPreview(longBody);
        QVERIFY(preview.size() < longBody.size());
        QVERIFY(preview.contains("truncated"));
        QVERIFY(preview.startsWith(longBody.first(512)));
    }
};

QTEST_GUILESS_MAIN(TestNetworkTls)
#include "test_network_tls.moc"
