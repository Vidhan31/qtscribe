#include "SpeechClient.h"

#include "common/SettingsKeys.h"

#include <QCoreApplication>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDebug>
#include <QSettings>

using namespace Qt::StringLiterals;

static const auto kServiceName = u"io.github.qtscribe.SpeechService"_s;
static const auto kObjectPath = u"/io/github/qtscribe/SpeechService"_s;
static const auto kInterface = u"io.github.qtscribe.SpeechService"_s;

SpeechClient::SpeechClient(QObject* parent)
    : QObject(parent) {
    s_instance = this;

    const QSettings settings;
    m_activeBackend =
        (settings.value(SettingsKeys::kSpeechBackend.toString(), u"Cloud"_s).toString() == u"WhisperCpp"_s) ? 1 : 0;
    m_recordingMode =
        (settings.value(SettingsKeys::kSpeechRecordingMode.toString(), u"Toggle"_s).toString() == u"PushToTalk"_s) ? 1
                                                                                                                   : 0;
    m_autoSwitchOffline = settings.value(SettingsKeys::kSpeechAutoSwitchOffline.toString(), true).toBool();
    m_statusMessage = tr("Connecting to service…");

    const bool disableRemoteAttach = !qEnvironmentVariableIsEmpty("QTSCRIBE_DISABLE_REMOTE_ATTACH") &&
                                     qEnvironmentVariable("QTSCRIBE_DISABLE_REMOTE_ATTACH") != "0";
    if (disableRemoteAttach) {
        m_statusMessage = tr("Background service offline");
        return;
    }

    if (QDBusConnection::sessionBus().isConnected()) {
        m_watcher = new QDBusServiceWatcher(
            kServiceName, QDBusConnection::sessionBus(),
            QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration, this);
        connect(m_watcher, &QDBusServiceWatcher::serviceRegistered, this, &SpeechClient::onServiceRegistered);
        connect(m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, &SpeechClient::onServiceUnregistered);

        auto* iface = QDBusConnection::sessionBus().interface();
        if (iface && iface->isServiceRegistered(kServiceName)) {
            onServiceRegistered(kServiceName);
        }
    }
}

SpeechClient::~SpeechClient() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

SpeechClient* SpeechClient::instance() {
    if (!s_instance) {
        s_instance = new SpeechClient(QCoreApplication::instance());
    }
    return s_instance;
}

SpeechClient* SpeechClient::create(QQmlEngine* /*qmlEngine*/, QJSEngine* /*jsEngine*/) {
    return instance();
}

bool SpeechClient::isConnected() const noexcept {
    return m_connected;
}

int SpeechClient::activeBackend() const noexcept {
    return m_activeBackend;
}

int SpeechClient::recordingMode() const noexcept {
    return m_recordingMode;
}

int SpeechClient::dictationState() const noexcept {
    return m_dictationState;
}

bool SpeechClient::isBusy() const noexcept {
    return m_dictationState == static_cast<int>(DictationState::Recording) ||
           m_dictationState == static_cast<int>(DictationState::Transcribing) ||
           m_dictationState == static_cast<int>(DictationState::Enhancing);
}

bool SpeechClient::canRecord() const noexcept {
    return m_connected && m_canRecord;
}

qreal SpeechClient::audioLevel() const noexcept {
    return m_audioLevel;
}

QString SpeechClient::statusMessage() const {
    return m_statusMessage;
}

QString SpeechClient::lastError() const {
    return m_lastError;
}

bool SpeechClient::recording() const noexcept {
    return m_dictationState == static_cast<int>(DictationState::Recording);
}

bool SpeechClient::transcribing() const noexcept {
    return m_dictationState == static_cast<int>(DictationState::Transcribing);
}

bool SpeechClient::enhancing() const noexcept {
    return m_dictationState == static_cast<int>(DictationState::Enhancing);
}

QString SpeechClient::lastTranscription() const {
    return m_lastTranscription;
}

bool SpeechClient::autoSwitchOffline() const noexcept {
    return m_autoSwitchOffline;
}

bool SpeechClient::clientWindowFocused() const noexcept {
    return m_clientWindowFocused;
}

bool SpeechClient::cloudApiKeySet() const noexcept {
    return m_cloudApiKeySet;
}

bool SpeechClient::cloudApiKeyInvalid() const noexcept {
    return m_cloudApiKeyInvalid;
}

bool SpeechClient::cloudRateLimited() const noexcept {
    return m_cloudRateLimited;
}

int SpeechClient::cloudRetrySecondsRemaining() const noexcept {
    return m_cloudRetrySecondsRemaining;
}

QString SpeechClient::cloudLastError() const {
    return m_cloudLastError;
}

QString SpeechClient::historyJson() const {
    return m_historyJson;
}

bool SpeechClient::sendRemoteCommand(const QString& command) {
    if (!QDBusConnection::sessionBus().isConnected()) {
        return false;
    }
    QDBusInterface iface(kServiceName, kObjectPath, kInterface, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        return false;
    }
    const QDBusMessage reply = iface.call(command);
    return reply.type() != QDBusMessage::ErrorMessage;
}

void SpeechClient::setActiveBackend(int backend, bool persistSettings) {
    if (m_activeBackend != backend) {
        m_activeBackend = backend;
        emit activeBackendChanged();
    }
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"setActiveBackend"_s, backend, persistSettings);
    }
}

void SpeechClient::setRecordingMode(int mode) {
    if (m_recordingMode != mode) {
        m_recordingMode = mode;
        emit recordingModeChanged();
    }
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"setRecordingMode"_s, mode);
    }
}

void SpeechClient::setAutoSwitchOffline(bool enable) {
    if (m_autoSwitchOffline != enable) {
        m_autoSwitchOffline = enable;
        emit autoSwitchOfflineChanged();
    }
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"setAutoSwitchOffline"_s, enable);
    }
}

void SpeechClient::setClientWindowFocused(bool focused) {
    if (m_clientWindowFocused != focused) {
        m_clientWindowFocused = focused;
        emit clientWindowFocusedChanged();
    }
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"setClientWindowFocused"_s, focused);
    }
}

void SpeechClient::toggleRecording() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"toggleRecording"_s);
    }
}

void SpeechClient::startRecording() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"startRecording"_s);
    }
}

void SpeechClient::stopRecording() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"stopRecording"_s);
    }
}

void SpeechClient::cancelDictation() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"cancelDictation"_s);
    }
}

void SpeechClient::retryTranscription() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"retryTranscription"_s);
    }
}

void SpeechClient::clearLastTranscription() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"clearLastTranscription"_s);
    }
}

void SpeechClient::clearError() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"clearError"_s);
    }
}

void SpeechClient::clearHistory() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"clearHistory"_s);
    }
}

void SpeechClient::reloadSettings() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"reloadSettings"_s);
    }
}

void SpeechClient::onServiceRegistered(const QString& serviceName) {
    if (serviceName != kServiceName) {
        return;
    }

    if (!m_interface) {
        m_interface = new QDBusInterface(kServiceName, kObjectPath, kInterface, QDBusConnection::sessionBus(), this);
    }

    setupDBusConnections();
    m_connected = true;
    emit connectedChanged();
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"setClientWindowFocused"_s, m_clientWindowFocused);
    }
    syncFromDaemon();
}

void SpeechClient::onServiceUnregistered(const QString& serviceName) {
    if (serviceName != kServiceName) {
        return;
    }

    m_connected = false;
    m_canRecord = false;
    m_audioLevel = 0.0;
    m_statusMessage = tr("Background service offline");
    m_cloudApiKeySet = false;
    m_cloudApiKeyInvalid = false;
    m_cloudRateLimited = false;
    m_cloudRetrySecondsRemaining = 0;
    m_cloudLastError.clear();
    m_historyJson = QStringLiteral("[]");

    emit connectedChanged();
    emit canRecordChanged();
    emit audioLevelChanged(0.0);
    emit statusMessageChanged();
    emit cloudStatusChanged();
    emit historyChanged();
}

void SpeechClient::setupDBusConnections() {
    if (!QDBusConnection::sessionBus().isConnected()) {
        return;
    }

    auto bus = QDBusConnection::sessionBus();

    bus.connect(kServiceName, kObjectPath, u"org.freedesktop.DBus.Properties"_s, u"PropertiesChanged"_s, this,
                SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));

    bus.connect(kServiceName, kObjectPath, kInterface, u"activeBackendChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"recordingModeChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"dictationStateChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"canRecordChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"audioLevelChanged"_s, this, SLOT(onAudioLevelChanged(double)));
    bus.connect(kServiceName, kObjectPath, kInterface, u"statusMessageChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"lastErrorChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"recordingChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"transcribingChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"enhancingChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"lastTranscriptionChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"autoSwitchOfflineChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"clientWindowFocusedChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"cloudStatusChanged"_s, this, SLOT(syncFromDaemon()));

    bus.connect(kServiceName, kObjectPath, kInterface, u"transcriptionFinished"_s, this,
                SIGNAL(transcriptionFinished(QString)));

    bus.connect(kServiceName, kObjectPath, kInterface, u"historyCleared"_s, this, SIGNAL(historyCleared()));

    bus.connect(kServiceName, kObjectPath, kInterface, u"historyChanged"_s, this, SLOT(syncFromDaemon()));

    bus.connect(kServiceName, kObjectPath, kInterface, u"errorOccurred"_s, this, SIGNAL(errorOccurred(QString)));
}

void SpeechClient::onAudioLevelChanged(double level) {
    m_audioLevel = level;
    emit audioLevelChanged(m_audioLevel);
}

void SpeechClient::syncFromDaemon() {
    if (!m_interface || !m_interface->isValid()) {
        return;
    }

    m_activeBackend = m_interface->property("activeBackend").toInt();
    m_recordingMode = m_interface->property("recordingMode").toInt();
    m_dictationState = m_interface->property("dictationState").toInt();
    m_canRecord = m_interface->property("canRecord").toBool();
    m_audioLevel = m_interface->property("audioLevel").toReal();
    m_statusMessage = m_interface->property("statusMessage").toString();
    m_lastError = m_interface->property("lastError").toString();
    m_lastTranscription = m_interface->property("lastTranscription").toString();
    m_autoSwitchOffline = m_interface->property("autoSwitchOffline").toBool();

    m_cloudApiKeySet = m_interface->property("cloudApiKeySet").toBool();
    m_cloudApiKeyInvalid = m_interface->property("cloudApiKeyInvalid").toBool();
    m_cloudRateLimited = m_interface->property("cloudRateLimited").toBool();
    m_cloudRetrySecondsRemaining = m_interface->property("cloudRetrySecondsRemaining").toInt();
    m_cloudLastError = m_interface->property("cloudLastError").toString();

    const QVariant historyProp = m_interface->property("historyJson");
    if (historyProp.isValid()) {
        m_historyJson = historyProp.toString();
    } else {
        const QDBusMessage reply = m_interface->call(u"getHistoryJson"_s);
        if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
            m_historyJson = reply.arguments().at(0).toString();
        }
    }

    emit activeBackendChanged();
    emit recordingModeChanged();
    emit dictationStateChanged();
    emit recordingChanged();
    emit transcribingChanged();
    emit enhancingChanged();
    emit canRecordChanged();
    emit audioLevelChanged(m_audioLevel);
    emit statusMessageChanged();
    emit lastErrorChanged();
    emit lastTranscriptionChanged();
    emit autoSwitchOfflineChanged();
    emit cloudStatusChanged();
    emit historyChanged();
}

void SpeechClient::onPropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties,
                                       const QStringList& invalidatedProperties) {
    Q_UNUSED(invalidatedProperties);
    if (interfaceName != kInterface) {
        return;
    }

    bool stateChanged = false;
    bool cloudChanged = false;

    for (auto it = changedProperties.constBegin(); it != changedProperties.constEnd(); ++it) {
        const QString& key = it.key();
        const QVariant& val = it.value();

        if (key == u"activeBackend"_s) {
            m_activeBackend = val.toInt();
            emit activeBackendChanged();
        } else if (key == u"recordingMode"_s) {
            m_recordingMode = val.toInt();
            emit recordingModeChanged();
        } else if (key == u"dictationState"_s) {
            m_dictationState = val.toInt();
            stateChanged = true;
        } else if (key == u"canRecord"_s) {
            m_canRecord = val.toBool();
            emit canRecordChanged();
        } else if (key == u"audioLevel"_s) {
            m_audioLevel = val.toReal();
            emit audioLevelChanged(m_audioLevel);
        } else if (key == u"statusMessage"_s) {
            m_statusMessage = val.toString();
            emit statusMessageChanged();
        } else if (key == u"lastError"_s) {
            m_lastError = val.toString();
            emit lastErrorChanged();
        } else if (key == u"lastTranscription"_s) {
            m_lastTranscription = val.toString();
            emit lastTranscriptionChanged();
        } else if (key == u"autoSwitchOffline"_s) {
            m_autoSwitchOffline = val.toBool();
            emit autoSwitchOfflineChanged();
        } else if (key == u"clientWindowFocused"_s) {
            m_clientWindowFocused = val.toBool();
            emit clientWindowFocusedChanged();
        } else if (key == u"cloudApiKeySet"_s) {
            m_cloudApiKeySet = val.toBool();
            cloudChanged = true;
        } else if (key == u"cloudApiKeyInvalid"_s) {
            m_cloudApiKeyInvalid = val.toBool();
            cloudChanged = true;
        } else if (key == u"cloudRateLimited"_s) {
            m_cloudRateLimited = val.toBool();
            cloudChanged = true;
        } else if (key == u"cloudRetrySecondsRemaining"_s) {
            m_cloudRetrySecondsRemaining = val.toInt();
            cloudChanged = true;
        } else if (key == u"cloudLastError"_s) {
            m_cloudLastError = val.toString();
            cloudChanged = true;
        } else if (key == u"historyJson"_s) {
            m_historyJson = val.toString();
            emit historyChanged();
        }
    }

    if (stateChanged) {
        emit dictationStateChanged();
        emit recordingChanged();
        emit transcribingChanged();
        emit enhancingChanged();
    }
    if (cloudChanged) {
        emit cloudStatusChanged();
    }
}
