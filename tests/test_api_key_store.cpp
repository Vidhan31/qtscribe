#include "ApiKeyStore.h"

#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

class TestApiKeyStore : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName(u"QTranscribeTestStore"_s);
        QCoreApplication::setApplicationName(u"QTranscribeTestStore"_s);
    }

    void cleanup() {
        QSettings settings;
        settings.clear();
        settings.sync();
    }

    void testDefaultState() {
        ApiKeyStore store;
        QVERIFY(store.apiKey().isEmpty());
        QVERIFY(!store.apiKeySet());
        QVERIFY(!store.isLoading());
        QCOMPARE(store.keychainService(), u"QtScribe"_s);
        QCOMPARE(store.keychainKey(), u"groq_api_key"_s);
        QCOMPARE(store.settingsKey(), u"Cloud/Providers/Groq/ApiKey"_s);
    }

    void testSetAndGetApiKeyWithTrimming() {
        ApiKeyStore store;
        store.setStorageKeys(u"TestService"_s, u"test_key"_s, u"Test/ApiKey"_s);

        QSignalSpy keySpy(&store, &ApiKeyStore::apiKeyChanged);
        QSignalSpy setSpy(&store, &ApiKeyStore::apiKeySetChanged);

        store.setApiKey(u"   gsk_test_token_123   "_s);

        QCOMPARE(store.apiKey(), u"gsk_test_token_123"_s);
        QVERIFY(store.apiKeySet());
        QCOMPARE(keySpy.count(), 1);
        QCOMPARE(setSpy.count(), 1);

        // Setting same trimmed value is a no-op
        store.setApiKey(u"gsk_test_token_123"_s);
        QCOMPARE(keySpy.count(), 1);
        QCOMPARE(setSpy.count(), 1);

        // Clearing the key
        store.setApiKey(u""_s);
        QVERIFY(store.apiKey().isEmpty());
        QVERIFY(!store.apiKeySet());
        QCOMPARE(keySpy.count(), 2);
        QCOMPARE(setSpy.count(), 2);
    }

    void testSettingsFallbackAndLegacyMigration() {
        // Populate legacy key in QSettings
        {
            QSettings settings;
            settings.setValue(u"Groq/ApiKey"_s, u"gsk_legacy_migrated_key"_s);
            settings.sync();
        }

        ApiKeyStore store;
        store.setStorageKeys(u"QtScribeIsolatedTestSvc"_s, u"unused_nonexistent_key_9999"_s,
                             u"Cloud/Providers/Groq/ApiKey"_s);

        // Ensure key load happens
        store.ensureApiKeyLoaded();

        // Give event loop a turn for fallback execution
        QTRY_VERIFY(store.apiKeySet());
        QCOMPARE(store.apiKey(), u"gsk_legacy_migrated_key"_s);

        // Verify that the legacy key was migrated to the new settings key
        QSettings settings;
        QCOMPARE(settings.value(u"Cloud/Providers/Groq/ApiKey"_s).toString(), u"gsk_legacy_migrated_key"_s);
        QVERIFY(settings.value(u"Groq/ApiKey"_s).isNull());
    }

    void testCustomStorageKeys() {
        ApiKeyStore store;
        store.setStorageKeys(u"CustomService"_s, u"custom_id"_s, u"Custom/Settings/Key"_s);

        QCOMPARE(store.keychainService(), u"CustomService"_s);
        QCOMPARE(store.keychainKey(), u"custom_id"_s);
        QCOMPARE(store.settingsKey(), u"Custom/Settings/Key"_s);

        {
            QSettings settings;
            settings.setValue(u"Custom/Settings/Key"_s, u"custom_secret_value"_s);
            settings.sync();
        }

        store.loadApiKey();
        QTRY_COMPARE(store.apiKey(), u"custom_secret_value"_s);
        QVERIFY(store.apiKeySet());
    }
};

QTEST_GUILESS_MAIN(TestApiKeyStore)
#include "test_api_key_store.moc"
