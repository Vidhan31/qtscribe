#include "capability.h"
#include "ipc_server.h"
#include "logging.h"
#include "uinput_device.h"

#include "KeyboardMacroInjector.h"
#include "launcher_auth.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

#include <sys/prctl.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

namespace {

void printUsage(const char* progName) {
    std::fprintf(stderr,
                 "Usage: %s --socket-fd <FD> [OPTIONS]\n"
                 "\n"
                 "Required:\n"
                 "  --socket-fd <FD>   Inherited UNIX domain socket file descriptor\n"
                 "\n"
                 "Options:\n"
                 "  --delay-ms <N>     Inter-key delay in milliseconds (default: 18)\n"
                 "  -v, --verbose      Enable verbose output logging\n"
                 "  --help             Show this help\n"
                 "\n"
                 "Privilege setup:\n"
                 "  sudo setcap \"cap_dac_override+p\" %s\n"
                 "\n",
                 progName, progName);
}

} // namespace

int main(int argc, char* argv[]) {
    prctl(PR_SET_PDEATHSIG, SIGTERM);

    sigset_t blockMask;
    sigemptyset(&blockMask);
    sigaddset(&blockMask, SIGINT);
    sigaddset(&blockMask, SIGTERM);
    if (sigprocmask(SIG_BLOCK, &blockMask, nullptr) != 0) {
        KEYINJECTORD_LOG_ERROR("sigprocmask(SIG_BLOCK) failed: %s", std::strerror(errno));
        return 1;
    }

    int delayMs = 18;
    int socketFd = -1;
    [[maybe_unused]] bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--socket-fd") == 0 && i + 1 < argc) {
            socketFd = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--delay-ms") == 0 && i + 1 < argc) {
            delayMs = std::atoi(argv[++i]);
            if (delayMs < 1)
                delayMs = 1;
        } else if (std::strcmp(argv[i], "--verbose") == 0 || std::strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else {
            KEYINJECTORD_LOG_ERROR("Unknown option: %s", argv[i]);
            printUsage(argv[0]);
            return 1;
        }
    }

    if (socketFd < 0) {
        KEYINJECTORD_LOG_ERROR("Missing required option: --socket-fd <FD>");
        printUsage(argv[0]);
        return 1;
    }

    if (!keyinjectord::authorizeLauncher(socketFd)) {
        return 1;
    }

    KEYINJECTORD_LOG_DEBUG("Starting with delay=%dms, socket-fd=%d", delayMs, socketFd);

    int uinputFd;
    try {
        KEYINJECTORD_LOG_DEBUG("Opening /dev/uinput with capability...");
        keyinjectord::CapabilityGuard capGuard;

        uinputFd = open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
        if (uinputFd < 0) {
            KEYINJECTORD_LOG_ERROR("Failed to open /dev/uinput: %s", std::strerror(errno));
            KEYINJECTORD_LOG_ERROR("Did you run: sudo setcap \"cap_dac_override+p\" %s ?", argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        KEYINJECTORD_LOG_ERROR("Capability error while opening /dev/uinput: %s", e.what());
        KEYINJECTORD_LOG_ERROR("Did you run: sudo setcap \"cap_dac_override+p\" %s ?", argv[0]);
        return 1;
    }

    KEYINJECTORD_LOG_DEBUG("/dev/uinput opened successfully (fd=%d)", uinputFd);

    try {
        keyinjectord::permanentlyDropCapDacOverride();
    } catch (const std::exception& e) {
        KEYINJECTORD_LOG_ERROR("FATAL: Could not drop capabilities: %s", e.what());
        return 1;
    }

    try {
        keyinjectord::UinputDevice device(uinputFd);
        keyinjectord::KeyboardMacroInjector injector(device, std::chrono::milliseconds(delayMs));

        keyinjectord::IpcServer server(socketFd, injector);

        server.run();

        KEYINJECTORD_LOG_DEBUG("Clean shutdown complete");
        return 0;

    } catch (const std::exception& e) {
        KEYINJECTORD_LOG_ERROR("FATAL: %s", e.what());
        return 1;
    }
}
