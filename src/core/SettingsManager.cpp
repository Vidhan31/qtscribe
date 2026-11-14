#include "SettingsManager.h"

#include "LoggingCategories.h"

#include <QDebug>
#include <QSettings>

#include <algorithm>
#include <cmath>

using namespace Qt::StringLiterals;

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent) {
    if (!s_instance) {
        s_instance = this;
    }
    load();
}

SettingsManager::~SettingsManager() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

SettingsManager* SettingsManager::instance() {
    if (!s_instance) {
        s_instance = new SettingsManager();
    }
    return s_instance;
}

void SettingsManager::setInstance(SettingsManager* inst) {
    s_instance = inst;
}

QString SettingsManager::validateGroqModel(const QString& model) {
    if (model == u"whisper-large-v3-turbo"_s || model == u"whisper-large-v3"_s) {
        return model;
    }
    return u"whisper-large-v3-turbo"_s;
}

QString SettingsManager::validateGeminiModel(const QString& model) {
    const QString trimmed = model.trimmed();
    return trimmed.isEmpty() ? u"gemini-3.5-transcribe"_s : trimmed;
}

QString SettingsManager::validateGeminiMode(const QString& mode) {
    if (mode == u"smart"_s || mode == u"verbatim"_s) {
        return mode;
    }
    return u"smart"_s;
}

QString SettingsManager::validateMetaMusaModel(const QString& model) {
    const QString trimmed = model.trimmed();
    return trimmed.isEmpty() ? u"muse-voice-transcribe-1.0"_s : trimmed;
}

QString SettingsManager::validateLlmModel(const QString& model) {
    if (model.isEmpty() || model == u"llama-3.1-8b-instant"_s) {
        return u"openai/gpt-oss-20b"_s;
    }
    return model;
}

QString SettingsManager::validateLlmPreset(const QString& preset) {
    return preset.isEmpty() ? u"grammar"_s : preset;
}

double SettingsManager::validateLlmTemperature(double temp) {
    return std::clamp(temp, 0.0, 2.0);
}

int SettingsManager::validateInjectionDelay(int delayMs) {
    return std::clamp(delayMs, 0, 10000);
}

void SettingsManager::load() {
    const QSettings settings;

    SpeechConfig newSpeech;
    const QString backendStr = settings.value(SettingsKeys::kSpeechBackend.toString(), u"Cloud"_s).toString();
    newSpeech.backend =
        (backendStr == u"WhisperCpp"_s) ? TranscriptionBackend::WhisperCpp : TranscriptionBackend::Cloud;
    const QString modeStr = settings.value(SettingsKeys::kSpeechRecordingMode.toString(), u"Toggle"_s).toString();
    newSpeech.recordingMode = (modeStr == u"PushToTalk"_s) ? RecordingMode::PushToTalk : RecordingMode::Toggle;
    newSpeech.autoSwitchOffline = settings.value(SettingsKeys::kSpeechAutoSwitchOffline.toString(), true).toBool();

    GroqSttConfig newGroq;
    newGroq.model =
        validateGroqModel(settings.value(SettingsKeys::kGroqModel.toString(), u"whisper-large-v3-turbo"_s).toString());
    newGroq.language = settings.value(SettingsKeys::kGroqLanguage.toString(), QString()).toString();
    newGroq.customPrompt = settings.value(SettingsKeys::kGroqCustomPrompt.toString(), QString()).toString();

    GeminiSttConfig newGemini;
    newGemini.model = validateGeminiModel(
        settings.value(SettingsKeys::kGeminiModel.toString(), u"gemini-3.5-transcribe"_s).toString());
    newGemini.mode = validateGeminiMode(settings.value(SettingsKeys::kGeminiMode.toString(), u"smart"_s).toString());
    newGemini.customVocabulary = settings.value(SettingsKeys::kGeminiCustomVocabulary.toString(), QString()).toString();

    MetaMusaSttConfig newMetaMusa;
    newMetaMusa.model = validateMetaMusaModel(
        settings.value(SettingsKeys::kMetaMusaModel.toString(), u"muse-voice-transcribe-1.0"_s).toString());
    newMetaMusa.keywords = settings.value(SettingsKeys::kMetaMusaKeywords.toString(), QString()).toString();

    LlmConfig newLlm;
    newLlm.enabled = settings.value(SettingsKeys::kGroqLlmEnabled.toString(), false).toBool();
    newLlm.model =
        validateLlmModel(settings.value(SettingsKeys::kGroqLlmModel.toString(), u"openai/gpt-oss-20b"_s).toString());
    newLlm.preset = validateLlmPreset(settings.value(SettingsKeys::kGroqLlmPreset.toString(), u"grammar"_s).toString());
    newLlm.customPrompt = settings.value(SettingsKeys::kGroqLlmCustomPrompt.toString(), QString()).toString();
    newLlm.temperature =
        validateLlmTemperature(settings.value(SettingsKeys::kGroqLlmTemperature.toString(), 0.1).toDouble());

    TextInjectorConfig newInjector;
    newInjector.preventClipboardHistory =
        settings.value(SettingsKeys::kClipboardPreventHistory.toString(), true).toBool();
    newInjector.injectionDelayMs =
        validateInjectionDelay(settings.value(SettingsKeys::kTypingPreInjectionDelayMs.toString(), 200).toInt());

    AudioFeedbackConfig newAudio;
    newAudio.soundEnabled = settings.value(SettingsKeys::kAudioSoundEnabled.toString(), true).toBool();

    NotificationConfig newNotify;
    newNotify.notificationsEnabled = settings.value(SettingsKeys::kNotificationsEnabled.toString(), true).toBool();

    CloudRouterConfig newCloud;
    newCloud.activeProvider = settings.value(SettingsKeys::kCloudActiveProvider.toString(), u"groq"_s).toString();

    WhisperConfig newWhisper;
    newWhisper.selectedModel = settings.value(SettingsKeys::kWhisperSelectedModel.toString(), QString()).toString();

    const bool speechChanged = (m_speechConfig != newSpeech);
    const bool groqChanged = (m_groqSttConfig != newGroq);
    const bool geminiChanged = (m_geminiSttConfig != newGemini);
    const bool metaMusaChanged = (m_metaMusaSttConfig != newMetaMusa);
    const bool llmChanged = (m_llmConfig != newLlm);
    const bool injectorChanged = (m_textInjectorConfig != newInjector);
    const bool audioChanged = (m_audioFeedbackConfig != newAudio);
    const bool notifyChanged = (m_notificationConfig != newNotify);
    const bool cloudChanged = (m_cloudRouterConfig != newCloud);
    const bool whisperChanged = (m_whisperConfig != newWhisper);

    m_speechConfig = newSpeech;
    m_groqSttConfig = newGroq;
    m_geminiSttConfig = newGemini;
    m_metaMusaSttConfig = newMetaMusa;
    m_llmConfig = newLlm;
    m_textInjectorConfig = newInjector;
    m_audioFeedbackConfig = newAudio;
    m_notificationConfig = newNotify;
    m_cloudRouterConfig = newCloud;
    m_whisperConfig = newWhisper;

    if (speechChanged) {
        emit speechConfigChanged(m_speechConfig);
    }
    if (groqChanged) {
        emit groqSttConfigChanged(m_groqSttConfig);
    }
    if (geminiChanged) {
        emit geminiSttConfigChanged(m_geminiSttConfig);
    }
    if (metaMusaChanged) {
        emit metaMusaSttConfigChanged(m_metaMusaSttConfig);
    }
    if (llmChanged) {
        emit llmConfigChanged(m_llmConfig);
    }
    if (injectorChanged) {
        emit textInjectorConfigChanged(m_textInjectorConfig);
    }
    if (audioChanged) {
        emit audioFeedbackConfigChanged(m_audioFeedbackConfig);
    }
    if (notifyChanged) {
        emit notificationConfigChanged(m_notificationConfig);
    }
    if (cloudChanged) {
        emit cloudRouterConfigChanged(m_cloudRouterConfig);
    }
    if (whisperChanged) {
        emit whisperConfigChanged(m_whisperConfig);
    }
}

void SettingsManager::save() const {
    QSettings settings;
    settings.setValue(SettingsKeys::kSpeechBackend.toString(),
                      m_speechConfig.backend == TranscriptionBackend::WhisperCpp ? u"WhisperCpp"_s : u"Cloud"_s);
    settings.setValue(SettingsKeys::kSpeechTranscriptionBackend.toString(), static_cast<int>(m_speechConfig.backend));
    settings.setValue(SettingsKeys::kSpeechRecordingMode.toString(),
                      m_speechConfig.recordingMode == RecordingMode::PushToTalk ? u"PushToTalk"_s : u"Toggle"_s);
    settings.setValue(SettingsKeys::kSpeechAutoSwitchOffline.toString(), m_speechConfig.autoSwitchOffline);

    settings.setValue(SettingsKeys::kGroqModel.toString(), m_groqSttConfig.model);
    settings.setValue(SettingsKeys::kGroqLanguage.toString(), m_groqSttConfig.language);
    settings.setValue(SettingsKeys::kGroqCustomPrompt.toString(), m_groqSttConfig.customPrompt);

    settings.setValue(SettingsKeys::kGeminiModel.toString(), m_geminiSttConfig.model);
    settings.setValue(SettingsKeys::kGeminiMode.toString(), m_geminiSttConfig.mode);
    settings.setValue(SettingsKeys::kGeminiCustomVocabulary.toString(), m_geminiSttConfig.customVocabulary);

    settings.setValue(SettingsKeys::kMetaMusaModel.toString(), m_metaMusaSttConfig.model);
    settings.setValue(SettingsKeys::kMetaMusaKeywords.toString(), m_metaMusaSttConfig.keywords);

    settings.setValue(SettingsKeys::kGroqLlmEnabled.toString(), m_llmConfig.enabled);
    settings.setValue(SettingsKeys::kGroqLlmModel.toString(), m_llmConfig.model);
    settings.setValue(SettingsKeys::kGroqLlmPreset.toString(), m_llmConfig.preset);
    settings.setValue(SettingsKeys::kGroqLlmCustomPrompt.toString(), m_llmConfig.customPrompt);
    settings.setValue(SettingsKeys::kGroqLlmTemperature.toString(), m_llmConfig.temperature);

    settings.setValue(SettingsKeys::kClipboardPreventHistory.toString(), m_textInjectorConfig.preventClipboardHistory);
    settings.setValue(SettingsKeys::kTypingPreInjectionDelayMs.toString(), m_textInjectorConfig.injectionDelayMs);

    settings.setValue(SettingsKeys::kAudioSoundEnabled.toString(), m_audioFeedbackConfig.soundEnabled);
    settings.setValue(SettingsKeys::kNotificationsEnabled.toString(), m_notificationConfig.notificationsEnabled);
    settings.setValue(SettingsKeys::kCloudActiveProvider.toString(), m_cloudRouterConfig.activeProvider);
    settings.setValue(SettingsKeys::kWhisperSelectedModel.toString(), m_whisperConfig.selectedModel);
    settings.sync();
}

void SettingsManager::setSpeechConfig(const SpeechConfig& config) {
    if (m_speechConfig == config) {
        return;
    }
    m_speechConfig = config;
    QSettings settings;
    settings.setValue(SettingsKeys::kSpeechBackend.toString(),
                      m_speechConfig.backend == TranscriptionBackend::WhisperCpp ? u"WhisperCpp"_s : u"Cloud"_s);
    settings.setValue(SettingsKeys::kSpeechTranscriptionBackend.toString(), static_cast<int>(m_speechConfig.backend));
    settings.setValue(SettingsKeys::kSpeechRecordingMode.toString(),
                      m_speechConfig.recordingMode == RecordingMode::PushToTalk ? u"PushToTalk"_s : u"Toggle"_s);
    settings.setValue(SettingsKeys::kSpeechAutoSwitchOffline.toString(), m_speechConfig.autoSwitchOffline);
    settings.sync();
    emit speechConfigChanged(m_speechConfig);
}

void SettingsManager::setBackend(TranscriptionBackend backend) {
    if (m_speechConfig.backend != backend) {
        m_speechConfig.backend = backend;
        QSettings settings;
        settings.setValue(SettingsKeys::kSpeechBackend.toString(),
                          backend == TranscriptionBackend::WhisperCpp ? u"WhisperCpp"_s : u"Cloud"_s);
        settings.setValue(SettingsKeys::kSpeechTranscriptionBackend.toString(), static_cast<int>(backend));
        settings.sync();
        emit speechConfigChanged(m_speechConfig);
        emit settingChanged(SettingsKeys::kSpeechBackend.toString(),
                            backend == TranscriptionBackend::WhisperCpp ? u"WhisperCpp"_s : u"Cloud"_s);
    }
}

void SettingsManager::setRecordingMode(RecordingMode mode) {
    if (m_speechConfig.recordingMode != mode) {
        m_speechConfig.recordingMode = mode;
        QSettings settings;
        settings.setValue(SettingsKeys::kSpeechRecordingMode.toString(),
                          mode == RecordingMode::PushToTalk ? u"PushToTalk"_s : u"Toggle"_s);
        settings.sync();
        emit speechConfigChanged(m_speechConfig);
        emit settingChanged(SettingsKeys::kSpeechRecordingMode.toString(),
                            mode == RecordingMode::PushToTalk ? u"PushToTalk"_s : u"Toggle"_s);
    }
}

void SettingsManager::setAutoSwitchOffline(bool enable) {
    if (m_speechConfig.autoSwitchOffline != enable) {
        m_speechConfig.autoSwitchOffline = enable;
        QSettings settings;
        settings.setValue(SettingsKeys::kSpeechAutoSwitchOffline.toString(), enable);
        settings.sync();
        emit speechConfigChanged(m_speechConfig);
        emit settingChanged(SettingsKeys::kSpeechAutoSwitchOffline.toString(), enable);
    }
}

void SettingsManager::setGroqSttConfig(const GroqSttConfig& config) {
    GroqSttConfig validated = config;
    validated.model = validateGroqModel(validated.model);
    if (m_groqSttConfig == validated) {
        return;
    }
    m_groqSttConfig = validated;
    QSettings settings;
    settings.setValue(SettingsKeys::kGroqModel.toString(), m_groqSttConfig.model);
    settings.setValue(SettingsKeys::kGroqLanguage.toString(), m_groqSttConfig.language);
    settings.setValue(SettingsKeys::kGroqCustomPrompt.toString(), m_groqSttConfig.customPrompt);
    settings.sync();
    emit groqSttConfigChanged(m_groqSttConfig);
}

void SettingsManager::setGroqModel(const QString& model) {
    const QString valid = validateGroqModel(model.trimmed());
    if (m_groqSttConfig.model != valid) {
        m_groqSttConfig.model = valid;
        QSettings settings;
        settings.setValue(SettingsKeys::kGroqModel.toString(), valid);
        settings.sync();
        emit groqSttConfigChanged(m_groqSttConfig);
        emit settingChanged(SettingsKeys::kGroqModel.toString(), valid);
    }
}

void SettingsManager::setGroqLanguage(const QString& language) {
    const QString trimmed = language.trimmed();
    if (m_groqSttConfig.language != trimmed) {
        m_groqSttConfig.language = trimmed;
        QSettings settings;
        settings.setValue(SettingsKeys::kGroqLanguage.toString(), trimmed);
        settings.sync();
        emit groqSttConfigChanged(m_groqSttConfig);
        emit settingChanged(SettingsKeys::kGroqLanguage.toString(), trimmed);
    }
}

void SettingsManager::setGroqCustomPrompt(const QString& prompt) {
    if (m_groqSttConfig.customPrompt != prompt) {
        m_groqSttConfig.customPrompt = prompt;
        QSettings settings;
        settings.setValue(SettingsKeys::kGroqCustomPrompt.toString(), prompt);
        settings.sync();
        emit groqSttConfigChanged(m_groqSttConfig);
        emit settingChanged(SettingsKeys::kGroqCustomPrompt.toString(), prompt);
    }
}

void SettingsManager::setGeminiSttConfig(const GeminiSttConfig& config) {
    GeminiSttConfig validated = config;
    validated.model = validateGeminiModel(validated.model);
    validated.mode = validateGeminiMode(validated.mode);
    if (m_geminiSttConfig == validated) {
        return;
    }
    m_geminiSttConfig = validated;
    QSettings settings;
    settings.setValue(SettingsKeys::kGeminiModel.toString(), m_geminiSttConfig.model);
    settings.setValue(SettingsKeys::kGeminiMode.toString(), m_geminiSttConfig.mode);
    settings.setValue(SettingsKeys::kGeminiCustomVocabulary.toString(), m_geminiSttConfig.customVocabulary);
    settings.sync();
    emit geminiSttConfigChanged(m_geminiSttConfig);
}

void SettingsManager::setMetaMusaSttConfig(const MetaMusaSttConfig& config) {
    MetaMusaSttConfig validated = config;
    validated.model = validateMetaMusaModel(validated.model);
    if (m_metaMusaSttConfig == validated) {
        return;
    }
    m_metaMusaSttConfig = validated;
    QSettings settings;
    settings.setValue(SettingsKeys::kMetaMusaModel.toString(), m_metaMusaSttConfig.model);
    settings.setValue(SettingsKeys::kMetaMusaKeywords.toString(), m_metaMusaSttConfig.keywords);
    settings.sync();
    emit metaMusaSttConfigChanged(m_metaMusaSttConfig);
}

void SettingsManager::setMetaMusaModel(const QString& model) {
    const QString valid = validateMetaMusaModel(model);
    if (m_metaMusaSttConfig.model != valid) {
        m_metaMusaSttConfig.model = valid;
        QSettings settings;
        settings.setValue(SettingsKeys::kMetaMusaModel.toString(), valid);
        settings.sync();
        emit metaMusaSttConfigChanged(m_metaMusaSttConfig);
        emit settingChanged(SettingsKeys::kMetaMusaModel.toString(), valid);
    }
}

void SettingsManager::setMetaMusaKeywords(const QString& keywords) {
    if (m_metaMusaSttConfig.keywords != keywords) {
        m_metaMusaSttConfig.keywords = keywords;
        QSettings settings;
        settings.setValue(SettingsKeys::kMetaMusaKeywords.toString(), keywords);
        settings.sync();
        emit metaMusaSttConfigChanged(m_metaMusaSttConfig);
        emit settingChanged(SettingsKeys::kMetaMusaKeywords.toString(), keywords);
    }
}

void SettingsManager::setGeminiModel(const QString& model) {
    const QString valid = validateGeminiModel(model);
    if (m_geminiSttConfig.model != valid) {
        m_geminiSttConfig.model = valid;
        QSettings settings;
        settings.setValue(SettingsKeys::kGeminiModel.toString(), valid);
        settings.sync();
        emit geminiSttConfigChanged(m_geminiSttConfig);
        emit settingChanged(SettingsKeys::kGeminiModel.toString(), valid);
    }
}

void SettingsManager::setGeminiMode(const QString& mode) {
    const QString valid = validateGeminiMode(mode);
    if (m_geminiSttConfig.mode != valid) {
        m_geminiSttConfig.mode = valid;
        QSettings settings;
        settings.setValue(SettingsKeys::kGeminiMode.toString(), valid);
        settings.sync();
        emit geminiSttConfigChanged(m_geminiSttConfig);
        emit settingChanged(SettingsKeys::kGeminiMode.toString(), valid);
    }
}

void SettingsManager::setGeminiCustomVocabulary(const QString& vocabulary) {
    if (m_geminiSttConfig.customVocabulary != vocabulary) {
        m_geminiSttConfig.customVocabulary = vocabulary;
        QSettings settings;
        settings.setValue(SettingsKeys::kGeminiCustomVocabulary.toString(), vocabulary);
        settings.sync();
        emit geminiSttConfigChanged(m_geminiSttConfig);
        emit settingChanged(SettingsKeys::kGeminiCustomVocabulary.toString(), vocabulary);
    }
}

void SettingsManager::setLlmConfig(const LlmConfig& config) {
    LlmConfig validated = config;
    validated.model = validateLlmModel(validated.model);
    validated.preset = validateLlmPreset(validated.preset);
    validated.temperature = validateLlmTemperature(validated.temperature);
    if (m_llmConfig == validated) {
        return;
    }
    m_llmConfig = validated;
    QSettings settings;
    settings.setValue(SettingsKeys::kGroqLlmEnabled.toString(), m_llmConfig.enabled);
    settings.setValue(SettingsKeys::kGroqLlmModel.toString(), m_llmConfig.model);
    settings.setValue(SettingsKeys::kGroqLlmPreset.toString(), m_llmConfig.preset);
    settings.setValue(SettingsKeys::kGroqLlmCustomPrompt.toString(), m_llmConfig.customPrompt);
    settings.setValue(SettingsKeys::kGroqLlmTemperature.toString(), m_llmConfig.temperature);
    settings.sync();
    emit llmConfigChanged(m_llmConfig);
}

void SettingsManager::setLlmEnabled(bool enabled) {
    if (m_llmConfig.enabled != enabled) {
        m_llmConfig.enabled = enabled;
        QSettings settings;
        settings.setValue(SettingsKeys::kGroqLlmEnabled.toString(), enabled);
        settings.sync();
        emit llmConfigChanged(m_llmConfig);
        emit settingChanged(SettingsKeys::kGroqLlmEnabled.toString(), enabled);
    }
}

void SettingsManager::setLlmModel(const QString& model) {
    const QString valid = validateLlmModel(model);
    if (m_llmConfig.model != valid) {
        m_llmConfig.model = valid;
        QSettings settings;
        settings.setValue(SettingsKeys::kGroqLlmModel.toString(), valid);
        settings.sync();
        emit llmConfigChanged(m_llmConfig);
        emit settingChanged(SettingsKeys::kGroqLlmModel.toString(), valid);
    }
}

void SettingsManager::setLlmPreset(const QString& preset) {
    const QString valid = validateLlmPreset(preset);
    if (m_llmConfig.preset != valid) {
        m_llmConfig.preset = valid;
        QSettings settings;
        settings.setValue(SettingsKeys::kGroqLlmPreset.toString(), valid);
        settings.sync();
        emit llmConfigChanged(m_llmConfig);
        emit settingChanged(SettingsKeys::kGroqLlmPreset.toString(), valid);
    }
}

void SettingsManager::setLlmCustomPrompt(const QString& prompt) {
    if (m_llmConfig.customPrompt != prompt) {
        m_llmConfig.customPrompt = prompt;
        QSettings settings;
        settings.setValue(SettingsKeys::kGroqLlmCustomPrompt.toString(), prompt);
        settings.sync();
        emit llmConfigChanged(m_llmConfig);
        emit settingChanged(SettingsKeys::kGroqLlmCustomPrompt.toString(), prompt);
    }
}

void SettingsManager::setLlmTemperature(double temp) {
    const double valid = validateLlmTemperature(temp);
    if (std::abs(m_llmConfig.temperature - valid) > 0.001) {
        m_llmConfig.temperature = valid;
        QSettings settings;
        settings.setValue(SettingsKeys::kGroqLlmTemperature.toString(), valid);
        settings.sync();
        emit llmConfigChanged(m_llmConfig);
        emit settingChanged(SettingsKeys::kGroqLlmTemperature.toString(), valid);
    }
}

void SettingsManager::setTextInjectorConfig(const TextInjectorConfig& config) {
    TextInjectorConfig validated = config;
    validated.injectionDelayMs = validateInjectionDelay(validated.injectionDelayMs);
    if (m_textInjectorConfig == validated) {
        return;
    }
    m_textInjectorConfig = validated;
    QSettings settings;
    settings.setValue(SettingsKeys::kClipboardPreventHistory.toString(), m_textInjectorConfig.preventClipboardHistory);
    settings.setValue(SettingsKeys::kTypingPreInjectionDelayMs.toString(), m_textInjectorConfig.injectionDelayMs);
    settings.sync();
    emit textInjectorConfigChanged(m_textInjectorConfig);
}

void SettingsManager::setPreventClipboardHistory(bool prevent) {
    if (m_textInjectorConfig.preventClipboardHistory != prevent) {
        m_textInjectorConfig.preventClipboardHistory = prevent;
        QSettings settings;
        settings.setValue(SettingsKeys::kClipboardPreventHistory.toString(), prevent);
        settings.sync();
        emit textInjectorConfigChanged(m_textInjectorConfig);
        emit settingChanged(SettingsKeys::kClipboardPreventHistory.toString(), prevent);
    }
}

void SettingsManager::setInjectionDelay(int delayMs) {
    const int valid = validateInjectionDelay(delayMs);
    if (m_textInjectorConfig.injectionDelayMs != valid) {
        m_textInjectorConfig.injectionDelayMs = valid;
        QSettings settings;
        settings.setValue(SettingsKeys::kTypingPreInjectionDelayMs.toString(), valid);
        settings.sync();
        emit textInjectorConfigChanged(m_textInjectorConfig);
        emit settingChanged(SettingsKeys::kTypingPreInjectionDelayMs.toString(), valid);
    }
}

void SettingsManager::setAudioFeedbackConfig(const AudioFeedbackConfig& config) {
    if (m_audioFeedbackConfig == config) {
        return;
    }
    m_audioFeedbackConfig = config;
    QSettings settings;
    settings.setValue(SettingsKeys::kAudioSoundEnabled.toString(), m_audioFeedbackConfig.soundEnabled);
    settings.sync();
    emit audioFeedbackConfigChanged(m_audioFeedbackConfig);
}

void SettingsManager::setSoundEnabled(bool enabled) {
    if (m_audioFeedbackConfig.soundEnabled != enabled) {
        m_audioFeedbackConfig.soundEnabled = enabled;
        QSettings settings;
        settings.setValue(SettingsKeys::kAudioSoundEnabled.toString(), enabled);
        settings.sync();
        emit audioFeedbackConfigChanged(m_audioFeedbackConfig);
        emit settingChanged(SettingsKeys::kAudioSoundEnabled.toString(), enabled);
    }
}

void SettingsManager::setNotificationConfig(const NotificationConfig& config) {
    if (m_notificationConfig == config) {
        return;
    }
    m_notificationConfig = config;
    QSettings settings;
    settings.setValue(SettingsKeys::kNotificationsEnabled.toString(), m_notificationConfig.notificationsEnabled);
    settings.sync();
    emit notificationConfigChanged(m_notificationConfig);
}

void SettingsManager::setNotificationsEnabled(bool enabled) {
    if (m_notificationConfig.notificationsEnabled != enabled) {
        m_notificationConfig.notificationsEnabled = enabled;
        QSettings settings;
        settings.setValue(SettingsKeys::kNotificationsEnabled.toString(), enabled);
        settings.sync();
        emit notificationConfigChanged(m_notificationConfig);
        emit settingChanged(SettingsKeys::kNotificationsEnabled.toString(), enabled);
    }
}

void SettingsManager::setCloudRouterConfig(const CloudRouterConfig& config) {
    if (m_cloudRouterConfig == config) {
        return;
    }
    m_cloudRouterConfig = config;
    QSettings settings;
    settings.setValue(SettingsKeys::kCloudActiveProvider.toString(), m_cloudRouterConfig.activeProvider);
    settings.sync();
    emit cloudRouterConfigChanged(m_cloudRouterConfig);
}

void SettingsManager::setActiveProvider(const QString& providerId) {
    if (m_cloudRouterConfig.activeProvider != providerId) {
        m_cloudRouterConfig.activeProvider = providerId;
        QSettings settings;
        settings.setValue(SettingsKeys::kCloudActiveProvider.toString(), providerId);
        settings.sync();
        emit cloudRouterConfigChanged(m_cloudRouterConfig);
        emit settingChanged(SettingsKeys::kCloudActiveProvider.toString(), providerId);
    }
}

void SettingsManager::setWhisperConfig(const WhisperConfig& config) {
    if (m_whisperConfig == config) {
        return;
    }
    m_whisperConfig = config;
    QSettings settings;
    settings.setValue(SettingsKeys::kWhisperSelectedModel.toString(), m_whisperConfig.selectedModel);
    settings.sync();
    emit whisperConfigChanged(m_whisperConfig);
}

void SettingsManager::setWhisperSelectedModel(const QString& modelId) {
    if (m_whisperConfig.selectedModel != modelId) {
        m_whisperConfig.selectedModel = modelId;
        QSettings settings;
        settings.setValue(SettingsKeys::kWhisperSelectedModel.toString(), modelId);
        settings.sync();
        emit whisperConfigChanged(m_whisperConfig);
        emit settingChanged(SettingsKeys::kWhisperSelectedModel.toString(), modelId);
    }
}

void SettingsManager::setSetting(const QString& key, const QVariant& value) {
    if (key == SettingsKeys::kSpeechBackend || key == SettingsKeys::kSpeechTranscriptionBackend) {
        if (value.metaType().id() == QMetaType::Int) {
            setBackend(value.toInt() == 1 ? TranscriptionBackend::WhisperCpp : TranscriptionBackend::Cloud);
        } else {
            setBackend(value.toString() == u"WhisperCpp"_s ? TranscriptionBackend::WhisperCpp
                                                           : TranscriptionBackend::Cloud);
        }
    } else if (key == SettingsKeys::kSpeechRecordingMode) {
        if (value.metaType().id() == QMetaType::Int) {
            setRecordingMode(value.toInt() == 1 ? RecordingMode::PushToTalk : RecordingMode::Toggle);
        } else {
            setRecordingMode(value.toString() == u"PushToTalk"_s ? RecordingMode::PushToTalk : RecordingMode::Toggle);
        }
    } else if (key == SettingsKeys::kSpeechAutoSwitchOffline) {
        setAutoSwitchOffline(value.toBool());
    } else if (key == SettingsKeys::kGroqModel) {
        setGroqModel(value.toString());
    } else if (key == SettingsKeys::kGroqLanguage) {
        setGroqLanguage(value.toString());
    } else if (key == SettingsKeys::kGroqCustomPrompt) {
        setGroqCustomPrompt(value.toString());
    } else if (key == SettingsKeys::kGeminiModel) {
        setGeminiModel(value.toString());
    } else if (key == SettingsKeys::kGeminiMode) {
        setGeminiMode(value.toString());
    } else if (key == SettingsKeys::kGeminiCustomVocabulary) {
        setGeminiCustomVocabulary(value.toString());
    } else if (key == SettingsKeys::kMetaMusaModel) {
        setMetaMusaModel(value.toString());
    } else if (key == SettingsKeys::kMetaMusaKeywords) {
        setMetaMusaKeywords(value.toString());
    } else if (key == SettingsKeys::kGroqLlmEnabled) {
        setLlmEnabled(value.toBool());
    } else if (key == SettingsKeys::kGroqLlmModel) {
        setLlmModel(value.toString());
    } else if (key == SettingsKeys::kGroqLlmPreset) {
        setLlmPreset(value.toString());
    } else if (key == SettingsKeys::kGroqLlmCustomPrompt) {
        setLlmCustomPrompt(value.toString());
    } else if (key == SettingsKeys::kGroqLlmTemperature) {
        setLlmTemperature(value.toDouble());
    } else if (key == SettingsKeys::kClipboardPreventHistory) {
        setPreventClipboardHistory(value.toBool());
    } else if (key == SettingsKeys::kTypingPreInjectionDelayMs) {
        setInjectionDelay(value.toInt());
    } else if (key == SettingsKeys::kAudioSoundEnabled) {
        setSoundEnabled(value.toBool());
    } else if (key == SettingsKeys::kNotificationsEnabled) {
        setNotificationsEnabled(value.toBool());
    } else if (key == SettingsKeys::kCloudActiveProvider) {
        setActiveProvider(value.toString());
    } else if (key == SettingsKeys::kWhisperSelectedModel) {
        setWhisperSelectedModel(value.toString());
    } else {
        QSettings settings;
        settings.setValue(key, value);
        settings.sync();
        emit settingChanged(key, value);
    }
}

QVariant SettingsManager::getSetting(const QString& key, const QVariant& defaultValue) const {
    if (key == SettingsKeys::kSpeechBackend) {
        return m_speechConfig.backend == TranscriptionBackend::WhisperCpp ? u"WhisperCpp"_s : u"Cloud"_s;
    }
    if (key == SettingsKeys::kSpeechTranscriptionBackend) {
        return static_cast<int>(m_speechConfig.backend);
    }
    if (key == SettingsKeys::kSpeechRecordingMode) {
        return m_speechConfig.recordingMode == RecordingMode::PushToTalk ? u"PushToTalk"_s : u"Toggle"_s;
    }
    if (key == SettingsKeys::kSpeechAutoSwitchOffline) {
        return m_speechConfig.autoSwitchOffline;
    }
    if (key == SettingsKeys::kGroqModel) {
        return m_groqSttConfig.model;
    }
    if (key == SettingsKeys::kGroqLanguage) {
        return m_groqSttConfig.language;
    }
    if (key == SettingsKeys::kGroqCustomPrompt) {
        return m_groqSttConfig.customPrompt;
    }
    if (key == SettingsKeys::kGeminiModel) {
        return m_geminiSttConfig.model;
    }
    if (key == SettingsKeys::kGeminiMode) {
        return m_geminiSttConfig.mode;
    }
    if (key == SettingsKeys::kGeminiCustomVocabulary) {
        return m_geminiSttConfig.customVocabulary;
    }
    if (key == SettingsKeys::kMetaMusaModel) {
        return m_metaMusaSttConfig.model;
    }
    if (key == SettingsKeys::kMetaMusaKeywords) {
        return m_metaMusaSttConfig.keywords;
    }
    if (key == SettingsKeys::kGroqLlmEnabled) {
        return m_llmConfig.enabled;
    }
    if (key == SettingsKeys::kGroqLlmModel) {
        return m_llmConfig.model;
    }
    if (key == SettingsKeys::kGroqLlmPreset) {
        return m_llmConfig.preset;
    }
    if (key == SettingsKeys::kGroqLlmCustomPrompt) {
        return m_llmConfig.customPrompt;
    }
    if (key == SettingsKeys::kGroqLlmTemperature) {
        return m_llmConfig.temperature;
    }
    if (key == SettingsKeys::kClipboardPreventHistory) {
        return m_textInjectorConfig.preventClipboardHistory;
    }
    if (key == SettingsKeys::kTypingPreInjectionDelayMs) {
        return m_textInjectorConfig.injectionDelayMs;
    }
    if (key == SettingsKeys::kAudioSoundEnabled) {
        return m_audioFeedbackConfig.soundEnabled;
    }
    if (key == SettingsKeys::kNotificationsEnabled) {
        return m_notificationConfig.notificationsEnabled;
    }
    if (key == SettingsKeys::kCloudActiveProvider) {
        return m_cloudRouterConfig.activeProvider;
    }
    if (key == SettingsKeys::kWhisperSelectedModel) {
        return m_whisperConfig.selectedModel;
    }

    const QSettings settings;
    return settings.value(key, defaultValue);
}

QVariantMap SettingsManager::allSettings() const {
    QVariantMap map;
    map.insert(SettingsKeys::kSpeechBackend.toString(),
               m_speechConfig.backend == TranscriptionBackend::WhisperCpp ? u"WhisperCpp"_s : u"Cloud"_s);
    map.insert(SettingsKeys::kSpeechTranscriptionBackend.toString(), static_cast<int>(m_speechConfig.backend));
    map.insert(SettingsKeys::kSpeechRecordingMode.toString(),
               m_speechConfig.recordingMode == RecordingMode::PushToTalk ? u"PushToTalk"_s : u"Toggle"_s);
    map.insert(SettingsKeys::kSpeechAutoSwitchOffline.toString(), m_speechConfig.autoSwitchOffline);
    map.insert(SettingsKeys::kGroqModel.toString(), m_groqSttConfig.model);
    map.insert(SettingsKeys::kGroqLanguage.toString(), m_groqSttConfig.language);
    map.insert(SettingsKeys::kGroqCustomPrompt.toString(), m_groqSttConfig.customPrompt);
    map.insert(SettingsKeys::kGeminiModel.toString(), m_geminiSttConfig.model);
    map.insert(SettingsKeys::kGeminiMode.toString(), m_geminiSttConfig.mode);
    map.insert(SettingsKeys::kGeminiCustomVocabulary.toString(), m_geminiSttConfig.customVocabulary);
    map.insert(SettingsKeys::kMetaMusaModel.toString(), m_metaMusaSttConfig.model);
    map.insert(SettingsKeys::kMetaMusaKeywords.toString(), m_metaMusaSttConfig.keywords);
    map.insert(SettingsKeys::kGroqLlmEnabled.toString(), m_llmConfig.enabled);
    map.insert(SettingsKeys::kGroqLlmModel.toString(), m_llmConfig.model);
    map.insert(SettingsKeys::kGroqLlmPreset.toString(), m_llmConfig.preset);
    map.insert(SettingsKeys::kGroqLlmCustomPrompt.toString(), m_llmConfig.customPrompt);
    map.insert(SettingsKeys::kGroqLlmTemperature.toString(), m_llmConfig.temperature);
    map.insert(SettingsKeys::kClipboardPreventHistory.toString(), m_textInjectorConfig.preventClipboardHistory);
    map.insert(SettingsKeys::kTypingPreInjectionDelayMs.toString(), m_textInjectorConfig.injectionDelayMs);
    map.insert(SettingsKeys::kAudioSoundEnabled.toString(), m_audioFeedbackConfig.soundEnabled);
    map.insert(SettingsKeys::kNotificationsEnabled.toString(), m_notificationConfig.notificationsEnabled);
    map.insert(SettingsKeys::kCloudActiveProvider.toString(), m_cloudRouterConfig.activeProvider);
    map.insert(SettingsKeys::kWhisperSelectedModel.toString(), m_whisperConfig.selectedModel);
    return map;
}
