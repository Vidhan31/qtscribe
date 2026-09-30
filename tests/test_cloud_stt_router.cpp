#include "AbstractSttClient.h"
#include "CloudProviderModel.h"
#include "CloudSttRouter.h"

#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

class FakeMockCloudSttClient : public AbstractSttClient {
    Q_OBJECT

public:
    explicit FakeMockCloudSttClient(bool smart = false, QObject* parent = nullptr)
        : AbstractSttClient(parent)
        , m_smart(smart) { }

    void transcribe(const QByteArray& wavData) override {
        m_lastWavData = wavData;
        m_transcribeCallCount++;
        emit transcriptionReady(m_mockText);
    }

    void cancel() override {
        m_cancelCallCount++;
    }

    void retryLast() override {
        m_retryCallCount++;
    }

    bool handlesSmartFormatting() const override {
        return m_smart;
    }

    bool isReady() const override {
        return m_apiKeySet;
    }

    bool isBusy() const override {
        return m_busy;
    }

    bool isApiKeySet() const override {
        return m_apiKeySet;
    }

    bool isApiKeyInvalid() const override {
        return m_apiKeySet && m_errorCategory == ErrorCategory::InvalidApiKey;
    }

    bool isRateLimited() const override {
        return m_errorCategory == ErrorCategory::RateLimited && m_retrySecondsRemaining > 0;
    }

    int retrySecondsRemaining() const override {
        return m_retrySecondsRemaining;
    }

    ErrorCategory errorCategory() const override {
        return m_errorCategory;
    }

    QString lastError() const override {
        return m_lastError;
    }

    void setMockApiKeySet(bool set) {
        if (m_apiKeySet != set) {
            m_apiKeySet = set;
            emit apiKeySetChanged();
            emit readyChanged();
        }
    }

    void triggerError(const QString& error, ErrorCategory category) {
        m_lastError = error;
        m_errorCategory = category;
        emit lastErrorChanged();
        emit errorCategoryChanged();
        if (category == ErrorCategory::InvalidApiKey) {
            emit isApiKeyInvalidChanged();
        }
        emit errorOccurred(error);
    }

    void setMockRetrySeconds(int seconds) {
        if (m_retrySecondsRemaining != seconds) {
            m_retrySecondsRemaining = seconds;
            emit retrySecondsRemainingChanged();
            if (m_errorCategory == ErrorCategory::RateLimited) {
                emit isRateLimitedChanged();
            }
        }
    }

    void setMockBusy(bool busy) {
        if (m_busy != busy) {
            m_busy = busy;
            emit busyChanged();
        }
    }

    bool m_smart = false;
    bool m_apiKeySet = false;
    bool m_busy = false;
    int m_transcribeCallCount = 0;
    int m_cancelCallCount = 0;
    int m_retryCallCount = 0;
    int m_retrySecondsRemaining = 0;
    ErrorCategory m_errorCategory = ErrorCategory::None;
    QString m_lastError;
    QString m_mockText = u"Mock transcription result"_s;
    QByteArray m_lastWavData;
};

class TestCloudSttRouter : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName(u"QTranscribeTestRouter"_s);
        QCoreApplication::setApplicationName(u"QTranscribeTestRouter"_s);
    }

    void cleanup() {
        QSettings settings;
        settings.remove(u"Cloud/ActiveProvider"_s);
        settings.sync();
    }

    void testRoutingAndProviderSwitching() {
        cleanup();
        CloudProviderModel model;
        model.setActiveProviderId(u"groq"_s);
        CloudSttRouter router;
        router.setCloudProviderModel(&model);

        FakeMockCloudSttClient groqClient(false);
        groqClient.m_mockText = u"Groq result"_s;
        groqClient.setMockApiKeySet(true);

        FakeMockCloudSttClient geminiClient(true);
        geminiClient.m_mockText = u"Gemini result"_s;
        geminiClient.setMockApiKeySet(true);

        router.registerProvider(u"groq"_s, &groqClient);
        router.registerProvider(u"gemini"_s, &geminiClient);

        QCOMPARE(model.activeProviderId(), u"groq"_s);
        QCOMPARE(router.activeClient(), &groqClient);
        QCOMPARE(router.activeCloudClient(), &groqClient);
        QCOMPARE(router.handlesSmartFormatting(), false);
        QCOMPARE(router.isReady(), true);
        QCOMPARE(router.isApiKeySet(), true);

        QSignalSpy spyTransReady(&router, &AbstractSttClient::transcriptionReady);

        router.transcribe(QByteArray("fakeAudio"));
        QCOMPARE(groqClient.m_transcribeCallCount, 1);
        QCOMPARE(geminiClient.m_transcribeCallCount, 0);
        QCOMPARE(spyTransReady.count(), 1);
        QCOMPARE(spyTransReady.at(0).at(0).toString(), u"Groq result"_s);

        model.setActiveProviderId(u"gemini"_s);
        QCOMPARE(router.activeClient(), &geminiClient);
        QCOMPARE(router.activeCloudClient(), &geminiClient);
        QCOMPARE(router.handlesSmartFormatting(), true);

        spyTransReady.clear();
        router.transcribe(QByteArray("fakeAudio2"));
        QCOMPARE(geminiClient.m_transcribeCallCount, 1);
        QCOMPARE(groqClient.m_transcribeCallCount, 1);
        QCOMPARE(spyTransReady.count(), 1);
        QCOMPARE(spyTransReady.at(0).at(0).toString(), u"Gemini result"_s);
    }

    void testCancelAndSignalDelegation() {
        CloudProviderModel model;
        model.setActiveProviderId(u"gemini"_s);

        CloudSttRouter router;
        router.setCloudProviderModel(&model);

        FakeMockCloudSttClient geminiClient(true);
        geminiClient.setMockApiKeySet(true);
        router.registerProvider(u"gemini"_s, &geminiClient);

        QSignalSpy spyBusy(&router, &AbstractSttClient::busyChanged);
        QSignalSpy spyReady(&router, &AbstractSttClient::readyChanged);

        geminiClient.setMockBusy(true);
        QCOMPARE(spyBusy.count(), 1);
        QCOMPARE(router.isBusy(), true);

        geminiClient.setMockApiKeySet(false);
        QCOMPARE(spyReady.count(), 1);
        QCOMPARE(router.isReady(), false);

        router.cancel();
        QCOMPARE(geminiClient.m_cancelCallCount, 1);
    }

    void testPolymorphicStatusPropertiesAndSignals() {
        CloudProviderModel model;
        model.setActiveProviderId(u"groq"_s);

        CloudSttRouter router;
        router.setCloudProviderModel(&model);

        FakeMockCloudSttClient client(false);
        router.registerProvider(u"groq"_s, &client);

        QSignalSpy spyApiKey(&router, &CloudSttRouter::apiKeySetChanged);
        QSignalSpy spyKeyInvalid(&router, &CloudSttRouter::isApiKeyInvalidChanged);
        QSignalSpy spyRateLimited(&router, &CloudSttRouter::isRateLimitedChanged);
        QSignalSpy spyRetrySeconds(&router, &CloudSttRouter::retrySecondsRemainingChanged);
        QSignalSpy spyLastError(&router, &CloudSttRouter::lastErrorChanged);
        QSignalSpy spyErrorCategory(&router, &CloudSttRouter::errorCategoryChanged);

        QCOMPARE(router.isApiKeySet(), false);
        QCOMPARE(router.isApiKeyInvalid(), false);
        QCOMPARE(router.isRateLimited(), false);
        QCOMPARE(router.retrySecondsRemaining(), 0);
        QCOMPARE(router.lastError(), QString());

        client.setMockApiKeySet(true);
        QCOMPARE(spyApiKey.count(), 1);
        QCOMPARE(router.isApiKeySet(), true);
        QCOMPARE(router.isApiKeyInvalid(), false);

        client.triggerError(u"Unauthorized API key"_s, AbstractSttClient::ErrorCategory::InvalidApiKey);
        QCOMPARE(spyKeyInvalid.count(), 1);
        QCOMPARE(spyLastError.count(), 1);
        QCOMPARE(spyErrorCategory.count(), 1);
        QCOMPARE(router.isApiKeyInvalid(), true);
        QCOMPARE(router.lastError(), u"Unauthorized API key"_s);
        QCOMPARE(router.errorCategory(), AbstractSttClient::ErrorCategory::InvalidApiKey);

        client.triggerError(u"Rate limit reached"_s, AbstractSttClient::ErrorCategory::RateLimited);
        client.setMockRetrySeconds(30);
        QCOMPARE(spyRateLimited.count(), 1);
        QCOMPARE(spyRetrySeconds.count(), 1);
        QCOMPARE(router.isRateLimited(), true);
        QCOMPARE(router.retrySecondsRemaining(), 30);
    }

    void testProviderSwitchUpdatesStatusPropertiesAndRewiresSignals() {
        CloudProviderModel model;
        model.setActiveProviderId(u"groq"_s);

        CloudSttRouter router;
        router.setCloudProviderModel(&model);

        FakeMockCloudSttClient groqClient(false);
        groqClient.setMockApiKeySet(true);

        FakeMockCloudSttClient geminiClient(true);
        geminiClient.setMockApiKeySet(false);
        geminiClient.triggerError(u"Gemini Rate Limited"_s, AbstractSttClient::ErrorCategory::RateLimited);
        geminiClient.setMockRetrySeconds(15);

        router.registerProvider(u"groq"_s, &groqClient);
        router.registerProvider(u"gemini"_s, &geminiClient);

        QCOMPARE(router.isApiKeySet(), true);
        QCOMPARE(router.isRateLimited(), false);
        QCOMPARE(router.retrySecondsRemaining(), 0);

        QSignalSpy spyApiKey(&router, &CloudSttRouter::apiKeySetChanged);
        QSignalSpy spyRateLimited(&router, &CloudSttRouter::isRateLimitedChanged);
        QSignalSpy spyRetrySeconds(&router, &CloudSttRouter::retrySecondsRemainingChanged);

        model.setActiveProviderId(u"gemini"_s);
        QCOMPARE(spyApiKey.count(), 1);
        QCOMPARE(spyRateLimited.count(), 1);
        QCOMPARE(spyRetrySeconds.count(), 1);

        QCOMPARE(router.isApiKeySet(), false);
        QCOMPARE(router.isRateLimited(), true);
        QCOMPARE(router.retrySecondsRemaining(), 15);
        QCOMPARE(router.lastError(), u"Gemini Rate Limited"_s);

        spyApiKey.clear();
        groqClient.setMockApiKeySet(false);
        QCOMPARE(spyApiKey.count(), 0);
    }
};

QTEST_GUILESS_MAIN(TestCloudSttRouter)
#include "test_cloud_stt_router.moc"
