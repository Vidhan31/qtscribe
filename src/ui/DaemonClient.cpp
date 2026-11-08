#include "DaemonClient.h"

#include <QCoreApplication>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDebug>
#include <QSettings>

using namespace Qt::StringLiterals;

static const auto kServiceName = u"io.github.qtscribe.SpeechService"_s;
static const auto kObjectPath = u"/io/github/qtscribe/SpeechService"_s;
static const auto kInterface = u"io.github.qtscribe.SpeechService"_s;

DaemonClient::DaemonClient(QObject* parent)
    : QObject(parent) {
    s_instance = this;

    QSettings settings;
    m_activeBackend = (settings.value(u"Speech/Backend"_s, u"Cloud"_s).toString() == u"WhisperCpp"_s) ? 1 : 0;
    m_recordingMode = (settings.value(u"Speech/RecordingMode"_s, u"Toggle"_s).toString() == u"PushToTalk"_s) ? 1 : 0;
    m_autoSwitchOffline = settings.value(u"Speech/AutoSwitchOffline"_s, true).toBool();
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
        connect(m_watcher, &QDBusServiceWatcher::serviceRegistered, this, &DaemonClient::onServiceRegistered);
        connect(m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, &DaemonClient::onServiceUnregistered);

        auto* iface = QDBusConnection::sessionBus().interface();
        if (iface && iface->isServiceRegistered(kServiceName)) {
            onServiceRegistered(kServiceName);
        }
    }
}

DaemonClient::~DaemonClient() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

DaemonClient* DaemonClient::instance() {
    if (!s_instance) {
        s_instance = new DaemonClient(QCoreApplication::instance());
    }
    return s_instance;
}

DaemonClient* DaemonClient::create(QQmlEngine* /*qmlEngine*/, QJSEngine* /*jsEngine*/) {
    return instance();
}

bool DaemonClient::isConnected() const noexcept {
    return m_connected;
}

int DaemonClient::activeBackend() const noexcept {
    return m_activeBackend;
}

int DaemonClient::recordingMode() const noexcept {
    return m_recordingMode;
}

int DaemonClient::dictationState() const noexcept {
    return m_dictationState;
}

bool DaemonClient::isBusy() const noexcept {
    return m_dictationState == static_cast<int>(DictationState::Recording) ||
           m_dictationState == static_cast<int>(DictationState::Transcribing) ||
           m_dictationState == static_cast<int>(DictationState::Enhancing);
}

bool DaemonClient::canRecord() const noexcept {
    return m_connected && m_canRecord;
}

qreal DaemonClient::audioLevel() const noexcept {
    return m_audioLevel;
}

QString DaemonClient::statusMessage() const {
    return m_statusMessage;
}

QString DaemonClient::lastError() const {
    return m_lastError;
}

bool DaemonClient::recording() const noexcept {
    return m_dictationState == static_cast<int>(DictationState::Recording);
}

bool DaemonClient::transcribing() const noexcept {
    return m_dictationState == static_cast<int>(DictationState::Transcribing);
}

bool DaemonClient::enhancing() const noexcept {
    return m_dictationState == static_cast<int>(DictationState::Enhancing);
}

QString DaemonClient::lastTranscription() const {
    return m_lastTranscription;
}

bool DaemonClient::autoSwitchOffline() const noexcept {
    return m_autoSwitchOffline;
}

bool DaemonClient::clientWindowFocused() const noexcept {
    return m_clientWindowFocused;
}

bool DaemonClient::pushToTalkSupported() const noexcept {
    return m_pushToTalkSupported;
}

bool DaemonClient::shortcutAvailable() const noexcept {
    return m_shortcutAvailable;
}

bool DaemonClient::shortcutSupported() const noexcept {
    return m_shortcutSupported;
}

QString DaemonClient::shortcutStatusMessage() const {
    return m_shortcutStatusMessage;
}

bool DaemonClient::systemShortcutHasIssue() const noexcept {
    return !m_shortcutSupported || !m_shortcutAvailable;
}

bool DaemonClient::directTypingConnected() const noexcept {
    return m_directTypingConnected;
}

bool DaemonClient::directTypingHasFatalError() const noexcept {
    return m_directTypingHasFatalError;
}

QString DaemonClient::directTypingFatalErrorMessage() const {
    return m_directTypingFatalErrorMessage;
}

QString DaemonClient::directTypingStatus() const {
    return m_directTypingStatus;
}

bool DaemonClient::directTypingHasIssue() const noexcept {
    return m_directTypingHasFatalError || !m_directTypingConnected;
}

bool DaemonClient::isModelLoaded() const noexcept {
    return m_isModelLoaded;
}

bool DaemonClient::isModelBusy() const noexcept {
    return m_isModelBusy;
}

QString DaemonClient::loadedModelPath() const {
    return m_loadedModelPath;
}

QString DaemonClient::modelPath() const {
    return m_modelPath;
}

QString DaemonClient::modelFileName() const {
    return m_modelFileName;
}

QString DaemonClient::computeDevice() const {
    return m_computeDevice;
}

bool DaemonClient::isVulkanSupported() const noexcept {
    return m_isVulkanSupported;
}

bool DaemonClient::isModelInstalled() const noexcept {
    return m_isModelInstalled;
}

void DaemonClient::setActiveBackend(int backend, bool persistSettings) {
    m_activeBackend = backend;
    if (persistSettings) {
        QSettings settings;
        settings.setValue(u"Speech/TranscriptionBackend"_s, backend);
        settings.setValue(u"Speech/Backend"_s, (backend == 1) ? u"WhisperCpp"_s : u"Cloud"_s);
    }
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"setActiveBackend"_s, backend, persistSettings);
    }
    emit activeBackendChanged();
}

void DaemonClient::setRecordingMode(int mode) {
    if (mode == static_cast<int>(RecordingMode::PushToTalk) && !m_pushToTalkSupported) {
        mode = static_cast<int>(RecordingMode::Toggle);
    }
    m_recordingMode = mode;
    QSettings settings;
    settings.setValue(u"Speech/RecordingMode"_s,
                      (mode == static_cast<int>(RecordingMode::PushToTalk)) ? u"PushToTalk"_s : u"Toggle"_s);
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"setRecordingMode"_s, mode);
    }
    emit recordingModeChanged();
}

void DaemonClient::setAutoSwitchOffline(bool enable) {
    m_autoSwitchOffline = enable;
    QSettings settings;
    settings.setValue(u"Speech/AutoSwitchOffline"_s, enable);
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"setAutoSwitchOffline"_s, enable);
    }
    emit autoSwitchOfflineChanged();
}

void DaemonClient::setClientWindowFocused(bool focused) {
    if (m_clientWindowFocused != focused) {
        m_clientWindowFocused = focused;
        emit clientWindowFocusedChanged();
    }
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"setClientWindowFocused"_s, focused);
    }
}

void DaemonClient::startRecording() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"startRecording"_s);
    }
}

void DaemonClient::stopRecording() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"stopRecording"_s);
    }
}

void DaemonClient::toggleRecording() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"toggleRecording"_s);
    }
}

void DaemonClient::cancelDictation() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"cancelDictation"_s);
    }
}

void DaemonClient::retryTranscription() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"retryTranscription"_s);
    }
}

void DaemonClient::clearLastTranscription() {
    m_lastTranscription.clear();
    emit lastTranscriptionChanged();
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"clearLastTranscription"_s);
    }
}

void DaemonClient::clearError() {
    m_lastError.clear();
    emit lastErrorChanged();
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"clearError"_s);
    }
}

void DaemonClient::clearHistory() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"clearHistory"_s);
    }
}

void DaemonClient::reloadSettings() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"reloadSettings"_s);
    }
}

void DaemonClient::testDirectTyping() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"testDirectTyping"_s);
    }
}

void DaemonClient::restartDirectTyping() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"restartDirectTyping"_s);
    }
}

void DaemonClient::stopDirectTyping() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"stopDirectTyping"_s);
    }
}

void DaemonClient::showWindow() {
    emit requestShowWindow();
}

void DaemonClient::quitApp() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"quitApp"_s);
    }
    emit requestQuitApp();
}

void DaemonClient::navigateToSection(const QString& section) {
    emit requestNavigate(section);
}

void DaemonClient::loadModel(const QString& customPath) {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"loadModel"_s, customPath);
    }
}

void DaemonClient::unloadModel() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"unloadModel"_s);
    }
}

void DaemonClient::checkModelStatus() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"checkModelStatus"_s);
    }
}

void DaemonClient::onServiceRegistered(const QString& serviceName) {
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

void DaemonClient::onServiceUnregistered(const QString& serviceName) {
    if (serviceName != kServiceName) {
        return;
    }

    m_connected = false;
    m_canRecord = false;
    m_audioLevel = 0.0;
    m_isModelLoaded = false;
    m_isModelBusy = false;
    m_statusMessage = tr("Background service offline");
    m_isVulkanSupported = false;
    emit connectedChanged();
    emit canRecordChanged();
    emit statusMessageChanged();
    emit shortcutStatusChanged();
    emit directTypingStatusChanged();
    emit audioLevelChanged(0.0);
    emit modelStatusChanged();
    emit vulkanSupportedChanged();
}

void DaemonClient::setupDBusConnections() {
    auto bus = QDBusConnection::sessionBus();

    bus.connect(kServiceName, kObjectPath, kInterface, u"recordingChanged"_s, this, SIGNAL(recordingChanged()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"canRecordChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"dictationStateChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"activeBackendChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"recordingModeChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"autoSwitchOfflineChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"statusMessageChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"lastErrorChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"lastTranscriptionChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"shortcutStatusChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"directTypingStatusChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"modelStatusChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"computeDeviceChanged"_s, this, SLOT(syncFromDaemon()));

    bus.connect(kServiceName, kObjectPath, kInterface, u"audioLevelChanged"_s, this, SLOT(onAudioLevelChanged(double)));
    bus.connect(kServiceName, kObjectPath, kInterface, u"transcriptionFinished"_s, this,
                SLOT(onTranscriptionFinished(QString)));
    bus.connect(kServiceName, kObjectPath, kInterface, u"historyCleared"_s, this, SIGNAL(historyCleared()));

    bus.connect(kServiceName, kObjectPath, kInterface, u"requestShowWindow"_s, this, SIGNAL(requestShowWindow()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"requestNavigate"_s, this, SIGNAL(requestNavigate(QString)));
    bus.connect(kServiceName, kObjectPath, kInterface, u"requestQuitApp"_s, this, SIGNAL(requestQuitApp()));
}

void DaemonClient::onAudioLevelChanged(double level) {
    m_audioLevel = level;
    emit audioLevelChanged(level);
}

void DaemonClient::onTranscriptionFinished(const QString& text) {
    m_lastTranscription = text;
    emit lastTranscriptionChanged();
    emit transcriptionFinished(text);
}

/**
 * @brief Synchronizes all local client properties with the daemon's current D-Bus state.
 *
 * Queries all exported properties (dictation state, audio levels, active backend,
 * model statuses, and configuration parameters) and emits corresponding Qt Quick
 * change notifications when differences are detected.
 */
void DaemonClient::syncFromDaemon() {
    if (!m_interface || !m_interface->isValid()) {
        return;
    }

    const QVariant stateVar = m_interface->property("dictationState");
    if (stateVar.isValid()) {
        const int newState = stateVar.toInt();
        if (m_dictationState != newState) {
            m_dictationState = newState;
            emit dictationStateChanged();
            emit recordingChanged();
            emit transcribingChanged();
            emit enhancingChanged();
        }
    }

    const QVariant canRecordVar = m_interface->property("canRecord");
    if (canRecordVar.isValid()) {
        const bool newCanRecord = canRecordVar.toBool();
        if (m_canRecord != newCanRecord) {
            m_canRecord = newCanRecord;
            emit canRecordChanged();
        }
    }

    const QVariant backendVar = m_interface->property("activeBackend");
    if (backendVar.isValid() && m_activeBackend != backendVar.toInt()) {
        m_activeBackend = backendVar.toInt();
        emit activeBackendChanged();
    }

    const QVariant modeVar = m_interface->property("recordingMode");
    if (modeVar.isValid() && m_recordingMode != modeVar.toInt()) {
        m_recordingMode = modeVar.toInt();
        emit recordingModeChanged();
    }

    const QVariant autoSwitchVar = m_interface->property("autoSwitchOffline");
    if (autoSwitchVar.isValid() && m_autoSwitchOffline != autoSwitchVar.toBool()) {
        m_autoSwitchOffline = autoSwitchVar.toBool();
        emit autoSwitchOfflineChanged();
    }

    const QVariant statusVar = m_interface->property("statusMessage");
    if (statusVar.isValid()) {
        m_statusMessage = statusVar.toString();
        emit statusMessageChanged();
    }

    const QVariant errVar = m_interface->property("lastError");
    if (errVar.isValid()) {
        m_lastError = errVar.toString();
        emit lastErrorChanged();
    }

    const QVariant transVar = m_interface->property("lastTranscription");
    if (transVar.isValid()) {
        m_lastTranscription = transVar.toString();
        emit lastTranscriptionChanged();
    }

    const QVariant pttVar = m_interface->property("pushToTalkSupported");
    const QVariant availVar = m_interface->property("shortcutAvailable");
    const QVariant suppVar = m_interface->property("shortcutSupported");
    const QVariant scStatusVar = m_interface->property("shortcutStatusMessage");
    if (pttVar.isValid()) {
        m_pushToTalkSupported = pttVar.toBool();
    }
    if (availVar.isValid()) {
        m_shortcutAvailable = availVar.toBool();
    }
    if (suppVar.isValid()) {
        m_shortcutSupported = suppVar.toBool();
    }
    if (scStatusVar.isValid()) {
        m_shortcutStatusMessage = scStatusVar.toString();
    }
    emit shortcutStatusChanged();

    const QVariant dtConnVar = m_interface->property("directTypingConnected");
    const QVariant dtFatalVar = m_interface->property("directTypingHasFatalError");
    const QVariant dtFatalMsgVar = m_interface->property("directTypingFatalErrorMessage");
    const QVariant dtStatusVar = m_interface->property("directTypingStatus");
    if (dtConnVar.isValid()) {
        m_directTypingConnected = dtConnVar.toBool();
    }
    if (dtFatalVar.isValid()) {
        m_directTypingHasFatalError = dtFatalVar.toBool();
    }
    if (dtFatalMsgVar.isValid()) {
        m_directTypingFatalErrorMessage = dtFatalMsgVar.toString();
    }
    if (dtStatusVar.isValid()) {
        m_directTypingStatus = dtStatusVar.toString();
    }
    emit directTypingStatusChanged();

    const QVariant modelLoadedVar = m_interface->property("isModelLoaded");
    const QVariant modelBusyVar = m_interface->property("isModelBusy");
    const QVariant loadedPathVar = m_interface->property("loadedModelPath");
    const QVariant modelPathVar = m_interface->property("modelPath");
    const QVariant modelFileNameVar = m_interface->property("modelFileName");
    const QVariant modelInstVar = m_interface->property("isModelInstalled");
    const QVariant compDevVar = m_interface->property("computeDevice");
    const QVariant vulkanVar = m_interface->property("isVulkanSupported");

    bool modelChanged = false;
    if (modelLoadedVar.isValid() && m_isModelLoaded != modelLoadedVar.toBool()) {
        m_isModelLoaded = modelLoadedVar.toBool();
        modelChanged = true;
    }
    if (modelBusyVar.isValid() && m_isModelBusy != modelBusyVar.toBool()) {
        m_isModelBusy = modelBusyVar.toBool();
        modelChanged = true;
    }
    if (loadedPathVar.isValid() && m_loadedModelPath != loadedPathVar.toString()) {
        m_loadedModelPath = loadedPathVar.toString();
        modelChanged = true;
    }
    if (modelPathVar.isValid() && m_modelPath != modelPathVar.toString()) {
        m_modelPath = modelPathVar.toString();
        modelChanged = true;
    }
    if (modelFileNameVar.isValid() && m_modelFileName != modelFileNameVar.toString()) {
        m_modelFileName = modelFileNameVar.toString();
        modelChanged = true;
    }
    if (modelInstVar.isValid() && m_isModelInstalled != modelInstVar.toBool()) {
        m_isModelInstalled = modelInstVar.toBool();
        modelChanged = true;
    }
    if (vulkanVar.isValid() && m_isVulkanSupported != vulkanVar.toBool()) {
        m_isVulkanSupported = vulkanVar.toBool();
        emit vulkanSupportedChanged();
    }
    if (compDevVar.isValid() && m_computeDevice != compDevVar.toString()) {
        m_computeDevice = compDevVar.toString();
        emit computeDeviceChanged();
    }
    if (modelChanged) {
        emit modelStatusChanged();
    }
}
