#include "TextInjector.h"

#include "ClipboardManager.h"
#include "DaemonConnector.h"

#include "keyinjectord/protocol.h"

#include <QSettings>
#include <QTimer>

#include <algorithm>
#include <chrono>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

TextInjector::TextInjector(DaemonConnector* connector, ClipboardManager* clipboard, QObject* parent)
    : AbstractTextInjector(parent)
    , m_connector(connector ? connector : new DaemonConnector(this))
    , m_clipboard(clipboard ? clipboard : new ClipboardManager(this))
    , m_injectionTimer(new QTimer(this)) {
    m_injectionTimer->setSingleShot(true);
    connect(m_injectionTimer, &QTimer::timeout, this, [this]() {
        const QString text = m_pendingText;
        m_pendingText.clear();
        doInjectText(text);
    });

    QSettings settings;
    m_preventClipboardHistory = settings.value(u"Clipboard/PreventHistory"_s, true).toBool();
    m_injectionDelay = settings.value(u"Typing/PreInjectionDelayMs"_s, 200).toInt();
}

bool TextInjector::inject(const QString& text) {
    if (text.isEmpty()) {
        return true;
    }

    if (m_injectionDelay <= 0) {
        cancel();
        return doInjectText(text);
    }

    if (m_injectionTimer->isActive()) {
        m_injectionTimer->stop();
    }

    m_pendingText = text;
    m_injectionTimer->start(std::chrono::milliseconds(m_injectionDelay));
    return true;
}

void TextInjector::cancel() {
    if (m_injectionTimer && m_injectionTimer->isActive()) {
        m_injectionTimer->stop();
    }
    m_pendingText.clear();
}

int TextInjector::injectionDelay() const noexcept {
    return m_injectionDelay;
}

void TextInjector::setInjectionDelay(int delayMs) {
    delayMs = std::clamp(delayMs, 0, 10000);
    if (m_injectionDelay != delayMs) {
        m_injectionDelay = delayMs;
        QSettings settings;
        settings.setValue(u"Typing/PreInjectionDelayMs"_s, delayMs);
    }
}

bool TextInjector::preventClipboardHistory() const noexcept {
    return m_preventClipboardHistory;
}

void TextInjector::setPreventClipboardHistory(bool prevent) {
    if (m_preventClipboardHistory != prevent) {
        m_preventClipboardHistory = prevent;
        QSettings settings;
        settings.setValue(u"Clipboard/PreventHistory"_s, prevent);
    }
}

bool TextInjector::doInjectText(const QString& text) {
    if (text.isEmpty()) {
        return true;
    }

    if (!m_connector->isConnected()) {
        if (!m_connector->connectToServer()) {
            return false;
        }
    }

    QString savedText;
    bool hadContent = false;
    if (!m_clipboard->hasActiveBackup()) {
        savedText = m_clipboard->backupText();
        hadContent = !savedText.isEmpty();
    }

    if (!m_clipboard->setText(text, m_preventClipboardHistory)) {
        return false;
    }

    bool pasteOk = m_connector->sendCommand(keyinjectord::Opcode::Paste);
    if (!pasteOk && !m_connector->isConnected()) {
        if (m_connector->connectToServer()) {
            pasteOk = m_connector->sendCommand(keyinjectord::Opcode::Paste);
        }
    }

    if (!pasteOk) {
        m_clipboard->restore(savedText, hadContent);
        return false;
    }

    m_clipboard->scheduleRestore(savedText, hadContent, 800ms);
    return true;
}
