#include "ModelClient.h"

#include "common/SettingsKeys.h"

#include <QCoreApplication>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

static const auto kServiceName = u"io.github.qtscribe.SpeechService"_s;
static const auto kObjectPath = u"/io/github/qtscribe/SpeechService"_s;
static const auto kInterface = u"io.github.qtscribe.ModelService"_s;

ModelClient::ModelClient(QObject* parent)
    : QObject(parent) {
    s_instance = this;

    m_computeDevice = u"CPU (SIMD)"_s;
    m_modelsDirectory = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/qtscribe/models"_s;
    const QSettings settings;
    const QString id = settings.value(SettingsKeys::kWhisperSelectedModel.toString(), u"tiny.en"_s).toString();
    m_modelFileName = QString(u"ggml-%1.bin"_s).arg(id);
    m_modelPath = m_modelsDirectory + u"/"_s + m_modelFileName;
    m_isModelInstalled = QFile::exists(m_modelPath);

    const bool disableRemoteAttach = !qEnvironmentVariableIsEmpty("QTSCRIBE_DISABLE_REMOTE_ATTACH") &&
                                     qEnvironmentVariable("QTSCRIBE_DISABLE_REMOTE_ATTACH") != "0";
    if (disableRemoteAttach) {
        return;
    }

    if (QDBusConnection::sessionBus().isConnected()) {
        m_watcher = new QDBusServiceWatcher(
            kServiceName, QDBusConnection::sessionBus(),
            QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration, this);
        connect(m_watcher, &QDBusServiceWatcher::serviceRegistered, this, &ModelClient::onServiceRegistered);
        connect(m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, &ModelClient::onServiceUnregistered);

        auto* iface = QDBusConnection::sessionBus().interface();
        if (iface && iface->isServiceRegistered(kServiceName)) {
            onServiceRegistered(kServiceName);
        }
    }
}

ModelClient::~ModelClient() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

ModelClient* ModelClient::instance() {
    if (!s_instance) {
        s_instance = new ModelClient(QCoreApplication::instance());
    }
    return s_instance;
}

ModelClient* ModelClient::create(QQmlEngine* /*qmlEngine*/, QJSEngine* /*jsEngine*/) {
    return instance();
}

bool ModelClient::isConnected() const noexcept {
    return m_connected;
}

bool ModelClient::isModelLoaded() const noexcept {
    return m_isModelLoaded;
}

bool ModelClient::isModelBusy() const noexcept {
    return m_isModelBusy;
}

QString ModelClient::loadedModelPath() const {
    return m_loadedModelPath;
}

QString ModelClient::modelPath() const {
    return m_modelPath;
}

QString ModelClient::modelFileName() const {
    return m_modelFileName;
}

QString ModelClient::computeDevice() const {
    return m_computeDevice;
}

bool ModelClient::isVulkanSupported() const noexcept {
    return m_isVulkanSupported;
}

bool ModelClient::isModelInstalled() const noexcept {
    if (m_isModelInstalled) {
        return true;
    }
    const QString path = modelPath();
    return !path.isEmpty() && QFile::exists(path);
}

QString ModelClient::modelCatalogJson() const {
    return m_modelCatalogJson;
}

QString ModelClient::modelsDirectory() const {
    return m_modelsDirectory;
}

bool ModelClient::modelDownloadingAny() const noexcept {
    return m_modelDownloadingAny;
}

QString ModelClient::modelDownloadBytes() const {
    return m_modelDownloadBytes;
}

QString ModelClient::modelCatalogLastError() const {
    return m_modelCatalogLastError;
}

QString ModelClient::modelDiskSpace() const {
    return m_modelDiskSpace;
}

void ModelClient::loadModel(const QString& customPath) {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"loadModel"_s, customPath);
    }
}

void ModelClient::unloadModel() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"unloadModel"_s);
    }
}

void ModelClient::checkModelStatus() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"checkModelStatus"_s);
    }
}

void ModelClient::selectModel(const QString& modelId) {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"selectModel"_s, modelId);
    }
}

void ModelClient::startModelDownload(const QString& modelId) {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"startModelDownload"_s, modelId);
    }
}

void ModelClient::cancelModelDownload(const QString& modelId) {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"cancelModelDownload"_s, modelId);
    }
}

void ModelClient::deleteModel(const QString& modelId) {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"deleteModel"_s, modelId);
    }
}

void ModelClient::refreshModelCatalog() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"refreshModelCatalog"_s);
    }
}

void ModelClient::checkModelDiskSpace() {
    if (m_interface && m_interface->isValid()) {
        m_interface->call(QDBus::NoBlock, u"checkModelDiskSpace"_s);
    }
}

void ModelClient::onServiceRegistered(const QString& serviceName) {
    if (serviceName != kServiceName) {
        return;
    }

    if (!m_interface) {
        m_interface = new QDBusInterface(kServiceName, kObjectPath, kInterface, QDBusConnection::sessionBus(), this);
    }

    setupDBusConnections();
    m_connected = true;
    emit connectedChanged();
    syncFromDaemon();
    syncModelCatalog();
}

void ModelClient::onServiceUnregistered(const QString& serviceName) {
    if (serviceName != kServiceName) {
        return;
    }

    m_connected = false;
    m_isModelLoaded = false;
    m_isModelBusy = false;
    m_isVulkanSupported = false;
    m_modelDownloadingAny = false;

    emit connectedChanged();
    emit modelStatusChanged();
    emit vulkanSupportedChanged();
    emit modelCatalogChanged();
}

void ModelClient::setupDBusConnections() {
    if (!QDBusConnection::sessionBus().isConnected()) {
        return;
    }

    auto bus = QDBusConnection::sessionBus();

    bus.connect(kServiceName, kObjectPath, u"org.freedesktop.DBus.Properties"_s, u"PropertiesChanged"_s, this,
                SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));

    bus.connect(kServiceName, kObjectPath, kInterface, u"modelStatusChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"computeDeviceChanged"_s, this, SLOT(syncFromDaemon()));
    bus.connect(kServiceName, kObjectPath, kInterface, u"modelCatalogChanged"_s, this, SLOT(syncModelCatalog()));
}

void ModelClient::syncFromDaemon() {
    if (!m_interface || !m_interface->isValid()) {
        return;
    }

    m_isModelLoaded = m_interface->property("isModelLoaded").toBool();
    m_isModelBusy = m_interface->property("isModelBusy").toBool();
    m_loadedModelPath = m_interface->property("loadedModelPath").toString();
    m_modelPath = m_interface->property("modelPath").toString();
    m_modelFileName = m_interface->property("modelFileName").toString();
    m_computeDevice = m_interface->property("computeDevice").toString();
    m_isVulkanSupported = m_interface->property("isVulkanSupported").toBool();
    m_isModelInstalled = m_interface->property("isModelInstalled").toBool();

    emit modelStatusChanged();
    emit computeDeviceChanged();
    emit vulkanSupportedChanged();
}

void ModelClient::syncModelCatalog() {
    if (!m_interface || !m_interface->isValid()) {
        return;
    }

    m_modelCatalogJson = m_interface->property("modelCatalogJson").toString();
    m_modelsDirectory = m_interface->property("modelsDirectory").toString();
    m_modelDownloadingAny = m_interface->property("modelDownloadingAny").toBool();
    m_modelDownloadBytes = m_interface->property("modelDownloadBytes").toString();
    m_modelCatalogLastError = m_interface->property("modelCatalogLastError").toString();
    m_modelDiskSpace = m_interface->property("modelDiskSpace").toString();

    emit modelCatalogChanged();
}

void ModelClient::onPropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties,
                                      const QStringList& invalidatedProperties) {
    Q_UNUSED(invalidatedProperties);
    if (interfaceName != kInterface) {
        return;
    }

    bool statusChanged = false;
    bool catalogChanged = false;

    for (auto it = changedProperties.constBegin(); it != changedProperties.constEnd(); ++it) {
        const QString& key = it.key();
        const QVariant& val = it.value();

        if (key == u"isModelLoaded"_s) {
            m_isModelLoaded = val.toBool();
            statusChanged = true;
        } else if (key == u"isModelBusy"_s) {
            m_isModelBusy = val.toBool();
            statusChanged = true;
        } else if (key == u"loadedModelPath"_s) {
            m_loadedModelPath = val.toString();
            statusChanged = true;
        } else if (key == u"modelPath"_s) {
            m_modelPath = val.toString();
            statusChanged = true;
        } else if (key == u"modelFileName"_s) {
            m_modelFileName = val.toString();
            statusChanged = true;
        } else if (key == u"computeDevice"_s) {
            m_computeDevice = val.toString();
            emit computeDeviceChanged();
        } else if (key == u"isVulkanSupported"_s) {
            m_isVulkanSupported = val.toBool();
            emit vulkanSupportedChanged();
        } else if (key == u"isModelInstalled"_s) {
            m_isModelInstalled = val.toBool();
            statusChanged = true;
        } else if (key == u"modelCatalogJson"_s) {
            m_modelCatalogJson = val.toString();
            catalogChanged = true;
        } else if (key == u"modelsDirectory"_s) {
            m_modelsDirectory = val.toString();
            catalogChanged = true;
        } else if (key == u"modelDownloadingAny"_s) {
            m_modelDownloadingAny = val.toBool();
            catalogChanged = true;
        } else if (key == u"modelDownloadBytes"_s) {
            m_modelDownloadBytes = val.toString();
            catalogChanged = true;
        } else if (key == u"modelCatalogLastError"_s) {
            m_modelCatalogLastError = val.toString();
            catalogChanged = true;
        } else if (key == u"modelDiskSpace"_s) {
            m_modelDiskSpace = val.toString();
            catalogChanged = true;
        }
    }

    if (statusChanged) {
        emit modelStatusChanged();
    }
    if (catalogChanged) {
        emit modelCatalogChanged();
    }
}
