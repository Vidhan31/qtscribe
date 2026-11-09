#include "TextInjector.h"

#include "ClipboardManager.h"
#include "DaemonConnector.h"

#include "common/SettingsKeys.h"
#include "keyinjectord/protocol.h"
#include "SettingsManager.h"

#include <QDebug>
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

    if (auto* inst = SettingsManager::instance()) {
        setSettingsManager(inst);
    } else {
        loadSettings();
    }
}

void TextInjector::setSettingsManager(SettingsManager* settings) {
    if (m_settingsManager == settings) {
        return;
    }
    if (m_settingsManager) {
        disconnect(m_settingsManager, nullptr, this, nullptr);
    }
    m_settingsManager = settings;
    if (m_settingsManager) {
        connect(m_settingsManager, &SettingsManager::textInjectorConfigChanged, this,
                [this](const SettingsManager::TextInjectorConfig& config) {
                    m_preventClipboardHistory = config.preventClipboardHistory;
                    m_injectionDelay = config.injectionDelayMs;
                });
    }
    loadSettings();
}

SettingsManager* TextInjector::settingsManager() const noexcept {
    return m_settingsManager;
}

void TextInjector::loadSettings() {
    if (m_settingsManager) {
        const auto& config = m_settingsManager->textInjectorConfig();
        m_preventClipboardHistory = config.preventClipboardHistory;
        m_injectionDelay = config.injectionDelayMs;
    } else {
        const QSettings settings;
        m_preventClipboardHistory = settings.value(SettingsKeys::kClipboardPreventHistory.toString(), true).toBool();
        m_injectionDelay = settings.value(SettingsKeys::kTypingPreInjectionDelayMs.toString(), 200).toInt();
    }
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
        if (m_settingsManager) {
            m_settingsManager->setInjectionDelay(delayMs);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kTypingPreInjectionDelayMs.toString(), delayMs);
            settings.sync();
        }
    }
}

bool TextInjector::preventClipboardHistory() const noexcept {
    return m_preventClipboardHistory;
}

void TextInjector::setPreventClipboardHistory(bool prevent) {
    if (m_preventClipboardHistory != prevent) {
        m_preventClipboardHistory = prevent;
        if (m_settingsManager) {
            m_settingsManager->setPreventClipboardHistory(prevent);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kClipboardPreventHistory.toString(), prevent);
            settings.sync();
        }
    }
}

bool TextInjector::doInjectText(const QString& text) {
    if (text.isEmpty()) {
        return true;
    }

    if (!m_connector->isConnected()) {
        if (!m_connector->connectToServer()) {
            qWarning() << "TextInjector: Failed to connect to keyinjectord daemon:" << m_connector->lastError();
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
        qWarning() << "TextInjector: Paste command failed on keyinjectord:" << m_connector->lastError();
        m_clipboard->restore(savedText, hadContent);
        return false;
    }

    m_clipboard->scheduleRestore(savedText, hadContent, 800ms);
    return true;
}
