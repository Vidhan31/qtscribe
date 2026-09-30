#pragma once

#include "AbstractTextInjector.h"

class ClipboardManager;
class DaemonConnector;
class QTimer;

class TextInjector : public AbstractTextInjector {
    Q_OBJECT

public:
    explicit TextInjector(DaemonConnector* connector = nullptr, ClipboardManager* clipboard = nullptr,
                          QObject* parent = nullptr);
    ~TextInjector() override = default;

    bool inject(const QString& text) override;
    void cancel() override;

    [[nodiscard]] int injectionDelay() const noexcept;
    void setInjectionDelay(int delayMs);

    [[nodiscard]] bool preventClipboardHistory() const noexcept;
    void setPreventClipboardHistory(bool prevent);

private:
    bool doInjectText(const QString& text);

    DaemonConnector* m_connector = nullptr;
    ClipboardManager* m_clipboard = nullptr;
    QTimer* m_injectionTimer = nullptr;
    QString m_pendingText;
    int m_injectionDelay = 200;
    bool m_preventClipboardHistory = true;
};
