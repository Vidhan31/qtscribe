#pragma once

#include <QString>
#include <QStringList>

class PresetProvider {
public:
    static QString systemPromptForPreset(const QString& preset, const QString& customPrompt = QString());

    static QString grammarPrompt();
    static QString bulletsPrompt();
    static QString professionalPrompt();
    static QString defaultCustomPrompt();

    static QString defaultPreset();
    static QStringList availablePresets();
};
