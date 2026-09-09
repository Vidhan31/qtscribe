#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>

#include <optional>

class AbstractSttClient;
class AudioRecorder;
class GlobalShortcutManager;
class DaemonConnector;

class SystemHealthMonitor : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool systemShortcutHasIssue READ systemShortcutHasIssue NOTIFY systemHealthChanged FINAL)
    Q_PROPERTY(bool systemShortcutSupported READ systemShortcutSupported NOTIFY systemHealthChanged FINAL)
    Q_PROPERTY(QString systemShortcutStatus READ systemShortcutStatus NOTIFY systemHealthChanged FINAL)
    Q_PROPERTY(bool directTypingHasIssue READ directTypingHasIssue NOTIFY systemHealthChanged FINAL)
    Q_PROPERTY(bool directTypingConnected READ directTypingConnected NOTIFY systemHealthChanged FINAL)
    Q_PROPERTY(bool directTypingFatalError READ directTypingFatalError NOTIFY systemHealthChanged FINAL)
    Q_PROPERTY(QString directTypingStatus READ directTypingStatus NOTIFY systemHealthChanged FINAL)
    Q_PROPERTY(bool pushToTalkSupported READ pushToTalkSupported NOTIFY systemHealthChanged FINAL)
    Q_PROPERTY(bool networkOnline READ isNetworkOnline NOTIFY networkOnlineChanged FINAL)

public:
    explicit SystemHealthMonitor(QObject* parent = nullptr);
    ~SystemHealthMonitor() override = default;

    void setShortcutManager(GlobalShortcutManager* mgr);
    void setDaemonConnector(DaemonConnector* connector);
    void setAudioRecorder(AudioRecorder* recorder);
    void setActiveSttClient(AbstractSttClient* client);

    bool systemShortcutHasIssue() const;
    bool systemShortcutSupported() const;
    QString systemShortcutStatus() const;

    bool directTypingHasIssue() const;
    bool directTypingConnected() const;
    bool directTypingFatalError() const;
    QString directTypingStatus() const;

    bool pushToTalkSupported() const;
    bool isNetworkOnline() const;
    void setNetworkOnlineForTesting(std::optional<bool> online);

    bool canRecord(bool notProcessing) const;

public slots:
    void notifyHealthChanged();

signals:
    void systemHealthChanged();
    void canRecordChanged();
    void networkOnlineChanged(bool online);

private slots:
    void onReachabilityChanged();

private:
    bool computeNetworkOnline() const;

    GlobalShortcutManager* m_shortcutMgr = nullptr;
    DaemonConnector* m_connector = nullptr;
    AudioRecorder* m_recorder = nullptr;
    AbstractSttClient* m_activeSttClient = nullptr;

    std::optional<bool> m_networkOnlineOverride;
    bool m_networkOnline = true;
};
