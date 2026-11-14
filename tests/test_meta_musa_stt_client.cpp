#include "ApiKeyStore.h"
#include "MetaMusaSttClient.h"
#include "AbstractCloudSttClient.h"
#include "mock_network_access_manager.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

class TestMetaMusaSttClient : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName(u"QTranscribeTest"_s);
        QCoreApplication::setApplicationName(u"QTranscribeTest_MetaMusa"_s);
    }

    void cleanup() {
        QSettings settings;
        settings.remove(u"MetaMusa/Model"_s);
        settings.remove(u"MetaMusa/Keywords"_s);
        settings.remove(u"Cloud/Providers/MetaMusa/ApiKey"_s);
        settings.remove(u"MetaMusaTest/ApiKey"_s);
        settings.sync();
    }

    void testDefaultProperties() {
        MetaMusaSttClient client;
        QCOMPARE(client.selectedModel(), u"muse-voice-transcribe-1.0"_s);
        QCOMPARE(client.keywords(), QString());
        QVERIFY(client.handlesSmartFormatting());
        QCOMPARE(client.isReady(), false);
        QCOMPARE(client.isBusy(), false);
    }

    void testPropertyMutationsAndPersistence() {
        MetaMusaSttClient client;

        QSignalSpy spyModel(&client, &MetaMusaSttClient::selectedModelChanged);
        QSignalSpy spyKeywords(&client, &MetaMusaSttClient::keywordsChanged);

        client.setSelectedModel(u"muse-voice-transcribe-1.0"_s);
        client.setKeywords(u"QtScribe, Wayland, PyTorch"_s);

        QCOMPARE(client.selectedModel(), u"muse-voice-transcribe-1.0"_s);
        QCOMPARE(client.keywords(), u"QtScribe, Wayland, PyTorch"_s);

        QCOMPARE(spyKeywords.count(), 1);

        QSettings settings;
        QCOMPARE(settings.value(u"MetaMusa/Keywords"_s).toString(), u"QtScribe, Wayland, PyTorch"_s);
    }

    void testApiKeyDelegationAndActivateLazyLoad() {
        QSettings settings;
        settings.setValue(u"MetaMusaTest/ApiKey"_s, u"LLM_test_key_12345"_s);
        settings.sync();

        MetaMusaSttClient client;
        auto* store = new ApiKeyStore(&client);
        store->setStorageKeys(u"QTranscribeTestService"_s, u"test_meta_musa_api_key"_s, u"MetaMusaTest/ApiKey"_s);
        client.setKeyStore(store);

        QCOMPARE(client.isApiKeySet(), false);
        QCOMPARE(client.isReady(), false);

        client.activate();

        QTRY_VERIFY(client.isReady());
        QTRY_COMPARE(client.apiKey(), u"LLM_test_key_12345"_s);
        QCOMPARE(client.isApiKeySet(), true);
    }

    void testSetAndClearApiKey() {
        MetaMusaSttClient client;
        auto* store = new ApiKeyStore(&client);
        store->setStorageKeys(u"QTranscribeTestService"_s, u"test_meta_musa_api_key"_s, u"MetaMusaTest/ApiKey"_s);
        client.setKeyStore(store);

        QSignalSpy spyKeyChanged(&client, &MetaMusaSttClient::apiKeyChanged);
        QSignalSpy spyKeySetChanged(&client, &AbstractCloudSttClient::apiKeySetChanged);

        client.setApiKey(u"  LLM_direct_assigned_key  "_s);

        QCOMPARE(client.apiKey(), u"LLM_direct_assigned_key"_s);
        QCOMPARE(client.isApiKeySet(), true);
        QCOMPARE(client.isReady(), true);
        QCOMPARE(spyKeyChanged.count(), 1);
        QCOMPARE(spyKeySetChanged.count(), 1);

        client.setApiKey(QString());
        QCOMPARE(client.apiKey(), QString());
        QCOMPARE(client.isApiKeySet(), false);
        QCOMPARE(client.isReady(), false);
        QCOMPARE(spyKeyChanged.count(), 2);
        QCOMPARE(spyKeySetChanged.count(), 2);
    }

    void testTranscribeEmptyAudioFailsImmediately() {
        MetaMusaSttClient client;
        client.setApiKey(u"valid_key"_s);

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);
        client.transcribe(QByteArray());

        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).toString(), u"No audio data to transcribe"_s);
        QCOMPARE(client.lastError(), u"No audio data to transcribe"_s);
    }

    void testTranscribeWithoutApiKeyFails() {
        MetaMusaSttClient client;

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);
        client.transcribe(QByteArray("fakeAudioPayload"));

        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(client.errorCategory(), AbstractCloudSttClient::ErrorCategory::InvalidApiKey);
        QVERIFY(!client.isApiKeySet());
        QVERIFY(!client.isReady());
    }

    void testTranscribeSuccessTranscript() {
        MetaMusaSttClient client;
        client.setApiKey(u"LLM_valid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 200;
        mockNam->nextResponseData = R"({"sessionId": "12345", "transcript": "Hello world from Meta Musa."})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy readySpy(&client, &AbstractSttClient::transcriptionReady);
        QSignalSpy busySpy(&client, &AbstractSttClient::busyChanged);

        client.transcribe(QByteArray("RIFFfakeWavData"));

        QTRY_COMPARE(readySpy.count(), 1);
        QCOMPARE(readySpy.first().at(0).toString(), u"Hello world from Meta Musa."_s);
        QVERIFY(!client.isBusy());
        QCOMPARE(mockNam->requestCount, 1);
    }

    void testTranscribeEmptyTranscriptReturnsError() {
        MetaMusaSttClient client;
        client.setApiKey(u"LLM_valid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 200;
        mockNam->nextResponseData = R"({"sessionId": "12345", "transcript": ""})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);

        client.transcribe(QByteArray("RIFFfakeWavData"));

        QTRY_COMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).toString(), u"Meta Musa API returned empty transcription"_s);
        QCOMPARE(client.errorCategory(), AbstractCloudSttClient::ErrorCategory::GeneralError);
    }

    void testTranscribeBillingError402() {
        MetaMusaSttClient client;
        client.setApiKey(u"LLM_valid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 402;
        mockNam->nextResponseData = R"({"error": {"message": "Billing account is not configured", "type": "billing_error", "code": "billing_not_configured"}})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);

        client.transcribe(QByteArray("RIFFfakeWavData"));

        QTRY_COMPARE(errorSpy.count(), 1);
        QCOMPARE(client.errorCategory(), AbstractCloudSttClient::ErrorCategory::BillingError);
    }

    void testTranscribeUnauthorized401() {
        MetaMusaSttClient client;
        client.setApiKey(u"LLM_invalid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 401;
        mockNam->nextResponseData = R"({"error": {"message": "Invalid API key provided", "type": "authentication_error", "code": "invalid_api_key"}})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy keyInvalidSpy(&client, &AbstractCloudSttClient::isApiKeyInvalidChanged);
        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);

        client.transcribe(QByteArray("RIFFfakeWavData"));

        QTRY_VERIFY(client.isApiKeyInvalid());
        QCOMPARE(client.errorCategory(), AbstractCloudSttClient::ErrorCategory::InvalidApiKey);
        QCOMPARE(errorSpy.count(), 1);
    }

    void testTranscribeRateLimit429WithRetryAfter() {
        MetaMusaSttClient client;
        client.setApiKey(u"LLM_valid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 429;
        mockNam->nextHeaders.append({QByteArray("retry-after"), QByteArray("15")});
        mockNam->nextResponseData = R"({"error": {"message": "Rate limit exceeded", "type": "rate_limit_error", "code": "rate_limit_exceeded"}})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);
        QSignalSpy rateSpy(&client, &AbstractCloudSttClient::isRateLimitedChanged);

        client.transcribe(QByteArray("RIFFfakeWavData"));

        QTRY_VERIFY(client.isRateLimited());
        QCOMPARE(client.retrySecondsRemaining(), 15);
        QCOMPARE(client.errorCategory(), AbstractCloudSttClient::ErrorCategory::RateLimited);
        QVERIFY(errorSpy.count() >= 1);
    }

    void testCancelResetsState() {
        MetaMusaSttClient client;
        client.setApiKey(u"LLM_valid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        client.setNetworkAccessManager(mockNam);

        client.cancel();
        QCOMPARE(client.isBusy(), false);
        QCOMPARE(client.isCancelled(), true);
    }

    void testKeywordsSentInMultipart() {
        MetaMusaSttClient client;
        client.setApiKey(u"LLM_valid_key"_s);
        client.setKeywords(u"QtScribe, Wayland, Voice"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 200;
        mockNam->nextResponseData = R"({"sessionId": "123", "transcript": "Testing keywords."})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy readySpy(&client, &AbstractSttClient::transcriptionReady);

        client.transcribe(QByteArray("RIFFfakeWavData"));

        QTRY_COMPARE(readySpy.count(), 1);
        QCOMPARE(mockNam->requestCount, 1);
        QVERIFY(mockNam->lastOutgoingData.contains("QtScribe"));
        QVERIFY(mockNam->lastOutgoingData.contains("Wayland"));
        QVERIFY(mockNam->lastOutgoingData.contains("Voice"));
    }
};

QTEST_GUILESS_MAIN(TestMetaMusaSttClient)
#include "test_meta_musa_stt_client.moc"
