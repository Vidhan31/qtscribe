#pragma once

#include <stdexcept>
#include <string>

#include <sys/capability.h>

namespace keyinjectord {

void raiseCapDacOverride();
void lowerCapDacOverride();
void permanentlyDropCapDacOverride();

class CapabilityGuard {
public:
    CapabilityGuard();
    ~CapabilityGuard();

    CapabilityGuard(const CapabilityGuard&) = delete;
    CapabilityGuard& operator=(const CapabilityGuard&) = delete;

private:
    bool m_raised = false;
};

} // namespace keyinjectord
