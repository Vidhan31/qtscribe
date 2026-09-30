#include "AudioFeedbackPlayer.h"

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

    QSettings settings;
    m_soundEnabled = settings.value(u"Audio/SoundEnabled"_s, true).toBool();
}

bool AudioFeedbackPlayer::soundEnabled() const {
    return m_soundEnabled;
}

void AudioFeedbackPlayer::setSoundEnabled(bool enabled) {
    if (m_soundEnabled != enabled) {
        m_soundEnabled = enabled;
        QSettings settings;
        settings.setValue(u"Audio/SoundEnabled"_s, m_soundEnabled);
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
