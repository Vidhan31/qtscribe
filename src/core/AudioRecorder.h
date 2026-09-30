#pragma once

#include <QAudioFormat>
#include <QByteArray>
#include <QObject>
#include <QQmlEngine>
#include <QString>

class QAudioSource;
class QTimer;

class AudioRecorder : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged FINAL)
    Q_PROPERTY(bool hasAudioInputDevice READ hasAudioInputDevice NOTIFY hasAudioInputDeviceChanged FINAL)
    Q_PROPERTY(QString currentDeviceName READ currentDeviceName NOTIFY currentDeviceNameChanged FINAL)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged FINAL)
    Q_PROPERTY(qreal audioLevel READ audioLevel NOTIFY audioLevelChanged FINAL)

public:
    explicit AudioRecorder(QObject* parent = nullptr);
    ~AudioRecorder() override;

    bool recording() const;
    bool hasAudioInputDevice() const;
    QString currentDeviceName() const;
    QString statusMessage() const;
    qreal audioLevel() const;

public slots:
    virtual void startRecording();
    virtual void stopRecording();
    virtual void cancelRecording();

signals:
    void recordingFinished(const QByteArray& wavData);
    void recordingChanged();
    void hasAudioInputDeviceChanged();
    void currentDeviceNameChanged();
    void audioInputDeviceDisconnected(const QString& deviceName, const QString& fallbackDeviceName);
    void statusMessageChanged();
    void audioLevelChanged();
    void maxDurationReached();

protected:
    void setRecording(bool recording);
    void setHasAudioInputDevice(bool hasDevice);
    void setCurrentDeviceName(const QString& name);
    void setStatusMessage(const QString& message);
    void setAudioLevel(qreal level);

private slots:
    void onReadyRead();
    void onAudioInputsChanged();

private:
    QByteArray buildWavFile(const QByteArray& pcmData) const;
    void cleanupRecording();

    QAudioFormat m_format;
    QAudioSource* m_source = nullptr;
    QIODevice* m_ioDevice = nullptr;
    QByteArray m_pcmData;

    bool m_recording = false;
    bool m_hasAudioInputDevice = false;
    QByteArray m_currentDeviceId;
    QString m_currentDeviceName;
    QString m_statusMessage;
    qreal m_audioLevel = 0.0;
    QTimer* m_maxDurationTimer = nullptr;

    static constexpr int kSampleRate = 16000;
    static constexpr int kChannelCount = 1;
    static constexpr auto kMaxRecordingDuration = std::chrono::minutes(5);
};
