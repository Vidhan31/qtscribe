#include "ApiKeyStore.h"
#include "GroqSttClient.h"
#include "PresetProvider.h"

#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

class TestGroqSttClient : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName(u"QTranscribeTest"_s);
        QCoreApplication::setApplicationName(u"QTranscribeTest"_s);
    }

    void cleanup() {
        QSettings settings;
        settings.remove(u"Groq/Model"_s);
        settings.remove(u"Groq/Language"_s);
        settings.remove(u"Groq/CustomPrompt"_s);
        settings.remove(u"Cloud/Providers/Groq/ApiKey"_s);
        settings.remove(u"GroqTest/ApiKey"_s);
        settings.sync();
    }

    void testDefaultProperties() {
        GroqSttClient client;
        QCOMPARE(client.selectedModel(), u"whisper-large-v3-turbo"_s);
        QCOMPARE(client.language(), QString());
        QCOMPARE(client.customPrompt(), QString());
        QCOMPARE(client.isReady(), false);
        QCOMPARE(client.isBusy(), false);
        QCOMPARE(client.handlesSmartFormatting(), false);
    }

    void testPropertyMutationsAndPersistence() {
        GroqSttClient client;

        QSignalSpy spyModel(&client, &GroqSttClient::selectedModelChanged);
        QSignalSpy spyLang(&client, &GroqSttClient::languageChanged);
        QSignalSpy spyPrompt(&client, &GroqSttClient::customPromptChanged);

        client.setSelectedModel(u"whisper-large-v3"_s);
        client.setLanguage(u"en"_s);
        client.setCustomPrompt(u"Transcribe technical terms verbatim."_s);

        QCOMPARE(client.selectedModel(), u"whisper-large-v3"_s);
        QCOMPARE(client.language(), u"en"_s);
        QCOMPARE(client.customPrompt(), u"Transcribe technical terms verbatim."_s);

        QCOMPARE(spyModel.count(), 1);
        QCOMPARE(spyLang.count(), 1);
        QCOMPARE(spyPrompt.count(), 1);

        QSettings settings;
        QCOMPARE(settings.value(u"Groq/Model"_s).toString(), u"whisper-large-v3"_s);
        QCOMPARE(settings.value(u"Groq/Language"_s).toString(), u"en"_s);
        QCOMPARE(settings.value(u"Groq/CustomPrompt"_s).toString(), u"Transcribe technical terms verbatim."_s);
    }

    void testApiKeyDelegationAndActivateLazyLoad() {
        QSettings settings;
        settings.setValue(u"GroqTest/ApiKey"_s, u"gsk_stt_integration_key"_s);
        settings.sync();

        GroqSttClient client;
        auto* store = new ApiKeyStore(&client);
        store->setStorageKeys(u"QTranscribeTestService"_s, u"test_groq_api_key"_s, u"GroqTest/ApiKey"_s);
        client.setKeyStore(store);

        QCOMPARE(client.isApiKeySet(), false);
        QCOMPARE(client.isReady(), false);

        client.activate();

        QTRY_VERIFY(client.isReady());
        QTRY_COMPARE(client.apiKey(), u"gsk_stt_integration_key"_s);
        QCOMPARE(client.isApiKeySet(), true);
    }

    void testSetAndClearApiKey() {
        GroqSttClient client;
        auto* store = new ApiKeyStore(&client);
        store->setStorageKeys(u"QTranscribeTestService"_s, u"test_groq_api_key"_s, u"GroqTest/ApiKey"_s);
        client.setKeyStore(store);

        QSignalSpy spyKeyChanged(&client, &GroqSttClient::apiKeyChanged);
        QSignalSpy spyKeySetChanged(&client, &AbstractSttClient::apiKeySetChanged);

        client.setApiKey(u"  gsk_new_active_key_12345  "_s);

        QCOMPARE(client.apiKey(), u"gsk_new_active_key_12345"_s);
        QCOMPARE(client.isApiKeySet(), true);
        QCOMPARE(client.isReady(), true);
        QCOMPARE(spyKeyChanged.count(), 1);
        QCOMPARE(spyKeySetChanged.count(), 1);

        client.setApiKey(QString());

        QCOMPARE(client.apiKey(), QString());
        QCOMPARE(client.isApiKeySet(), false);
        QCOMPARE(client.isReady(), false);

        QSettings settings;
        QVERIFY(settings.value(u"GroqTest/ApiKey"_s).toString().isEmpty());
    }

    void testInjectCustomKeyStore() {
        GroqSttClient client;
        auto* customStore = new ApiKeyStore(&client);
        customStore->setStorageKeys(u"CustomSvc"_s, u"custom_key"_s, u"Cloud/Providers/Groq/ApiKey"_s);

        client.setKeyStore(customStore);
        QCOMPARE(client.keyStore(), customStore);

        QSignalSpy spyKeyChanged(&client, &GroqSttClient::apiKeyChanged);
        customStore->setApiKey(u"gsk_injected_store_key"_s);

        QCOMPARE(client.apiKey(), u"gsk_injected_store_key"_s);
        QCOMPARE(client.isApiKeySet(), true);
        QCOMPARE(spyKeyChanged.count(), 1);
    }

    void testPresetProviderPresets() {
        const QString grammar = PresetProvider::systemPromptForPreset(u"grammar"_s);
        QVERIFY(!grammar.isEmpty());
        QVERIFY(grammar.contains(u"speech-to-text post-processor"_s));

        const QString bullets = PresetProvider::systemPromptForPreset(u"bullets"_s);
        QVERIFY(!bullets.isEmpty());
        QVERIFY(bullets.contains(u"bullet points"_s));

        const QString professional = PresetProvider::systemPromptForPreset(u"professional"_s);
        QVERIFY(!professional.isEmpty());
        QVERIFY(professional.contains(u"executive communication"_s));

        const QString customPrompt = u"Format text as JSON object only."_s;
        const QString resolvedCustom = PresetProvider::systemPromptForPreset(u"custom"_s, customPrompt);
        QCOMPARE(resolvedCustom, customPrompt);

        const QString defaultCustom = PresetProvider::systemPromptForPreset(u"custom"_s, QString());
        QCOMPARE(defaultCustom, PresetProvider::defaultCustomPrompt());

        const QString unknownFallback = PresetProvider::systemPromptForPreset(u"nonexistent_preset"_s);
        QCOMPARE(unknownFallback, PresetProvider::grammarPrompt());

        QCOMPARE(PresetProvider::defaultPreset(), u"grammar"_s);
        const QStringList presets = PresetProvider::availablePresets();
        QVERIFY(presets.contains(u"grammar"_s));
        QVERIFY(presets.contains(u"bullets"_s));
        QVERIFY(presets.contains(u"professional"_s));
        QVERIFY(presets.contains(u"custom"_s));
    }

    void testCancelResetsState() {
        GroqSttClient client;
        QSignalSpy spyBusy(&client, &GroqSttClient::busyChanged);

        client.cancel();
        QCOMPARE(client.isBusy(), false);
        QCOMPARE(client.isCancelled(), true);
    }
};

QTEST_GUILESS_MAIN(TestGroqSttClient)
#include "test_groq_stt_client.moc"
