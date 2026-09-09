#include "capability.h"

#include "logging.h"

#include <cstring>
#include <stdexcept>

#include <sys/capability.h>
#include <sys/prctl.h>

namespace keyinjectord {

namespace {

struct CapHandle {
    cap_t caps = nullptr;

    CapHandle() {
        caps = cap_get_proc();
        if (!caps) {
            throw std::runtime_error(std::string("cap_get_proc failed: ") + std::strerror(errno));
        }
    }
    ~CapHandle() {
        if (caps) {
            cap_free(caps);
        }
    }

    CapHandle(const CapHandle&) = delete;
    CapHandle& operator=(const CapHandle&) = delete;
};

void setCapEffective(cap_flag_value_t value) {
    CapHandle h;
    cap_value_t capList[] = {CAP_DAC_OVERRIDE};

    if (cap_set_flag(h.caps, CAP_EFFECTIVE, 1, capList, value) != 0) {
        throw std::runtime_error(std::string("cap_set_flag failed: ") + std::strerror(errno));
    }

    if (cap_set_proc(h.caps) != 0) {
        throw std::runtime_error(std::string("cap_set_proc failed: ") + std::strerror(errno));
    }
}

} // anonymous namespace

void raiseCapDacOverride() {
    setCapEffective(CAP_SET);
    KEYINJECTORD_LOG_INFO("CAP_DAC_OVERRIDE raised into effective set");
}

void lowerCapDacOverride() {
    setCapEffective(CAP_CLEAR);
    KEYINJECTORD_LOG_INFO("CAP_DAC_OVERRIDE lowered from effective set");
}

void permanentlyDropCapDacOverride() {
    CapHandle h;
    cap_value_t capList[] = {CAP_DAC_OVERRIDE};

    if (cap_set_flag(h.caps, CAP_EFFECTIVE, 1, capList, CAP_CLEAR) != 0 ||
        cap_set_flag(h.caps, CAP_PERMITTED, 1, capList, CAP_CLEAR) != 0) {
        throw std::runtime_error(std::string("cap_set_flag (permanent drop) failed: ") + std::strerror(errno));
    }
    if (cap_set_proc(h.caps) != 0) {
        throw std::runtime_error(std::string("cap_set_proc (permanent drop) failed: ") + std::strerror(errno));
    }
    KEYINJECTORD_LOG_INFO("CAP_DAC_OVERRIDE permanently dropped from Effective+Permitted");

    if (cap_drop_bound(CAP_DAC_OVERRIDE) == 0) {
        KEYINJECTORD_LOG_INFO("CAP_DAC_OVERRIDE dropped from Bounding set");
    } else {
        KEYINJECTORD_LOG_INFO("Bounding-set drop skipped (CAP_SETPCAP not available)");
    }

    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
        KEYINJECTORD_LOG_ERROR("PR_SET_NO_NEW_PRIVS failed: %s", std::strerror(errno));
    } else {
        KEYINJECTORD_LOG_INFO("PR_SET_NO_NEW_PRIVS set — process is now fully unprivileged");
    }
}

CapabilityGuard::CapabilityGuard() {
    raiseCapDacOverride();
    m_raised = true;
}

CapabilityGuard::~CapabilityGuard() {
    if (m_raised) {
        try {
            lowerCapDacOverride();
        } catch (const std::exception& e) {
            KEYINJECTORD_LOG_ERROR("Failed to lower capability: %s", e.what());
        }
    }
}

} // namespace keyinjectord
