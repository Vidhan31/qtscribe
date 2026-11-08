#include "AudioFeedbackPlayer.h"

#include <QDebug>
#include <QSettings>
#include <QSoundEffect>
#include <QUrl>

using namespace Qt::StringLiterals;

AudioFeedbackPlayer::AudioFeedbackPlayer(QObject* parent)
    : QObject(parent)
    , m_startChime(new QSoundEffect(this))
    , m_stopChime(new QSoundEffect(this)) {
    m_startChime->setSource(QUrl(u"qrc:/qt/qml/QtScribe/assets/chime_start.wav"_s));
    m_startChime->setVolume(0.8f);

    m_stopChime->setSource(QUrl(u"qrc:/qt/qml/QtScribe/assets/chime_stop.wav"_s));
    m_stopChime->setVolume(0.8f);

    loadSettings();
}

void AudioFeedbackPlayer::loadSettings() {
    QSettings settings;
    const bool sound = settings.value(u"Audio/SoundEnabled"_s, true).toBool();
    if (m_soundEnabled != sound) {
        m_soundEnabled = sound;
        emit soundEnabledChanged();
    }
}

bool AudioFeedbackPlayer::soundEnabled() const {
    return m_soundEnabled;
}

void AudioFeedbackPlayer::setSoundEnabled(bool enabled) {
    if (m_soundEnabled != enabled) {
        m_soundEnabled = enabled;
        {
            QSettings settings;
            settings.setValue(u"Audio/SoundEnabled"_s, m_soundEnabled);
            settings.sync();
            if (settings.status() != QSettings::NoError) {
                qWarning() << "AudioFeedbackPlayer: Failed to save Audio/SoundEnabled:" << settings.status();
            }
        }
        emit soundEnabledChanged();
    }
}

void AudioFeedbackPlayer::playStartSound() {
    if (m_soundEnabled && m_startChime) {
        m_startChime->play();
    }
}

void AudioFeedbackPlayer::playStopSound() {
    if (m_soundEnabled && m_stopChime) {
        m_stopChime->play();
    }
}
