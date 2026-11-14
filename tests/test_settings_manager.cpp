#include "SettingsManager.h"
#include "SettingsService.h"
#include "common/SettingsKeys.h"

#include <QCoreApplication>
#include <QDBusVariant>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

class TestSettingsManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName(u"QtScribeTest"_s);
        QCoreApplication::setApplicationName(u"QtScribeTest_SettingsManager"_s);
    }

    void cleanup() {
        QSettings settings;
        settings.clear();
        settings.sync();
    }

    void testDefaults() {
        SettingsManager manager;
        const auto& speech = manager.speechConfig();
        QCOMPARE(speech.backend, SettingsManager::TranscriptionBackend::Cloud);
        QCOMPARE(speech.recordingMode, SettingsManager::RecordingMode::Toggle);
        QCOMPARE(speech.autoSwitchOffline, true);

        const auto& groq = manager.groqSttConfig();
        QCOMPARE(groq.model, u"whisper-large-v3-turbo"_s);
        QCOMPARE(groq.language, QString());
        QCOMPARE(groq.customPrompt, QString());

        const auto& gemini = manager.geminiSttConfig();
        QCOMPARE(gemini.model, u"gemini-3.5-transcribe"_s);
        QCOMPARE(gemini.mode, u"smart"_s);
        QCOMPARE(gemini.customVocabulary, QString());

        const auto& metaMusa = manager.metaMusaSttConfig();
        QCOMPARE(metaMusa.model, u"muse-voice-transcribe-1.0"_s);
        QCOMPARE(metaMusa.keywords, QString());

        const auto& llm = manager.llmConfig();
        QCOMPARE(llm.enabled, false);
        QCOMPARE(llm.model, u"openai/gpt-oss-20b"_s);
        QCOMPARE(llm.preset, u"grammar"_s);
        QCOMPARE(llm.temperature, 0.1);

        const auto& injector = manager.textInjectorConfig();
        QCOMPARE(injector.preventClipboardHistory, true);
        QCOMPARE(injector.injectionDelayMs, 200);

        const auto& audio = manager.audioFeedbackConfig();
        QCOMPARE(audio.soundEnabled, true);

        const auto& notify = manager.notificationConfig();
        QCOMPARE(notify.notificationsEnabled, true);

        const auto& cloud = manager.cloudRouterConfig();
        QCOMPARE(cloud.activeProvider, u"groq"_s);
    }

    void testValidation() {
        SettingsManager manager;

        manager.setGroqModel(u"invalid-groq-model"_s);
        QCOMPARE(manager.groqSttConfig().model, u"whisper-large-v3-turbo"_s);

        manager.setGroqModel(u"whisper-large-v3"_s);
        QCOMPARE(manager.groqSttConfig().model, u"whisper-large-v3"_s);

        manager.setGeminiModel(u"   "_s);
        QCOMPARE(manager.geminiSttConfig().model, u"gemini-3.5-transcribe"_s);

        manager.setGeminiMode(u"not-a-mode"_s);
        QCOMPARE(manager.geminiSttConfig().mode, u"smart"_s);

        manager.setGeminiMode(u"verbatim"_s);
        QCOMPARE(manager.geminiSttConfig().mode, u"verbatim"_s);

        manager.setMetaMusaModel(u"   "_s);
        QCOMPARE(manager.metaMusaSttConfig().model, u"muse-voice-transcribe-1.0"_s);
        manager.setMetaMusaKeywords(u"test, keywords"_s);
        QCOMPARE(manager.metaMusaSttConfig().keywords, u"test, keywords"_s);

        manager.setLlmModel(u"llama-3.1-8b-instant"_s);
        QCOMPARE(manager.llmConfig().model, u"openai/gpt-oss-20b"_s);

        manager.setLlmPreset(QString());
        QCOMPARE(manager.llmConfig().preset, u"grammar"_s);

        manager.setLlmTemperature(5.0);
        QCOMPARE(manager.llmConfig().temperature, 2.0);

        manager.setLlmTemperature(-1.0);
        QCOMPARE(manager.llmConfig().temperature, 0.0);

        manager.setInjectionDelay(99999);
        QCOMPARE(manager.textInjectorConfig().injectionDelayMs, 10000);

        manager.setInjectionDelay(-50);
        QCOMPARE(manager.textInjectorConfig().injectionDelayMs, 0);
    }

    void testChangeSignals() {
        SettingsManager manager;

        QSignalSpy spySpeech(&manager, &SettingsManager::speechConfigChanged);
        QSignalSpy spyGroq(&manager, &SettingsManager::groqSttConfigChanged);
        QSignalSpy spyInjector(&manager, &SettingsManager::textInjectorConfigChanged);
        QSignalSpy spySetting(&manager, &SettingsManager::settingChanged);

        manager.setBackend(SettingsManager::TranscriptionBackend::WhisperCpp);
        QCOMPARE(spySpeech.count(), 1);
        QCOMPARE(spySetting.count(), 1);

        manager.setGroqLanguage(u"fr"_s);
        QCOMPARE(spyGroq.count(), 1);
        QCOMPARE(spySetting.count(), 2);

        manager.setInjectionDelay(350);
        QCOMPARE(spyInjector.count(), 1);
        QCOMPARE(spySetting.count(), 3);
    }

    void testGenericSetSettingAndPersistence() {
        SettingsManager manager;

        manager.setSetting(SettingsKeys::kSpeechBackend.toString(), u"WhisperCpp"_s);
        manager.setSetting(SettingsKeys::kSpeechRecordingMode.toString(), u"PushToTalk"_s);
        manager.setSetting(SettingsKeys::kSpeechAutoSwitchOffline.toString(), false);
        manager.setSetting(SettingsKeys::kCloudActiveProvider.toString(), u"gemini"_s);
        manager.setSetting(SettingsKeys::kAudioSoundEnabled.toString(), false);
        manager.setSetting(SettingsKeys::kNotificationsEnabled.toString(), false);

        QCOMPARE(manager.speechConfig().backend, SettingsManager::TranscriptionBackend::WhisperCpp);
        QCOMPARE(manager.speechConfig().recordingMode, SettingsManager::RecordingMode::PushToTalk);
        QCOMPARE(manager.speechConfig().autoSwitchOffline, false);
        QCOMPARE(manager.cloudRouterConfig().activeProvider, u"gemini"_s);
        QCOMPARE(manager.audioFeedbackConfig().soundEnabled, false);
        QCOMPARE(manager.notificationConfig().notificationsEnabled, false);

        // Verify stored in QSettings
        QSettings settings;
        QCOMPARE(settings.value(SettingsKeys::kSpeechBackend.toString()).toString(), u"WhisperCpp"_s);
        QCOMPARE(settings.value(SettingsKeys::kSpeechRecordingMode.toString()).toString(), u"PushToTalk"_s);
        QCOMPARE(settings.value(SettingsKeys::kSpeechAutoSwitchOffline.toString()).toBool(), false);
        QCOMPARE(settings.value(SettingsKeys::kCloudActiveProvider.toString()).toString(), u"gemini"_s);
        QCOMPARE(settings.value(SettingsKeys::kAudioSoundEnabled.toString()).toBool(), false);
        QCOMPARE(settings.value(SettingsKeys::kNotificationsEnabled.toString()).toBool(), false);

        // Create a new manager instance and verify load() restores the settings
        SettingsManager secondManager;
        QCOMPARE(secondManager.speechConfig().backend, SettingsManager::TranscriptionBackend::WhisperCpp);
        QCOMPARE(secondManager.speechConfig().recordingMode, SettingsManager::RecordingMode::PushToTalk);
        QCOMPARE(secondManager.speechConfig().autoSwitchOffline, false);
        QCOMPARE(secondManager.cloudRouterConfig().activeProvider, u"gemini"_s);
    }

    void testSettingsServiceIntegration() {
        SettingsManager manager;
        SettingsService service(&manager);

        QSignalSpy spyService(&service, &SettingsService::settingChanged);

        service.setStringSetting(SettingsKeys::kGroqModel.toString(), u"whisper-large-v3"_s);
        QCOMPARE(manager.groqSttConfig().model, u"whisper-large-v3"_s);
        QCOMPARE(spyService.count(), 1);

        service.setBoolSetting(SettingsKeys::kGroqLlmEnabled.toString(), true);
        QCOMPARE(manager.llmConfig().enabled, true);

        QDBusVariant val = service.getSetting(SettingsKeys::kGroqModel.toString());
        QCOMPARE(val.variant().toString(), u"whisper-large-v3"_s);
    }
};

QTEST_MAIN(TestSettingsManager)
#include "test_settings_manager.moc"
