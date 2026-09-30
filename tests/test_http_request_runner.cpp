#include <QCoreApplication>
#include <QNetworkReply>
#include <QTest>

#include <array>

#include "HttpRequestRunner.h"

class TestHttpRequestRunner : public QObject {
    Q_OBJECT

private slots:
    void testRequestPolicySuccess() {
        RequestPolicy policy;
        CloudApiResponse res;
        res.isSuccess = true;
        res.httpStatus = 200;

        QCOMPARE(policy.isTransientError(res), false);
        QCOMPARE(policy.shouldRetry(res, 0, false), false);
    }

    void testRequestPolicyTransientRateLimit() {
        RequestPolicy policy;
        CloudApiResponse res;
        res.isSuccess = false;
        res.httpStatus = 429;
        res.isRateLimited = true;
        res.retryAfterSeconds = 2;

        QCOMPARE(policy.isTransientError(res), true);
        QCOMPARE(policy.shouldRetry(res, 0, false), true);
        QCOMPARE(policy.calculateRetryDelayMs(res), 2000);

        res.retryAfterSeconds = 10;
        QCOMPARE(policy.isTransientError(res), false);
        QCOMPARE(policy.shouldRetry(res, 0, false), false);
    }

    void testRequestPolicyUnauthorized() {
        RequestPolicy policy;
        CloudApiResponse res;
        res.isSuccess = false;
        res.httpStatus = 401;

        QCOMPARE(policy.isTransientError(res), false);
        QCOMPARE(policy.shouldRetry(res, 0, false), false);
    }

    void testRequestPolicyServerErrors() {
        RequestPolicy policy;
        const auto serverErrors = std::to_array({500, 502, 503, 504, 599});

        for (int code : serverErrors) {
            CloudApiResponse res;
            res.isSuccess = false;
            res.httpStatus = code;

            QCOMPARE(policy.isTransientError(res), true);
            QCOMPARE(policy.shouldRetry(res, 0, false), true);
            QCOMPARE(policy.calculateRetryDelayMs(res), 1000);
        }
    }

    void testRequestPolicyNetworkErrors() {
        RequestPolicy policy;

        const auto transientErrors = std::to_array({
            QNetworkReply::RemoteHostClosedError,
            QNetworkReply::TemporaryNetworkFailureError,
            QNetworkReply::TimeoutError,
            QNetworkReply::NetworkSessionFailedError
        });

        for (auto err : transientErrors) {
            CloudApiResponse res;
            res.isSuccess = false;
            res.networkError = err;

            QCOMPARE(policy.isTransientError(res), true);
            QCOMPARE(policy.shouldRetry(res, 0, false), true);
        }

        const auto nonTransientErrors = std::to_array({
            QNetworkReply::HostNotFoundError,
            QNetworkReply::ConnectionRefusedError,
            QNetworkReply::OperationCanceledError,
            QNetworkReply::AuthenticationRequiredError
        });

        for (auto err : nonTransientErrors) {
            CloudApiResponse res;
            res.isSuccess = false;
            res.networkError = err;

            QCOMPARE(policy.isTransientError(res), false);
            QCOMPARE(policy.shouldRetry(res, 0, false), false);
        }
    }

    void testRequestPolicyClientErrors() {
        RequestPolicy policy;
        const auto clientErrors = std::to_array({400, 401, 403, 404, 422});

        for (int code : clientErrors) {
            CloudApiResponse res;
            res.isSuccess = false;
            res.httpStatus = code;

            QCOMPARE(policy.isTransientError(res), false);
            QCOMPARE(policy.shouldRetry(res, 0, false), false);
        }
    }

    void testRequestPolicyRetryLimitsAndCancellation() {
        RequestPolicy policy;
        CloudApiResponse res;
        res.isSuccess = false;
        res.httpStatus = 503;

        QCOMPARE(policy.shouldRetry(res, 0, false), true);
        QCOMPARE(policy.shouldRetry(res, 1, false), false);
        QCOMPARE(policy.shouldRetry(res, 0, true), false);
    }

    void testHttpRequestRunnerLifecycle() {
        HttpRequestRunner runner;
        QCOMPARE(runner.isBusy(), false);
        QCOMPARE(runner.isCancelled(), false);
        QCOMPARE(runner.retryCount(), 0);
        QVERIFY(runner.currentReply().isNull());

        runner.prepareNewRequest();
        QCOMPARE(runner.isBusy(), true);
        QCOMPARE(runner.isCancelled(), false);
        QCOMPARE(runner.retryCount(), 0);

        CloudApiResponse transientRes;
        transientRes.isSuccess = false;
        transientRes.httpStatus = 500;

        QCOMPARE(runner.shouldRetry(transientRes), true);
        QCOMPARE(runner.calculateRetryDelayMs(transientRes), 1000);

        runner.incrementRetryCount();
        QCOMPARE(runner.retryCount(), 1);
        QCOMPARE(runner.shouldRetry(transientRes), false);

        runner.reset();
        QCOMPARE(runner.isBusy(), false);
        QCOMPARE(runner.retryCount(), 0);

        runner.prepareNewRequest();
        runner.cancel();
        QCOMPARE(runner.isBusy(), false);
        QCOMPARE(runner.isCancelled(), true);
        QCOMPARE(runner.shouldRetry(transientRes), false);
    }
};

QTEST_GUILESS_MAIN(TestHttpRequestRunner)
#include "test_http_request_runner.moc"
