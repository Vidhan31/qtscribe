#include "ApiKeyStore.h"
#include "GeminiSttClient.h"
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

class TestGeminiSttClient : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName(u"QTranscribeTest"_s);
        QCoreApplication::setApplicationName(u"QTranscribeTest_Gemini"_s);
    }

    void cleanup() {
        QSettings settings;
        settings.remove(u"Gemini/Model"_s);
        settings.remove(u"Gemini/Mode"_s);
        settings.remove(u"Gemini/CustomVocabulary"_s);
        settings.remove(u"Cloud/Providers/Gemini/ApiKey"_s);
        settings.remove(u"GeminiTest/ApiKey"_s);
        settings.sync();
    }

    void testDefaultProperties() {
        GeminiSttClient client;
        QCOMPARE(client.selectedModel(), u"gemini-3.5-transcribe"_s);
        QCOMPARE(client.mode(), u"smart"_s);
        QCOMPARE(client.customVocabulary(), QString());
        QVERIFY(client.handlesSmartFormatting());
        QCOMPARE(client.isReady(), false);
        QCOMPARE(client.isBusy(), false);
    }

    void testPropertyMutationsAndPersistence() {
        GeminiSttClient client;

        QSignalSpy spyModel(&client, &GeminiSttClient::selectedModelChanged);
        QSignalSpy spyMode(&client, &GeminiSttClient::modeChanged);
        QSignalSpy spyVocab(&client, &GeminiSttClient::customVocabularyChanged);

        client.setSelectedModel(u"gemini-3.5-transcribe"_s);
        client.setMode(u"verbatim"_s);
        client.setCustomVocabulary(u"QtScribe, Wayland, CMake"_s);

        QCOMPARE(client.selectedModel(), u"gemini-3.5-transcribe"_s);
        QCOMPARE(client.mode(), u"verbatim"_s);
        QCOMPARE(client.customVocabulary(), u"QtScribe, Wayland, CMake"_s);
        QCOMPARE(client.handlesSmartFormatting(), false);

        QCOMPARE(spyMode.count(), 1);
        QCOMPARE(spyVocab.count(), 1);

        QSettings settings;
        QCOMPARE(settings.value(u"Gemini/Mode"_s).toString(), u"verbatim"_s);
        QCOMPARE(settings.value(u"Gemini/CustomVocabulary"_s).toString(), u"QtScribe, Wayland, CMake"_s);
    }

    void testApiKeyDelegationAndActivateLazyLoad() {
        QSettings settings;
        settings.setValue(u"GeminiTest/ApiKey"_s, u"AIzaSyTest_stt_key_789"_s);
        settings.sync();

        GeminiSttClient client;
        auto* store = new ApiKeyStore(&client);
        store->setStorageKeys(u"QTranscribeTestService"_s, u"test_gemini_api_key"_s, u"GeminiTest/ApiKey"_s);
        client.setKeyStore(store);

        QCOMPARE(client.isApiKeySet(), false);
        QCOMPARE(client.isReady(), false);

        client.activate();

        QTRY_VERIFY(client.isReady());
        QTRY_COMPARE(client.apiKey(), u"AIzaSyTest_stt_key_789"_s);
        QCOMPARE(client.isApiKeySet(), true);
    }

    void testSetAndClearApiKey() {
        GeminiSttClient client;
        auto* store = new ApiKeyStore(&client);
        store->setStorageKeys(u"QTranscribeTestService"_s, u"test_gemini_api_key"_s, u"GeminiTest/ApiKey"_s);
        client.setKeyStore(store);

        QSignalSpy spyKeyChanged(&client, &GeminiSttClient::apiKeyChanged);
        QSignalSpy spyKeySetChanged(&client, &AbstractCloudSttClient::apiKeySetChanged);

        client.setApiKey(u"  AIzaSyTest_new_active_key_12345  "_s);

        QCOMPARE(client.apiKey(), u"AIzaSyTest_new_active_key_12345"_s);
        QCOMPARE(client.isApiKeySet(), true);
        QCOMPARE(client.isReady(), true);
        QCOMPARE(spyKeyChanged.count(), 1);
        QCOMPARE(spyKeySetChanged.count(), 1);

        client.setApiKey(QString());

        QCOMPARE(client.apiKey(), QString());
        QCOMPARE(client.isApiKeySet(), false);
        QCOMPARE(client.isReady(), false);

        QSettings settings;
        QVERIFY(settings.value(u"GeminiTest/ApiKey"_s).toString().isEmpty());
    }

    void testInjectCustomKeyStore() {
        GeminiSttClient client;
        auto* customStore = new ApiKeyStore(&client);
        customStore->setStorageKeys(u"CustomSvc"_s, u"custom_key"_s, u"Cloud/Providers/Gemini/ApiKey"_s);

        client.setKeyStore(customStore);
        QCOMPARE(client.keyStore(), customStore);

        QSignalSpy spyKeyChanged(&client, &GeminiSttClient::apiKeyChanged);
        customStore->setApiKey(u"AIzaSyTest_injected_store_key"_s);

        QCOMPARE(client.apiKey(), u"AIzaSyTest_injected_store_key"_s);
        QCOMPARE(client.isApiKeySet(), true);
        QCOMPARE(spyKeyChanged.count(), 1);
    }

    void testTranscribeEmptyAudioFailsImmediately() {
        GeminiSttClient client;
        client.setApiKey(u"valid_key"_s);

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);
        client.transcribe(QByteArray());

        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).toString(), u"No audio data to transcribe"_s);
        QCOMPARE(client.lastError(), u"No audio data to transcribe"_s);
    }

    void testTranscribeWithoutApiKeyFails() {
        GeminiSttClient client;

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);
        client.transcribe(QByteArray("fakeAudioPayload"));

        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).toString(), u"Gemini API key is not set"_s);
        QCOMPARE(client.errorCategory(), AbstractCloudSttClient::ErrorCategory::InvalidApiKey);
        QVERIFY(!client.isApiKeySet());
        QVERIFY(!client.isReady());
    }

    void testTranscribeSuccessOutputText() {
        GeminiSttClient client;
        client.setApiKey(u"valid_gemini_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 200;
        mockNam->nextResponseData = R"({"output_text": "Hello world from Gemini Transcribe."})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy readySpy(&client, &AbstractSttClient::transcriptionReady);
        QSignalSpy busySpy(&client, &AbstractSttClient::busyChanged);

        client.transcribe(QByteArray("RIFFfakeWavData"));

        QTRY_COMPARE(readySpy.count(), 1);
        QCOMPARE(readySpy.first().at(0).toString(), u"Hello world from Gemini Transcribe."_s);
        QVERIFY(!client.isBusy());
        QCOMPARE(mockNam->requestCount, 1);
    }

    void testTranscribeSuccessStepsFallback() {
        GeminiSttClient client;
        client.setApiKey(u"valid_gemini_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 200;
        mockNam->nextResponseData = R"({
            "steps": [{
                "type": "model_output",
                "content": [{"type": "text", "text": "Step text transcription"}]
            }]
        })";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy readySpy(&client, &AbstractSttClient::transcriptionReady);

        client.transcribe(QByteArray("RIFFfakeWavData"));

        QTRY_COMPARE(readySpy.count(), 1);
        QCOMPARE(readySpy.first().at(0).toString(), u"Step text transcription"_s);
    }

    void testTranscribeRateLimit429WithRetryAfter() {
        GeminiSttClient client;
        client.setApiKey(u"valid_gemini_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 429;
        mockNam->nextHeaders.append({QByteArray("retry-after"), QByteArray("10")});
        mockNam->nextResponseData = R"({"error": {"message": "Resource has been exhausted (e.g. check quota)."}})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);
        QSignalSpy rateSpy(&client, &AbstractCloudSttClient::isRateLimitedChanged);

        client.transcribe(QByteArray("RIFFfakeWavData"));

        QTRY_VERIFY(client.isRateLimited());
        QCOMPARE(client.retrySecondsRemaining(), 10);
        QCOMPARE(client.errorCategory(), AbstractCloudSttClient::ErrorCategory::RateLimited);
        QVERIFY(errorSpy.count() >= 1);
    }

    void testTranscribeUnauthorized401() {
        GeminiSttClient client;
        client.setApiKey(u"invalid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 401;
        mockNam->nextResponseData = R"({"error": {"message": "API key not valid. Please pass a valid API key."}})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy keyInvalidSpy(&client, &AbstractCloudSttClient::isApiKeyInvalidChanged);
        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);

        client.transcribe(QByteArray("RIFFfakeWavData"));

        QTRY_VERIFY(client.isApiKeyInvalid());
        QCOMPARE(client.errorCategory(), AbstractCloudSttClient::ErrorCategory::InvalidApiKey);
        QCOMPARE(errorSpy.count(), 1);
    }

    void testTranscribeMalformedJsonFallback() {
        GeminiSttClient client;
        client.setApiKey(u"valid_gemini_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 200;
        mockNam->nextResponseData = "not_valid_json_payload";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);

        client.transcribe(QByteArray("RIFFfakeWavData"));

        QTRY_COMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).toString(), u"Gemini API returned empty transcription"_s);
    }

    void testCancelResetsState() {
        GeminiSttClient client;
        client.setApiKey(u"valid_gemini_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        client.setNetworkAccessManager(mockNam);

        QSignalSpy spyBusy(&client, &GeminiSttClient::busyChanged);

        client.cancel();
        QCOMPARE(client.isBusy(), false);
        QCOMPARE(client.isCancelled(), true);
    }
};

QTEST_GUILESS_MAIN(TestGeminiSttClient)
#include "test_gemini_stt_client.moc"
