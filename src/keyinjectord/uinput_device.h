#pragma once
#include "device_interface.h"

#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>

namespace keyinjectord {

class UinputDevice : public IRawDevice {
public:
    explicit UinputDevice(int uinputFd);
    ~UinputDevice() override;

    UinputDevice(const UinputDevice&) = delete;
    UinputDevice& operator=(const UinputDevice&) = delete;

    bool emitEvent(int type, int code, int value) override;

    bool isValid() const { return m_uidev != nullptr; }

private:
    struct libevdev* m_dev = nullptr;
    struct libevdev_uinput* m_uidev = nullptr;
    int m_fd = -1;
};

} // namespace keyinjectord
