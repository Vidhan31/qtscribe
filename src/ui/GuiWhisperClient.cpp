#include "GuiWhisperClient.h"

#include "WhisperModelManager.h"

#include <QCoreApplication>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

static GuiWhisperClient* s_instance = nullptr;

GuiWhisperClient::GuiWhisperClient(QObject* parent)
    : QObject(parent) {
    s_instance = this;
    if (auto* client = DaemonClient::instance()) {
        connect(client, &DaemonClient::modelStatusChanged, this, &GuiWhisperClient::modelStatusChanged);
        connect(client, &DaemonClient::modelStatusChanged, this, &GuiWhisperClient::busyChanged);
        connect(client, &DaemonClient::dictationStateChanged, this, &GuiWhisperClient::busyChanged);
        connect(client, &DaemonClient::computeDeviceChanged, this, &GuiWhisperClient::computeDeviceChanged);
        connect(client, &DaemonClient::vulkanSupportedChanged, this, &GuiWhisperClient::vulkanSupportedChanged);
    }
}

GuiWhisperClient* GuiWhisperClient::instance() {
    if (!s_instance) {
        s_instance = new GuiWhisperClient(QCoreApplication::instance());
    }
    return s_instance;
}

GuiWhisperClient* GuiWhisperClient::create(QQmlEngine* /*qmlEngine*/, QJSEngine* /*jsEngine*/) {
    return instance();
}

bool GuiWhisperClient::isModelInstalled() const {
    if (auto* client = DaemonClient::instance()) {
        if (client->isConnected()) {
            return client->isModelInstalled();
        }
    }
    if (auto* mgr = WhisperModelManager::instance()) {
        return mgr->isSelectedModelInstalled();
    }
    const QString path = modelPath();
    return !path.isEmpty() && QFile::exists(path);
}

bool GuiWhisperClient::isModelLoaded() const {
    if (auto* client = DaemonClient::instance()) {
        return client->isModelLoaded();
    }
    return false;
}

QString GuiWhisperClient::modelPath() const {
    if (auto* client = DaemonClient::instance()) {
        if (client->isConnected() && !client->modelPath().isEmpty()) {
            return client->modelPath();
        }
    }
    if (auto* mgr = WhisperModelManager::instance()) {
        return mgr->selectedModelPath();
    }
    return modelsDirectory() + u"/"_s + modelFileName();
}

QString GuiWhisperClient::loadedModelPath() const {
    if (auto* client = DaemonClient::instance()) {
        return client->loadedModelPath();
    }
    return QString();
}

QString GuiWhisperClient::modelsDirectory() const {
    if (auto* mgr = WhisperModelManager::instance()) {
        return mgr->modelsDirectory();
    }
    const QString genericData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return genericData + u"/qtscribe/models"_s;
}

QString GuiWhisperClient::modelFileName() const {
    if (auto* client = DaemonClient::instance()) {
        if (client->isConnected() && !client->modelFileName().isEmpty()) {
            return client->modelFileName();
        }
    }
    if (auto* mgr = WhisperModelManager::instance()) {
        const auto item = mgr->model(mgr->selectedModelId());
        if (item.has_value() && !item->fileName.isEmpty()) {
            return item->fileName;
        }
    }
    QSettings settings;
    const QString id = settings.value(u"Whisper/SelectedModel"_s, u"tiny.en"_s).toString();
    return QString(u"ggml-%1.bin"_s).arg(id);
}

QString GuiWhisperClient::computeDevice() const {
    if (auto* client = DaemonClient::instance()) {
        if (client->isConnected() && !client->computeDevice().isEmpty()) {
            return client->computeDevice();
        }
    }
#if defined(GGML_USE_VULKAN)
    return u"Vulkan (GPU)"_s;
#else
    return u"CPU (SIMD)"_s;
#endif
}

bool GuiWhisperClient::isVulkanSupported() const {
#if defined(GGML_USE_VULKAN)
    return true;
#else
    if (auto* client = DaemonClient::instance()) {
        return client->isVulkanSupported();
    }
    return false;
#endif
}

bool GuiWhisperClient::isBusy() const {
    if (auto* client = DaemonClient::instance()) {
        return client->isModelBusy() || client->isBusy();
    }
    return false;
}

void GuiWhisperClient::loadModel(const QString& customPath) {
    if (isBusy()) {
        return;
    }
    if (auto* client = DaemonClient::instance()) {
        client->loadModel(customPath);
    }
}

void GuiWhisperClient::unloadModel() {
    if (isBusy()) {
        return;
    }
    if (auto* client = DaemonClient::instance()) {
        client->unloadModel();
    }
}

void GuiWhisperClient::checkModelStatus() {
    if (auto* client = DaemonClient::instance()) {
        client->checkModelStatus();
    }
}
