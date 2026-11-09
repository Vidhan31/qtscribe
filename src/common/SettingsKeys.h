#pragma once

#include <QStringView>

namespace SettingsKeys {

// Speech & Recording
inline constexpr QStringView kSpeechBackend = u"Speech/Backend";
inline constexpr QStringView kSpeechTranscriptionBackend = u"Speech/TranscriptionBackend";
inline constexpr QStringView kSpeechRecordingMode = u"Speech/RecordingMode";
inline constexpr QStringView kSpeechAutoSwitchOffline = u"Speech/AutoSwitchOffline";

// Whisper Offline Model
inline constexpr QStringView kWhisperSelectedModel = u"Whisper/SelectedModel";

// Cloud Provider Routing
inline constexpr QStringView kCloudActiveProvider = u"Cloud/ActiveProvider";

// Groq Speech-to-Text
inline constexpr QStringView kGroqModel = u"Groq/Model";
inline constexpr QStringView kGroqLanguage = u"Groq/Language";
inline constexpr QStringView kGroqCustomPrompt = u"Groq/CustomPrompt";

// Gemini Speech-to-Text
inline constexpr QStringView kGeminiModel = u"Gemini/Model";
inline constexpr QStringView kGeminiMode = u"Gemini/Mode";
inline constexpr QStringView kGeminiCustomVocabulary = u"Gemini/CustomVocabulary";

// LLM Enhancement
inline constexpr QStringView kGroqLlmEnabled = u"Groq/LlmEnabled";
inline constexpr QStringView kGroqLlmModel = u"Groq/LlmModel";
inline constexpr QStringView kGroqLlmPreset = u"Groq/LlmPreset";
inline constexpr QStringView kGroqLlmCustomPrompt = u"Groq/LlmCustomPrompt";
inline constexpr QStringView kGroqLlmTemperature = u"Groq/LlmTemperature";

// Audio Feedback
inline constexpr QStringView kAudioSoundEnabled = u"Audio/SoundEnabled";

// System Notifications
inline constexpr QStringView kNotificationsEnabled = u"notifications/enabled";

// Typing & Clipboard Injection
inline constexpr QStringView kClipboardPreventHistory = u"Clipboard/PreventHistory";
inline constexpr QStringView kTypingPreInjectionDelayMs = u"Typing/PreInjectionDelayMs";

// UI Theme
inline constexpr QStringView kUiThemeMode = u"ui/themeMode";

// API Keys
inline constexpr QStringView kGroqSettingsKey = u"Cloud/Providers/Groq/ApiKey";
inline constexpr QStringView kGroqLegacySettingsKey = u"Groq/ApiKey";
inline constexpr QStringView kGeminiSettingsKey = u"Cloud/Providers/Gemini/ApiKey";

} // namespace SettingsKeys
