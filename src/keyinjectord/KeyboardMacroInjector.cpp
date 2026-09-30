#include "KeyboardMacroInjector.h"

#include "logging.h"

#include <thread>

#include <linux/input-event-codes.h>

namespace keyinjectord {

KeyboardMacroInjector::KeyboardMacroInjector(IRawDevice& device, std::chrono::milliseconds delay)
    : m_device(device)
    , m_delay(delay) { }

bool KeyboardMacroInjector::sendCtrlShiftV() {
    bool ok = true;
    ok = ok && m_device.emitEvent(EV_KEY, KEY_LEFTCTRL, 1);
    ok = ok && m_device.emitEvent(EV_SYN, SYN_REPORT, 0);
    if (m_delay.count() > 0) {
        std::this_thread::sleep_for(m_delay);
    }

    ok = ok && m_device.emitEvent(EV_KEY, KEY_LEFTSHIFT, 1);
    ok = ok && m_device.emitEvent(EV_SYN, SYN_REPORT, 0);
    if (m_delay.count() > 0) {
        std::this_thread::sleep_for(m_delay);
    }

    ok = ok && m_device.emitEvent(EV_KEY, KEY_V, 1);
    ok = ok && m_device.emitEvent(EV_SYN, SYN_REPORT, 0);
    if (m_delay.count() > 0) {
        std::this_thread::sleep_for(m_delay);
    }

    ok = ok && m_device.emitEvent(EV_KEY, KEY_V, 0);
    ok = ok && m_device.emitEvent(EV_SYN, SYN_REPORT, 0);
    if (m_delay.count() > 0) {
        std::this_thread::sleep_for(m_delay);
    }

    ok = ok && m_device.emitEvent(EV_KEY, KEY_LEFTSHIFT, 0);
    ok = ok && m_device.emitEvent(EV_SYN, SYN_REPORT, 0);
    if (m_delay.count() > 0) {
        std::this_thread::sleep_for(m_delay);
    }

    ok = ok && m_device.emitEvent(EV_KEY, KEY_LEFTCTRL, 0);
    ok = ok && m_device.emitEvent(EV_SYN, SYN_REPORT, 0);

    if (!ok) {
        KEYINJECTORD_LOG_ERROR("Ctrl+Shift+V sequence failed during event emission");
        return false;
    }

    KEYINJECTORD_LOG_DEBUG("Ctrl+Shift+V sequence successfully written to device");
    return true;
}

} // namespace keyinjectord
