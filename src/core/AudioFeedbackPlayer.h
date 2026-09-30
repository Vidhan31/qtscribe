#pragma once

#include <QObject>
#include <QQmlEngine>

class QSoundEffect;

class AudioFeedbackPlayer : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool soundEnabled READ soundEnabled WRITE setSoundEnabled NOTIFY soundEnabledChanged FINAL)

public:
    explicit AudioFeedbackPlayer(QObject* parent = nullptr);
    ~AudioFeedbackPlayer() override = default;

    bool soundEnabled() const;
    void setSoundEnabled(bool enabled);

public slots:
    void playStartSound();
    void playStopSound();

signals:
    void soundEnabledChanged();

private:
    QSoundEffect* m_startChime = nullptr;
    QSoundEffect* m_stopChime = nullptr;
    bool m_soundEnabled = true;
};
