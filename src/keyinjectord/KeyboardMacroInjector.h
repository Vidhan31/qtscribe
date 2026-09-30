#pragma once

#include "device_interface.h"

#include <chrono>

namespace keyinjectord {

class KeyboardMacroInjector : public IDevice {
public:
    explicit KeyboardMacroInjector(IRawDevice& device, std::chrono::milliseconds delay = std::chrono::milliseconds(18));
    ~KeyboardMacroInjector() override = default;

    KeyboardMacroInjector(const KeyboardMacroInjector&) = delete;
    KeyboardMacroInjector& operator=(const KeyboardMacroInjector&) = delete;

    bool sendCtrlShiftV() override;

private:
    IRawDevice& m_device;
    std::chrono::milliseconds m_delay;
};

} // namespace keyinjectord
