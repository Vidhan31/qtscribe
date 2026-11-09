#include "DaemonClient.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

using namespace Qt::StringLiterals;

static const auto kServiceName = u"io.github.qtscribe.SpeechService"_s;

DaemonClient::DaemonClient(QObject* parent)
    : QObject(parent) {
    s_instance = this;

    m_speechClient = SpeechClient::instance();
    m_modelClient = ModelClient::instance();
    m_systemClient = SystemClient::instance();
    m_settingsClient = SettingsClient::instance();

    if (m_speechClient) {
        connect(m_speechClient, &SpeechClient::connectedChanged, this, &DaemonClient::connectedChanged);
        connect(m_speechClient, &SpeechClient::activeBackendChanged, this, &DaemonClient::activeBackendChanged);
        connect(m_speechClient, &SpeechClient::recordingModeChanged, this, &DaemonClient::recordingModeChanged);
        connect(m_speechClient, &SpeechClient::dictationStateChanged, this, &DaemonClient::dictationStateChanged);
        connect(m_speechClient, &SpeechClient::dictationStateChanged, this, &DaemonClient::modelStatusChanged);
        connect(m_speechClient, &SpeechClient::canRecordChanged, this, &DaemonClient::canRecordChanged);
        connect(m_speechClient, &SpeechClient::audioLevelChanged, this, &DaemonClient::audioLevelChanged);
        connect(m_speechClient, &SpeechClient::statusMessageChanged, this, &DaemonClient::statusMessageChanged);
        connect(m_speechClient, &SpeechClient::lastErrorChanged, this, &DaemonClient::lastErrorChanged);
        connect(m_speechClient, &SpeechClient::recordingChanged, this, &DaemonClient::recordingChanged);
        connect(m_speechClient, &SpeechClient::transcribingChanged, this, &DaemonClient::transcribingChanged);
        connect(m_speechClient, &SpeechClient::enhancingChanged, this, &DaemonClient::enhancingChanged);
        connect(m_speechClient, &SpeechClient::lastTranscriptionChanged, this, &DaemonClient::lastTranscriptionChanged);
        connect(m_speechClient, &SpeechClient::autoSwitchOfflineChanged, this, &DaemonClient::autoSwitchOfflineChanged);
        connect(m_speechClient, &SpeechClient::clientWindowFocusedChanged, this,
                &DaemonClient::clientWindowFocusedChanged);
        connect(m_speechClient, &SpeechClient::cloudStatusChanged, this, &DaemonClient::cloudStatusChanged);
        connect(m_speechClient, &SpeechClient::transcriptionFinished, this, &DaemonClient::transcriptionFinished);
        connect(m_speechClient, &SpeechClient::historyCleared, this, &DaemonClient::historyCleared);
        connect(m_speechClient, &SpeechClient::historyChanged, this, &DaemonClient::historyChanged);
        connect(m_speechClient, &SpeechClient::errorOccurred, this, &DaemonClient::errorOccurred);
    }

    if (m_modelClient) {
        connect(m_modelClient, &ModelClient::modelStatusChanged, this, &DaemonClient::modelStatusChanged);
        connect(m_modelClient, &ModelClient::computeDeviceChanged, this, &DaemonClient::computeDeviceChanged);
        connect(m_modelClient, &ModelClient::vulkanSupportedChanged, this, &DaemonClient::vulkanSupportedChanged);
        connect(m_modelClient, &ModelClient::modelCatalogChanged, this, &DaemonClient::modelCatalogChanged);
    }

    if (m_systemClient) {
        connect(m_systemClient, &SystemClient::shortcutStatusChanged, this, &DaemonClient::shortcutStatusChanged);
        connect(m_systemClient, &SystemClient::textInjectionStatusChanged, this,
                &DaemonClient::textInjectionStatusChanged);
        connect(m_systemClient, &SystemClient::directTypingStatusChanged, this,
                &DaemonClient::directTypingStatusChanged);
        connect(m_systemClient, &SystemClient::microphoneStatusChanged, this, &DaemonClient::microphoneStatusChanged);
        connect(m_systemClient, &SystemClient::microphoneTestChanged, this, &DaemonClient::microphoneTestChanged);
        connect(m_systemClient, &SystemClient::microphoneTestLevelChanged, this,
                &DaemonClient::microphoneTestLevelChanged);
        connect(m_systemClient, &SystemClient::requestShowWindow, this, &DaemonClient::requestShowWindow);
        connect(m_systemClient, &SystemClient::requestNavigate, this, &DaemonClient::requestNavigate);
        connect(m_systemClient, &SystemClient::requestQuitApp, this, &DaemonClient::requestQuitApp);
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
    return m_speechClient && m_speechClient->isConnected();
}

DaemonClient::TranscriptionBackend DaemonClient::activeBackend() const noexcept {
    return m_speechClient ? static_cast<TranscriptionBackend>(m_speechClient->activeBackend())
                          : TranscriptionBackend::Cloud;
}

DaemonClient::RecordingMode DaemonClient::recordingMode() const noexcept {
    return m_speechClient ? static_cast<RecordingMode>(m_speechClient->recordingMode()) : RecordingMode::Toggle;
}

DaemonClient::DictationState DaemonClient::dictationState() const noexcept {
    return m_speechClient ? static_cast<DictationState>(m_speechClient->dictationState()) : DictationState::Idle;
}

bool DaemonClient::isBusy() const noexcept {
    return m_speechClient && m_speechClient->isBusy();
}

bool DaemonClient::canRecord() const noexcept {
    return m_speechClient && m_speechClient->canRecord();
}

qreal DaemonClient::audioLevel() const noexcept {
    return m_speechClient ? m_speechClient->audioLevel() : 0.0;
}

QString DaemonClient::statusMessage() const {
    return m_speechClient ? m_speechClient->statusMessage() : QString();
}

QString DaemonClient::lastError() const {
    return m_speechClient ? m_speechClient->lastError() : QString();
}

bool DaemonClient::recording() const noexcept {
    return m_speechClient && m_speechClient->recording();
}

bool DaemonClient::transcribing() const noexcept {
    return m_speechClient && m_speechClient->transcribing();
}

bool DaemonClient::enhancing() const noexcept {
    return m_speechClient && m_speechClient->enhancing();
}

QString DaemonClient::lastTranscription() const {
    return m_speechClient ? m_speechClient->lastTranscription() : QString();
}

bool DaemonClient::autoSwitchOffline() const noexcept {
    return m_speechClient && m_speechClient->autoSwitchOffline();
}

bool DaemonClient::clientWindowFocused() const noexcept {
    return m_speechClient && m_speechClient->clientWindowFocused();
}

bool DaemonClient::pushToTalkSupported() const noexcept {
    return m_systemClient && m_systemClient->pushToTalkSupported();
}

bool DaemonClient::shortcutAvailable() const noexcept {
    return m_systemClient && m_systemClient->shortcutAvailable();
}

bool DaemonClient::shortcutSupported() const noexcept {
    return m_systemClient && m_systemClient->shortcutSupported();
}

QString DaemonClient::shortcutStatusMessage() const {
    return m_systemClient ? m_systemClient->shortcutStatusMessage() : QString();
}

bool DaemonClient::systemShortcutHasIssue() const noexcept {
    return m_systemClient && m_systemClient->systemShortcutHasIssue();
}

bool DaemonClient::textInjectionConnected() const noexcept {
    return m_systemClient && m_systemClient->textInjectionConnected();
}

bool DaemonClient::directTypingConnected() const noexcept {
    return textInjectionConnected();
}

bool DaemonClient::textInjectionHasFatalError() const noexcept {
    return m_systemClient && m_systemClient->textInjectionHasFatalError();
}

bool DaemonClient::directTypingHasFatalError() const noexcept {
    return textInjectionHasFatalError();
}

QString DaemonClient::textInjectionFatalErrorMessage() const {
    return m_systemClient ? m_systemClient->textInjectionFatalErrorMessage() : QString();
}

QString DaemonClient::directTypingFatalErrorMessage() const {
    return textInjectionFatalErrorMessage();
}

QString DaemonClient::textInjectionStatus() const {
    return m_systemClient ? m_systemClient->textInjectionStatus() : QString();
}

QString DaemonClient::directTypingStatus() const {
    return textInjectionStatus();
}

bool DaemonClient::textInjectionHasIssue() const noexcept {
    return m_systemClient && m_systemClient->textInjectionHasIssue();
}

bool DaemonClient::directTypingHasIssue() const noexcept {
    return textInjectionHasIssue();
}

bool DaemonClient::isModelLoaded() const noexcept {
    return m_modelClient && m_modelClient->isModelLoaded();
}

bool DaemonClient::isModelBusy() const noexcept {
    return isBusy() || (m_modelClient && m_modelClient->isModelBusy());
}

QString DaemonClient::loadedModelPath() const {
    return m_modelClient ? m_modelClient->loadedModelPath() : QString();
}

QString DaemonClient::modelPath() const {
    return m_modelClient ? m_modelClient->modelPath() : QString();
}

QString DaemonClient::modelFileName() const {
    return m_modelClient ? m_modelClient->modelFileName() : QString();
}

QString DaemonClient::computeDevice() const {
    return m_modelClient ? m_modelClient->computeDevice() : QString();
}

bool DaemonClient::isVulkanSupported() const noexcept {
    return m_modelClient && m_modelClient->isVulkanSupported();
}

bool DaemonClient::isModelInstalled() const noexcept {
    return m_modelClient && m_modelClient->isModelInstalled();
}

bool DaemonClient::microphoneAvailable() const noexcept {
    return m_systemClient && m_systemClient->microphoneAvailable();
}

bool DaemonClient::microphoneTestActive() const noexcept {
    return m_systemClient && m_systemClient->microphoneTestActive();
}

qreal DaemonClient::microphoneTestLevel() const noexcept {
    return m_systemClient ? m_systemClient->microphoneTestLevel() : 0.0;
}

bool DaemonClient::cloudApiKeySet() const noexcept {
    return m_speechClient && m_speechClient->cloudApiKeySet();
}

bool DaemonClient::cloudApiKeyInvalid() const noexcept {
    return m_speechClient && m_speechClient->cloudApiKeyInvalid();
}

bool DaemonClient::cloudRateLimited() const noexcept {
    return m_speechClient && m_speechClient->cloudRateLimited();
}

int DaemonClient::cloudRetrySecondsRemaining() const noexcept {
    return m_speechClient ? m_speechClient->cloudRetrySecondsRemaining() : 0;
}

QString DaemonClient::cloudLastError() const {
    return m_speechClient ? m_speechClient->cloudLastError() : QString();
}

QString DaemonClient::modelCatalogJson() const {
    return m_modelClient ? m_modelClient->modelCatalogJson() : QStringLiteral("[]");
}

QString DaemonClient::modelsDirectory() const {
    return m_modelClient ? m_modelClient->modelsDirectory() : QString();
}

bool DaemonClient::modelDownloadingAny() const noexcept {
    return m_modelClient && m_modelClient->modelDownloadingAny();
}

QString DaemonClient::modelDownloadBytes() const {
    return m_modelClient ? m_modelClient->modelDownloadBytes() : QString();
}

QString DaemonClient::modelCatalogLastError() const {
    return m_modelClient ? m_modelClient->modelCatalogLastError() : QString();
}

QString DaemonClient::modelDiskSpace() const {
    return m_modelClient ? m_modelClient->modelDiskSpace() : QString();
}

QString DaemonClient::historyJson() const {
    return m_speechClient ? m_speechClient->historyJson() : QStringLiteral("[]");
}

bool DaemonClient::isServiceRunning() {
    if (!QDBusConnection::sessionBus().isConnected()) {
        return false;
    }
    auto* iface = QDBusConnection::sessionBus().interface();
    return iface && iface->isServiceRegistered(kServiceName);
}

bool DaemonClient::sendRemoteCommand(const QString& command) {
    if (command == u"quitApp"_s || command == u"showWindow"_s || command == u"navigateToSection"_s ||
        command == u"testTextInjection"_s || command == u"restartTextInjection"_s ||
        command == u"stopTextInjection"_s || command == u"testDirectTyping"_s || command == u"restartDirectTyping"_s ||
        command == u"stopDirectTyping"_s) {
        return SystemClient::sendRemoteCommand(command);
    }
    return SpeechClient::sendRemoteCommand(command);
}

void DaemonClient::setActiveBackend(TranscriptionBackend backend, bool persistSettings) {
    setActiveBackend(static_cast<int>(backend), persistSettings);
}

void DaemonClient::setActiveBackend(int backend, bool persistSettings) {
    if (m_speechClient) {
        m_speechClient->setActiveBackend(backend, persistSettings);
    }
}

void DaemonClient::setRecordingMode(RecordingMode mode) {
    setRecordingMode(static_cast<int>(mode));
}

void DaemonClient::setRecordingMode(int mode) {
    if (m_speechClient) {
        m_speechClient->setRecordingMode(mode);
    }
}

void DaemonClient::setAutoSwitchOffline(bool enable) {
    if (m_speechClient) {
        m_speechClient->setAutoSwitchOffline(enable);
    }
}

void DaemonClient::setClientWindowFocused(bool focused) {
    if (m_speechClient) {
        m_speechClient->setClientWindowFocused(focused);
    }
}

void DaemonClient::toggleRecording() {
    if (m_speechClient) {
        m_speechClient->toggleRecording();
    }
}

void DaemonClient::startRecording() {
    if (m_speechClient) {
        m_speechClient->startRecording();
    }
}

void DaemonClient::stopRecording() {
    if (m_speechClient) {
        m_speechClient->stopRecording();
    }
}

void DaemonClient::cancelDictation() {
    if (m_speechClient) {
        m_speechClient->cancelDictation();
    }
}

void DaemonClient::retryTranscription() {
    if (m_speechClient) {
        m_speechClient->retryTranscription();
    }
}

void DaemonClient::clearLastTranscription() {
    if (m_speechClient) {
        m_speechClient->clearLastTranscription();
    }
}

void DaemonClient::clearError() {
    if (m_speechClient) {
        m_speechClient->clearError();
    }
}

void DaemonClient::clearHistory() {
    if (m_speechClient) {
        m_speechClient->clearHistory();
    }
}

void DaemonClient::reloadSettings() {
    if (m_speechClient) {
        m_speechClient->reloadSettings();
    }
}

void DaemonClient::testTextInjection() {
    if (m_systemClient) {
        m_systemClient->testTextInjection();
    }
}

void DaemonClient::testDirectTyping() {
    testTextInjection();
}

void DaemonClient::restartTextInjection() {
    if (m_systemClient) {
        m_systemClient->restartTextInjection();
    }
}

void DaemonClient::restartDirectTyping() {
    restartTextInjection();
}

void DaemonClient::stopTextInjection() {
    if (m_systemClient) {
        m_systemClient->stopTextInjection();
    }
}

void DaemonClient::stopDirectTyping() {
    stopTextInjection();
}

void DaemonClient::showWindow() {
    if (m_systemClient) {
        m_systemClient->showWindow();
    }
}

void DaemonClient::quitApp() {
    if (m_systemClient) {
        m_systemClient->quitApp();
    }
}

void DaemonClient::navigateToSection(const QString& section) {
    if (m_systemClient) {
        m_systemClient->navigateToSection(section);
    }
}

void DaemonClient::loadModel(const QString& customPath) {
    if (m_modelClient) {
        m_modelClient->loadModel(customPath);
    }
}

void DaemonClient::unloadModel() {
    if (m_modelClient) {
        m_modelClient->unloadModel();
    }
}

void DaemonClient::checkModelStatus() {
    if (m_modelClient) {
        m_modelClient->checkModelStatus();
    }
}

void DaemonClient::startMicrophoneTest() {
    if (m_systemClient) {
        m_systemClient->startMicrophoneTest();
    }
}

void DaemonClient::stopMicrophoneTest() {
    if (m_systemClient) {
        m_systemClient->stopMicrophoneTest();
    }
}

void DaemonClient::playStartSound() {
    if (m_systemClient) {
        m_systemClient->playStartSound();
    }
}

void DaemonClient::playStopSound() {
    if (m_systemClient) {
        m_systemClient->playStopSound();
    }
}

void DaemonClient::selectModel(const QString& modelId) {
    if (m_modelClient) {
        m_modelClient->selectModel(modelId);
    }
}

void DaemonClient::startModelDownload(const QString& modelId) {
    if (m_modelClient) {
        m_modelClient->startModelDownload(modelId);
    }
}

void DaemonClient::cancelModelDownload(const QString& modelId) {
    if (m_modelClient) {
        m_modelClient->cancelModelDownload(modelId);
    }
}

void DaemonClient::deleteModel(const QString& modelId) {
    if (m_modelClient) {
        m_modelClient->deleteModel(modelId);
    }
}

void DaemonClient::refreshModelCatalog() {
    if (m_modelClient) {
        m_modelClient->refreshModelCatalog();
    }
}

void DaemonClient::checkModelDiskSpace() {
    if (m_modelClient) {
        m_modelClient->checkModelDiskSpace();
    }
}

void DaemonClient::onServiceRegistered(const QString& serviceName) {
    if (m_speechClient) {
        QMetaObject::invokeMethod(m_speechClient, "onServiceRegistered", Q_ARG(QString, serviceName));
    }
    if (m_modelClient) {
        QMetaObject::invokeMethod(m_modelClient, "onServiceRegistered", Q_ARG(QString, serviceName));
    }
    if (m_systemClient) {
        QMetaObject::invokeMethod(m_systemClient, "onServiceRegistered", Q_ARG(QString, serviceName));
    }
}

void DaemonClient::onServiceUnregistered(const QString& serviceName) {
    if (m_speechClient) {
        QMetaObject::invokeMethod(m_speechClient, "onServiceUnregistered", Q_ARG(QString, serviceName));
    }
    if (m_modelClient) {
        QMetaObject::invokeMethod(m_modelClient, "onServiceUnregistered", Q_ARG(QString, serviceName));
    }
    if (m_systemClient) {
        QMetaObject::invokeMethod(m_systemClient, "onServiceUnregistered", Q_ARG(QString, serviceName));
    }
}
