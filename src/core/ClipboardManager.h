#pragma once

#include <QObject>
#include <QString>

#include <chrono>

class QTimer;

class ClipboardManager : public QObject {
    Q_OBJECT

public:
    explicit ClipboardManager(QObject* parent = nullptr);
    ~ClipboardManager() override;

    QString backupText() const;
    bool setText(const QString& text, bool sensitive = true);
    void restore(const QString& backup, bool hadContent);

    void scheduleRestore(const QString& backupText, bool hadContent,
                         std::chrono::milliseconds delay = std::chrono::milliseconds(800));
    void cancelPendingRestore();
    bool hasActiveBackup() const;

    QString lastError() const;
    QString wlToolPath(const QString& toolName) const;

signals:
    void lastErrorChanged(const QString& error);

private:
    void setLastError(const QString& error) const;

    QTimer* m_restoreTimer = nullptr;
    QString m_savedClipboardText;
    bool m_hasActiveBackup = false;
    bool m_hadClipboardContent = false;
    mutable QString m_lastError;
};
