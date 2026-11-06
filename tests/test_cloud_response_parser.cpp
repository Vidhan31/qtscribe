#include "CloudResponseParser.h"
#include "mock_network_access_manager.h"

#include <QCoreApplication>
#include <QTest>

using namespace Qt::StringLiterals;

class TestCloudResponseParser : public QObject {
    Q_OBJECT

private slots:
    void testParseRetryAfterSeconds_data() {
        QTest::addColumn<QByteArray>("raw");
        QTest::addColumn<int>("expected");

        QTest::newRow("empty") << QByteArray() << 0;
        QTest::newRow("zero") << QByteArray("0") << 0;
        QTest::newRow("positive_integer") << QByteArray("5") << 5;
        QTest::newRow("suffix_s") << QByteArray("10s") << 10;
        QTest::newRow("suffix_s_uppercase") << QByteArray("3S") << 3;
        QTest::newRow("fractional_ceil") << QByteArray("1.2") << 2;
        QTest::newRow("fractional_suffix_s") << QByteArray("0.1s") << 1;
        QTest::newRow("negative") << QByteArray("-5") << 0;
        QTest::newRow("whitespace") << QByteArray("  7  ") << 7;
        QTest::newRow("invalid_garbage") << QByteArray("garbage_text") << 0;
        QTest::newRow("http_date_unsupported") << QByteArray("Wed, 21 Oct 2015 07:28:00 GMT") << 0;
    }

    void testParseRetryAfterSeconds() {
        QFETCH(QByteArray, raw);
        QFETCH(int, expected);

        QCOMPARE(CloudResponseParser::parseRetryAfterSeconds(raw), expected);
    }

    void testExtractApiErrorMessage_data() {
        QTest::addColumn<QByteArray>("responseBody");
        QTest::addColumn<QString>("defaultError");
        QTest::addColumn<QString>("expected");

        QTest::newRow("empty_body") << QByteArray() << u"Default Error"_s << u"Default Error"_s;

        QTest::newRow("nested_object_error")
            << QByteArray(R"({"error": {"message": "Rate limit reached"}})")
            << u"Default Error"_s
            << u"Cloud API error: Rate limit reached"_s;

        QTest::newRow("string_error")
            << QByteArray(R"({"error": "Direct error message"})")
            << u"Default Error"_s
            << u"Cloud API error: Direct error message"_s;

        QTest::newRow("empty_nested_message")
            << QByteArray(R"({"error": {"message": ""}})")
            << u"Fallback Error"_s
            << u"Fallback Error"_s;

        QTest::newRow("missing_error_field")
            << QByteArray(R"({"result": "success", "data": 123})")
            << u"Fallback Error"_s
            << u"Fallback Error"_s;

        QTest::newRow("malformed_json")
            << QByteArray("{bad_json_not_terminated")
            << u"Fallback Error"_s
            << u"Fallback Error"_s;

        QTest::newRow("json_array")
            << QByteArray("[1, 2, 3]")
            << u"Fallback Error"_s
            << u"Fallback Error"_s;
    }

    void testExtractApiErrorMessage() {
        QFETCH(QByteArray, responseBody);
        QFETCH(QString, defaultError);
        QFETCH(QString, expected);

        QCOMPARE(CloudResponseParser::extractApiErrorMessage(responseBody, defaultError), expected);
    }

    void testParseReplyNull() {
        const CloudApiResponse res = CloudResponseParser::parseReply(nullptr, 150);
        QVERIFY(!res.isSuccess);
        QCOMPARE(res.errorMessage, u"Null network reply"_s);
        QCOMPARE(res.latencyMs, 0);
    }

    void testParseReplySuccess() {
        MockNetworkReply reply(200, R"({"transcript": "hello"})");
        QTRY_VERIFY(reply.isFinished());

        const CloudApiResponse res = CloudResponseParser::parseReply(&reply, 42);
        QVERIFY(res.isSuccess);
        QCOMPARE(res.httpStatus, 200);
        QCOMPARE(res.latencyMs, 42);
        QVERIFY(!res.isRateLimited);
        QCOMPARE(res.retryAfterSeconds, 0);
        QCOMPARE(res.json.value(u"transcript"_s).toString(), u"hello"_s);
        QVERIFY(res.errorMessage.isEmpty());
    }

    void testParseReplyRateLimited429() {
        MockNetworkReply reply(429, R"({"error": {"message": "Too many requests"}})",
                               {{"retry-after", "5"}});
        QTRY_VERIFY(reply.isFinished());

        const CloudApiResponse res = CloudResponseParser::parseReply(&reply, 75);
        QVERIFY(!res.isSuccess);
        QCOMPARE(res.httpStatus, 429);
        QVERIFY(res.isRateLimited);
        QCOMPARE(res.retryAfterSeconds, 5);
        QCOMPARE(res.errorMessage, u"Cloud API error: Too many requests"_s);
    }

    void testParseReplyServerError500() {
        MockNetworkReply reply(500, R"({"error": {"message": "Internal cluster crash"}})",
                               {}, QNetworkReply::InternalServerError, u"Internal Server Error"_s);
        QTRY_VERIFY(reply.isFinished());

        const CloudApiResponse res = CloudResponseParser::parseReply(&reply, 110);
        QVERIFY(!res.isSuccess);
        QCOMPARE(res.httpStatus, 500);
        QVERIFY(!res.isRateLimited);
        QCOMPARE(res.networkError, QNetworkReply::InternalServerError);
        QCOMPARE(res.errorMessage, u"Cloud API error: Internal cluster crash"_s);
    }
};

QTEST_GUILESS_MAIN(TestCloudResponseParser)
#include "test_cloud_response_parser.moc"
