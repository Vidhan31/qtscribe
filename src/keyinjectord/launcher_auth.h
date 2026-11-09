#pragma once

#include <cstdint>
#include <filesystem>

#include <sys/stat.h>
#include <sys/types.h>

namespace keyinjectord {

enum class AuthResult : std::uint8_t {
    Success,
    InvalidFd,
    NotASocket,
    NotConnectedUnixSocket,
    PeerCredsFailed,
    UidMismatch,
    PidMismatch,
    ParentExeReadFailed,
    SelfExeReadFailed,
    UnauthorizedExecutable,
    StatFailed,
    NotRegularFile,
    WorldWritable,
    GroupWritable,
    NonRootOwner,
    UntrustedLocation,
    DeletedExecutable,
    InodeMismatch
};

const char* authResultToString(AuthResult result);

constexpr bool isDevAuthDefault() {
#ifdef KEYINJECTORD_DEV_AUTH
    return true;
#else
    return false;
#endif
}

bool validateDirectoryAncestry(const std::filesystem::path& path, AuthResult* outResult = nullptr,
                               bool allowDevMode = isDevAuthDefault(), struct stat* outLeafStat = nullptr,
                               bool rejectGroupWritable = !isDevAuthDefault());

bool validateExecutableTopology(const std::filesystem::path& parentExePath, const std::filesystem::path& selfExePath,
                                AuthResult* outResult = nullptr, bool allowDevMode = isDevAuthDefault(),
                                struct stat* outParentStat = nullptr, bool rejectGroupWritable = !isDevAuthDefault());

bool authorizeLauncher(int socketFd, AuthResult* outResult = nullptr, bool allowDevMode = isDevAuthDefault(),
                       bool rejectGroupWritable = !isDevAuthDefault());

} // namespace keyinjectord
