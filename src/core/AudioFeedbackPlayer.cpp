#include "AudioFeedbackPlayer.h"

#include "common/SettingsKeys.h"
#include "SettingsManager.h"

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
    m_startChime->setVolume(0.8F);

    m_stopChime->setSource(QUrl(u"qrc:/qt/qml/QtScribe/assets/chime_stop.wav"_s));
    m_stopChime->setVolume(0.8F);

    if (auto* inst = SettingsManager::instance()) {
        setSettingsManager(inst);
    } else {
        loadSettings();
    }
}

void AudioFeedbackPlayer::setSettingsManager(SettingsManager* settings) {
    if (m_settingsManager == settings) {
        return;
    }
    if (m_settingsManager) {
        disconnect(m_settingsManager, nullptr, this, nullptr);
    }
    m_settingsManager = settings;
    if (m_settingsManager) {
        connect(m_settingsManager, &SettingsManager::audioFeedbackConfigChanged, this,
                [this](const SettingsManager::AudioFeedbackConfig& config) {
                    if (m_soundEnabled != config.soundEnabled) {
                        m_soundEnabled = config.soundEnabled;
                        emit soundEnabledChanged();
                    }
                });
    }
    loadSettings();
}

SettingsManager* AudioFeedbackPlayer::settingsManager() const noexcept {
    return m_settingsManager;
}

void AudioFeedbackPlayer::loadSettings() {
    bool sound = true;
    if (m_settingsManager) {
        sound = m_settingsManager->audioFeedbackConfig().soundEnabled;
    } else {
        const QSettings settings;
        sound = settings.value(SettingsKeys::kAudioSoundEnabled.toString(), true).toBool();
    }
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
        if (m_settingsManager) {
            m_settingsManager->setSoundEnabled(m_soundEnabled);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kAudioSoundEnabled.toString(), m_soundEnabled);
            settings.sync();
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
