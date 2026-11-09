#pragma once

#include <QString>
#include <QStringList>

// Header-only so both the daemon (which applies prompts) and the GUI (which only
// previews them) can share the exact same preset definitions without pulling the
// core service layer into the GUI binary.
class PresetProvider {
public:
    static QString grammarPrompt() {
        return QStringLiteral(
            "You are an expert speech-to-text post-processor. Your task is to fix spelling errors, speech recognition "
            "typos, homophone mistakes, missing punctuation, and capitalization in the transcribed text while strictly "
            "preserving the original meaning, intent, vocabulary, and tone. Output ONLY the corrected text. Do NOT "
            "wrap in quotes, do NOT add explanations, do NOT add introductory or concluding remarks.");
    }

    static QString bulletsPrompt() {
        return QStringLiteral(
            "You are an expert speech-to-text post-processor. Your task is to organize and format the spoken "
            "transcription into clean markdown bullet points or numbered steps if sequential items are dictated. Group "
            "related points concisely. Output ONLY the formatted markdown list. Do NOT add any preamble, "
            "conversational remarks, or closing commentary.");
    }

    static QString professionalPrompt() {
        return QStringLiteral(
            "You are an expert executive communication assistant. Your task is to polish the dictated speech into "
            "crisp, clear, and professional text suitable for emails, messages, or documents. Remove vocal filler "
            "words (e.g., um, uh, like), fix all grammar and punctuation, and enhance flow while preserving the core "
            "message. Output ONLY the polished text with NO preamble, quotes, or conversational commentary.");
    }

    static QString defaultCustomPrompt() {
        return QStringLiteral(
            "You are a helpful speech post-processor. Clean up the transcribed speech while preserving its original "
            "meaning. Output ONLY the processed text with no extra conversational commentary.");
    }

    static QString defaultPreset() { return QStringLiteral("grammar"); }

    static QStringList availablePresets() {
        return {QStringLiteral("grammar"), QStringLiteral("bullets"), QStringLiteral("professional"),
                QStringLiteral("custom")};
    }

    static QString systemPromptForPreset(const QString& preset, const QString& customPrompt = QString()) {
        if (preset == QStringLiteral("bullets")) {
            return bulletsPrompt();
        }
        if (preset == QStringLiteral("professional")) {
            return professionalPrompt();
        }
        if (preset == QStringLiteral("custom")) {
            if (!customPrompt.trimmed().isEmpty()) {
                return customPrompt;
            }
            return defaultCustomPrompt();
        }
        return grammarPrompt();
    }
};
