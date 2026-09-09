#pragma once

#include "launcher_auth.h"

#include <sys/socket.h>
#include <sys/types.h>

namespace keyinjectord {

class SocketCredentials {
public:
    static bool verifySocketAndGetPeer(int socketFd, struct ucred& outPeerCreds, AuthResult* outResult = nullptr);
    static bool validatePeerIdentity(const struct ucred& peerCreds, AuthResult* outResult = nullptr);
};

} // namespace keyinjectord
