#include "launcher_auth.h"

#include "logging.h"

#include "FsSecurityChecker.h"
#include "ProcfsUtils.h"
#include "SocketCredentials.h"

#include <string>
#include <string_view>

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

const char* authResultToString(AuthResult result) {
    switch (result) {
        case AuthResult::Success:
            return "Success";
        case AuthResult::InvalidFd:
            return "Invalid file descriptor";
        case AuthResult::NotASocket:
            return "File descriptor is not a SOCK_STREAM socket";
        case AuthResult::NotConnectedUnixSocket:
            return "File descriptor is not a connected AF_UNIX socket";
        case AuthResult::PeerCredsFailed:
            return "Failed to retrieve SO_PEERCRED from socket";
        case AuthResult::UidMismatch:
            return "Peer UID does not match current process UID";
        case AuthResult::PidMismatch:
            return "Peer PID does not match parent process PID";
        case AuthResult::ParentExeReadFailed:
            return "Failed to resolve parent executable path from /proc";
        case AuthResult::SelfExeReadFailed:
            return "Failed to resolve self executable path from /proc";
        case AuthResult::UnauthorizedExecutable:
            return "Parent executable name is not authorized";
        case AuthResult::StatFailed:
            return "Failed to stat parent executable binary or directory";
        case AuthResult::NotRegularFile:
            return "Parent executable is not a regular file";
        case AuthResult::WorldWritable:
            return "Parent executable or ancestor directory is world-writable";
        case AuthResult::GroupWritable:
            return "Parent executable or ancestor directory is group-writable";
        case AuthResult::NonRootOwner:
            return "Production helper executable, parent binary, or directory is not owned by root (UID 0)";
        case AuthResult::UntrustedLocation:
            return "Parent executable is located in an untrusted or non-colocated directory";
        case AuthResult::DeletedExecutable:
            return "Executable binary was unlinked or deleted from disk";
        case AuthResult::InodeMismatch:
            return "Process executable inode does not match filesystem path inode";
    }
    return "Unknown error";
}

bool validateDirectoryAncestry(const std::filesystem::path& rawPath, AuthResult* outResult, bool allowDevMode,
                               struct stat* outLeafStat, bool rejectGroupWritable) {
    return FsSecurityChecker::validateDirectoryAncestry(rawPath, outResult, allowDevMode, outLeafStat,
                                                        rejectGroupWritable);
}

bool validateExecutableTopology(const std::filesystem::path& parentExePath, const std::filesystem::path& selfExePath,
                                AuthResult* outResult, bool allowDevMode, struct stat* outParentStat,
                                bool rejectGroupWritable) {
    std::string parentStr = parentExePath.string();
    std::string selfStr = selfExePath.string();

    if (parentStr.empty() || selfStr.empty()) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: empty executable path");
        return setResult(outResult, AuthResult::UntrustedLocation);
    }

    if (parentStr.ends_with(" (deleted)") || selfStr.ends_with(" (deleted)")) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: executable is deleted (parent: '%s', self: '%s')",
                               parentStr.c_str(), selfStr.c_str());
        return setResult(outResult, AuthResult::DeletedExecutable);
    }

    if (FsSecurityChecker::isUntrustedPath(parentExePath)) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: parent binary in untrusted directory '%s'",
                               parentExePath.c_str());
        return setResult(outResult, AuthResult::UntrustedLocation);
    }

    std::string parentExeName = parentExePath.filename().string();
    if (allowDevMode) {
        if (parentExeName != "qtscribe" && parentExeName != "test_ipc_server" &&
            parentExeName != "test_transcription_pipeline") {
            KEYINJECTORD_LOG_ERROR("Launcher authorization failed: unauthorized parent executable name '%s' at '%s'",
                                   parentExeName.c_str(), parentExePath.c_str());
            return setResult(outResult, AuthResult::UnauthorizedExecutable);
        }
    } else {
        if (parentExeName != "qtscribe") {
            KEYINJECTORD_LOG_ERROR("Launcher authorization failed: unauthorized parent executable name '%s' at '%s'",
                                   parentExeName.c_str(), parentExePath.c_str());
            return setResult(outResult, AuthResult::UnauthorizedExecutable);
        }
    }

    std::filesystem::path parentDir = parentExePath.parent_path().lexically_normal();
    std::filesystem::path selfDir = selfExePath.parent_path().lexically_normal();

    if (parentExePath != selfExePath && parentDir != selfDir) {
        KEYINJECTORD_LOG_ERROR(
            "Launcher authorization failed: parent path '%s' is not colocated in helper directory '%s'",
            parentExePath.c_str(), selfExePath.c_str());
        return setResult(outResult, AuthResult::UntrustedLocation);
    }

    struct stat selfLeafStat {};
    if (!validateDirectoryAncestry(selfExePath, outResult, allowDevMode, &selfLeafStat, rejectGroupWritable)) {
        return false;
    }

    if (!S_ISREG(selfLeafStat.st_mode)) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: self executable '%s' is not a regular file",
                               selfExePath.c_str());
        return setResult(outResult, AuthResult::NotRegularFile);
    }

    if (parentExePath == selfExePath) {
        if (outParentStat) {
            *outParentStat = selfLeafStat;
        }
        return setResult(outResult, AuthResult::Success);
    }

    struct stat parentLeafStat {};
    if (!validateDirectoryAncestry(parentExePath, outResult, allowDevMode, &parentLeafStat, rejectGroupWritable)) {
        return false;
    }

    if (!S_ISREG(parentLeafStat.st_mode)) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: parent executable '%s' is not a regular file",
                               parentExePath.c_str());
        return setResult(outResult, AuthResult::NotRegularFile);
    }

    if (outParentStat) {
        *outParentStat = parentLeafStat;
    }

    return setResult(outResult, AuthResult::Success);
}

bool authorizeLauncher(int socketFd, AuthResult* outResult, bool allowDevMode, bool rejectGroupWritable) {
    struct ucred peerCreds {};
    if (!SocketCredentials::verifySocketAndGetPeer(socketFd, peerCreds, outResult)) {
        return false;
    }

    if (!SocketCredentials::validatePeerIdentity(peerCreds, outResult)) {
        return false;
    }

    struct stat peerFdStat {};
    int peerExeFd = ProcfsUtils::openProcExeFd(peerCreds.pid, &peerFdStat, outResult);
    if (peerExeFd < 0) {
        return false;
    }

    if (!ProcfsUtils::validateProcFdSecurity(peerFdStat, "peer executable", outResult, allowDevMode,
                                             rejectGroupWritable)) {
        close(peerExeFd);
        return false;
    }

    struct stat selfFdStat {};
    int selfExeFd = ProcfsUtils::openSelfExeFd(&selfFdStat, outResult);
    if (selfExeFd < 0) {
        close(peerExeFd);
        return false;
    }
    close(selfExeFd);

    if (selfFdStat.st_nlink == 0) {
        close(peerExeFd);
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: self executable is deleted (st_nlink == 0)");
        return setResult(outResult, AuthResult::DeletedExecutable);
    }

    std::filesystem::path parentExePath;
    if (!ProcfsUtils::readProcExePath(peerCreds.pid, parentExePath, outResult)) {
        close(peerExeFd);
        return false;
    }

    std::filesystem::path selfExePath;
    if (!ProcfsUtils::readSelfExePath(selfExePath, outResult)) {
        close(peerExeFd);
        return false;
    }

    struct stat diskParentStat {};
    if (!validateExecutableTopology(parentExePath, selfExePath, outResult, allowDevMode, &diskParentStat,
                                    rejectGroupWritable)) {
        close(peerExeFd);
        return false;
    }

    if (peerFdStat.st_dev != diskParentStat.st_dev || peerFdStat.st_ino != diskParentStat.st_ino) {
        close(peerExeFd);
        KEYINJECTORD_LOG_ERROR(
            "Launcher authorization failed: inode mismatch between /proc/%d/exe (dev=%llu, ino=%llu) and disk '%s' "
            "(dev=%llu, ino=%llu)",
            peerCreds.pid, static_cast<unsigned long long>(peerFdStat.st_dev),
            static_cast<unsigned long long>(peerFdStat.st_ino), parentExePath.c_str(),
            static_cast<unsigned long long>(diskParentStat.st_dev),
            static_cast<unsigned long long>(diskParentStat.st_ino));
        return setResult(outResult, AuthResult::InodeMismatch);
    }

    close(peerExeFd);
    KEYINJECTORD_LOG_INFO("Launcher authorization verified for peer PID %d (%s)", peerCreds.pid, parentExePath.c_str());
    return setResult(outResult, AuthResult::Success);
}

} // namespace keyinjectord
