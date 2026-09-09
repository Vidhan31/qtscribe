#include "SocketCredentials.h"

#include "logging.h"

#include <cerrno>
#include <cstring>

#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>

namespace keyinjectord {

namespace {

bool setResult(AuthResult* outResult, AuthResult res) {
    if (outResult) {
        *outResult = res;
    }
    return res == AuthResult::Success;
}

} // namespace

bool SocketCredentials::verifySocketAndGetPeer(int socketFd, struct ucred& outPeerCreds, AuthResult* outResult) {
    if (socketFd < 0 || fcntl(socketFd, F_GETFD) == -1) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: invalid socket descriptor %d (%s)", socketFd,
                               std::strerror(errno));
        return setResult(outResult, AuthResult::InvalidFd);
    }

    int type = 0;
    socklen_t typeLen = sizeof(type);
    if (getsockopt(socketFd, SOL_SOCKET, SO_TYPE, &type, &typeLen) != 0 || type != SOCK_STREAM) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: descriptor %d is not a SOCK_STREAM socket", socketFd);
        return setResult(outResult, AuthResult::NotASocket);
    }

    struct sockaddr_storage addr {};
    socklen_t addrLen = sizeof(addr);
    if (getpeername(socketFd, reinterpret_cast<struct sockaddr*>(&addr), &addrLen) != 0 || addr.ss_family != AF_UNIX) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: descriptor %d is not an AF_UNIX connected socket",
                               socketFd);
        return setResult(outResult, AuthResult::NotConnectedUnixSocket);
    }

    struct ucred creds {};
    socklen_t credLen = sizeof(creds);
    if (getsockopt(socketFd, SOL_SOCKET, SO_PEERCRED, &creds, &credLen) != 0) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: getsockopt(SO_PEERCRED) error: %s",
                               std::strerror(errno));
        return setResult(outResult, AuthResult::PeerCredsFailed);
    }

    outPeerCreds = creds;
    return setResult(outResult, AuthResult::Success);
}

bool SocketCredentials::validatePeerIdentity(const struct ucred& peerCreds, AuthResult* outResult) {
    if (peerCreds.uid != getuid()) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: peer UID %d does not match process UID %d",
                               peerCreds.uid, getuid());
        return setResult(outResult, AuthResult::UidMismatch);
    }

    pid_t parentPid = getppid();
    if (peerCreds.pid != parentPid && peerCreds.pid != getpid()) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: peer PID %d does not match parent PID %d", peerCreds.pid,
                               parentPid);
        return setResult(outResult, AuthResult::PidMismatch);
    }

    if (peerCreds.pid <= 1) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: invalid peer PID %d", peerCreds.pid);
        return setResult(outResult, AuthResult::PidMismatch);
    }

    return setResult(outResult, AuthResult::Success);
}

} // namespace keyinjectord
