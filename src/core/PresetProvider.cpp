#include "PresetProvider.h"

using namespace Qt::StringLiterals;

QString PresetProvider::grammarPrompt() {
    return u"You are an expert speech-to-text post-processor. Your task is to fix spelling errors, speech recognition typos, homophone mistakes, missing punctuation, and capitalization in the transcribed text while strictly preserving the original meaning, intent, vocabulary, and tone. Output ONLY the corrected text. Do NOT wrap in quotes, do NOT add explanations, do NOT add introductory or concluding remarks."_s;
}

QString PresetProvider::bulletsPrompt() {
    return u"You are an expert speech-to-text post-processor. Your task is to organize and format the spoken transcription into clean markdown bullet points or numbered steps if sequential items are dictated. Group related points concisely. Output ONLY the formatted markdown list. Do NOT add any preamble, conversational remarks, or closing commentary."_s;
}

QString PresetProvider::professionalPrompt() {
    return u"You are an expert executive communication assistant. Your task is to polish the dictated speech into crisp, clear, and professional text suitable for emails, messages, or documents. Remove vocal filler words (e.g., um, uh, like), fix all grammar and punctuation, and enhance flow while preserving the core message. Output ONLY the polished text with NO preamble, quotes, or conversational commentary."_s;
}

QString PresetProvider::defaultCustomPrompt() {
    return u"You are a helpful speech post-processor. Clean up the transcribed speech while preserving its original meaning. Output ONLY the processed text with no extra conversational commentary."_s;
}

QString PresetProvider::defaultPreset() {
    return u"grammar"_s;
}

QStringList PresetProvider::availablePresets() {
    return {u"grammar"_s, u"bullets"_s, u"professional"_s, u"custom"_s};
}

QString PresetProvider::systemPromptForPreset(const QString& preset, const QString& customPrompt) {
    if (preset == u"bullets"_s) {
        return bulletsPrompt();
    }
    if (preset == u"professional"_s) {
        return professionalPrompt();
    }
    if (preset == u"custom"_s) {
        if (!customPrompt.trimmed().isEmpty()) {
            return customPrompt;
        }
        return defaultCustomPrompt();
    }
    return grammarPrompt();
}
