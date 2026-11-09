#pragma once

#include <QObject>

class QSoundEffect;
class SettingsManager;

class AudioFeedbackPlayer : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool soundEnabled READ soundEnabled WRITE setSoundEnabled NOTIFY soundEnabledChanged FINAL)

public:
    explicit AudioFeedbackPlayer(QObject* parent = nullptr);
    ~AudioFeedbackPlayer() override = default;

    void setSettingsManager(SettingsManager* settings);
    [[nodiscard]] SettingsManager* settingsManager() const noexcept;

    bool soundEnabled() const;
    void setSoundEnabled(bool enabled);

    void loadSettings();

    void playStartSound();
    void playStopSound();

signals:
    void soundEnabledChanged();

private:
    SettingsManager* m_settingsManager = nullptr;
    QSoundEffect* m_startChime = nullptr;
    QSoundEffect* m_stopChime = nullptr;
    bool m_soundEnabled = true;
};
