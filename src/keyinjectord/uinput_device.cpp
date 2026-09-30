#include "uinput_device.h"

#include "logging.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>

#include <linux/input-event-codes.h>
#include <unistd.h>

namespace keyinjectord {

using namespace std::chrono_literals;

UinputDevice::UinputDevice(int uinputFd)
    : m_fd(uinputFd) {
    m_dev = libevdev_new();
    if (!m_dev) {
        throw std::runtime_error("libevdev_new() failed");
    }

    libevdev_set_name(m_dev, "qtscribe-virtual-kbd");
    libevdev_set_id_bustype(m_dev, BUS_USB);
    libevdev_set_id_vendor(m_dev, 0x1234);
    libevdev_set_id_product(m_dev, 0x5678);
    libevdev_set_id_version(m_dev, 1);

    if (libevdev_enable_event_type(m_dev, EV_KEY) != 0) {
        throw std::runtime_error("Failed to enable EV_KEY");
    }
    if (libevdev_enable_event_type(m_dev, EV_SYN) != 0) {
        throw std::runtime_error("Failed to enable EV_SYN");
    }

    libevdev_enable_event_code(m_dev, EV_KEY, KEY_LEFTCTRL, nullptr);
    libevdev_enable_event_code(m_dev, EV_KEY, KEY_RIGHTCTRL, nullptr);
    libevdev_enable_event_code(m_dev, EV_KEY, KEY_LEFTSHIFT, nullptr);
    libevdev_enable_event_code(m_dev, EV_KEY, KEY_RIGHTSHIFT, nullptr);
    libevdev_enable_event_code(m_dev, EV_KEY, KEY_V, nullptr);

    int err = libevdev_uinput_create_from_device(m_dev, m_fd, &m_uidev);
    if (err != 0) {
        throw std::runtime_error(std::string("libevdev_uinput_create_from_device failed: ") + std::strerror(-err));
    }

    KEYINJECTORD_LOG_INFO("Virtual keyboard created: %s", libevdev_uinput_get_devnode(m_uidev));
    KEYINJECTORD_LOG_DEBUG("Waiting 200ms for udev registration...");
    std::this_thread::sleep_for(200ms);
    KEYINJECTORD_LOG_DEBUG("Virtual uinput device ready");
}

UinputDevice::~UinputDevice() {
    if (m_uidev) {
        libevdev_uinput_destroy(m_uidev);
        KEYINJECTORD_LOG_INFO("Virtual keyboard destroyed");
    }
    if (m_dev) {
        libevdev_free(m_dev);
    }
    if (m_fd >= 0) {
        close(m_fd);
    }
}

bool UinputDevice::emitEvent(int type, int code, int value) {
    if (!m_uidev) {
        KEYINJECTORD_LOG_ERROR("emitEvent() — device not initialized");
        return false;
    }

    int err = libevdev_uinput_write_event(m_uidev, type, code, value);
    if (err != 0) {
        KEYINJECTORD_LOG_ERROR("write_event(type=%d, code=%d, val=%d) failed: %s", type, code, value,
                               std::strerror(-err));
        return false;
    }

    KEYINJECTORD_LOG_DEBUG("emitEvent(type=%d, code=%d, val=%d) success", type, code, value);
    return true;
}

} // namespace keyinjectord
