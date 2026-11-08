#pragma once

#include "DaemonClient.h"

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

#include <optional>

class GuiHealthMonitor : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(SystemHealthMonitor)
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
    explicit GuiHealthMonitor(QObject* parent = nullptr);
    ~GuiHealthMonitor() override = default;

    [[nodiscard]] bool systemShortcutHasIssue() const noexcept;
    [[nodiscard]] bool systemShortcutSupported() const noexcept;
    [[nodiscard]] QString systemShortcutStatus() const;

    [[nodiscard]] bool directTypingHasIssue() const noexcept;
    [[nodiscard]] bool directTypingConnected() const noexcept;
    [[nodiscard]] bool directTypingFatalError() const noexcept;
    [[nodiscard]] QString directTypingStatus() const;

    [[nodiscard]] bool pushToTalkSupported() const noexcept;
    [[nodiscard]] bool isNetworkOnline() const noexcept;

    Q_INVOKABLE bool canRecord(bool notProcessing) const noexcept;

signals:
    void systemHealthChanged();
    void networkOnlineChanged(bool online);

private:
    std::optional<bool> m_networkOnlineOverride;
    bool m_networkOnline = true;
};
