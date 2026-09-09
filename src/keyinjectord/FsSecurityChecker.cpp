#include "FsSecurityChecker.h"

#include "logging.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include <sys/stat.h>
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

bool FsSecurityChecker::isUntrustedPath(const std::filesystem::path& path) {
    static constexpr auto kUntrustedPrefixes =
        std::to_array<std::string_view>({"/tmp", "/var/tmp", "/dev/shm", "/run/user"});
    const std::string norm = path.lexically_normal().string();
    const std::string_view normView = norm;
    return std::ranges::any_of(kUntrustedPrefixes, [&](std::string_view prefix) {
        return normView == prefix ||
               (normView.starts_with(prefix) && normView.size() > prefix.size() && normView[prefix.size()] == '/');
    });
}

bool FsSecurityChecker::validateDirectoryAncestry(const std::filesystem::path& rawPath, AuthResult* outResult,
                                                  bool allowDevMode, struct stat* outLeafStat,
                                                  bool rejectGroupWritable) {
    std::error_code ec;
    std::filesystem::path canon = std::filesystem::canonical(rawPath, ec);
    if (ec || canon.empty() || !canon.is_absolute()) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: canonicalize path '%s' error: %s", rawPath.c_str(),
                               ec.message().c_str());
        return setResult(outResult, AuthResult::StatFailed);
    }

    if (isUntrustedPath(canon)) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: path in untrusted directory '%s'",
                               canon.string().c_str());
        return setResult(outResult, AuthResult::UntrustedLocation);
    }

    std::vector<std::string> parts;
    for (const auto& part : canon) {
        std::string s = part.string();
        if (s.empty() || s == "/") {
            continue;
        }
        parts.push_back(s);
    }

    int currentFd = open("/", O_PATH | O_DIRECTORY | O_CLOEXEC);
    if (currentFd < 0) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: open('/') error: %s", std::strerror(errno));
        return setResult(outResult, AuthResult::StatFailed);
    }

    struct stat st {};
    if (fstat(currentFd, &st) != 0) {
        close(currentFd);
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: fstat('/') error: %s", std::strerror(errno));
        return setResult(outResult, AuthResult::StatFailed);
    }

    if (!S_ISDIR(st.st_mode)) {
        close(currentFd);
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: '/' is not a directory");
        return setResult(outResult, AuthResult::StatFailed);
    }

    if (st.st_mode & S_IWOTH) {
        close(currentFd);
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: root directory '/' is world-writable");
        return setResult(outResult, AuthResult::WorldWritable);
    }

    if (rejectGroupWritable && (st.st_mode & S_IWGRP)) {
        close(currentFd);
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: root directory '/' is group-writable");
        return setResult(outResult, AuthResult::GroupWritable);
    }

    if (!allowDevMode && st.st_uid != 0) {
        close(currentFd);
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: root directory '/' is not root-owned (UID: %d)",
                               st.st_uid);
        return setResult(outResult, AuthResult::NonRootOwner);
    }

    if (allowDevMode && st.st_uid != 0 && st.st_uid != getuid()) {
        close(currentFd);
        KEYINJECTORD_LOG_ERROR("Dev launcher authorization failed: '/' owned by UID %d (expected 0 or %d)", st.st_uid,
                               getuid());
        return setResult(outResult, AuthResult::UntrustedLocation);
    }

    if (parts.empty()) {
        if (outLeafStat) {
            *outLeafStat = st;
        }
        close(currentFd);
        return setResult(outResult, AuthResult::Success);
    }

    for (size_t i = 0; i + 1 < parts.size(); ++i) {
        int nextFd = openat(currentFd, parts[i].c_str(), O_PATH | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (nextFd < 0) {
            close(currentFd);
            KEYINJECTORD_LOG_ERROR("Launcher authorization failed: openat directory component '%s' error: %s",
                                   parts[i].c_str(), std::strerror(errno));
            return setResult(outResult, AuthResult::StatFailed);
        }
        close(currentFd);
        currentFd = nextFd;

        if (fstat(currentFd, &st) != 0) {
            close(currentFd);
            KEYINJECTORD_LOG_ERROR("Launcher authorization failed: fstat component '%s' error: %s", parts[i].c_str(),
                                   std::strerror(errno));
            return setResult(outResult, AuthResult::StatFailed);
        }

        if (!S_ISDIR(st.st_mode)) {
            close(currentFd);
            KEYINJECTORD_LOG_ERROR("Launcher authorization failed: ancestor '%s' is not a directory", parts[i].c_str());
            return setResult(outResult, AuthResult::StatFailed);
        }

        if (st.st_mode & S_IWOTH) {
            close(currentFd);
            KEYINJECTORD_LOG_ERROR("Launcher authorization failed: ancestor directory '%s' is world-writable",
                                   parts[i].c_str());
            return setResult(outResult, AuthResult::WorldWritable);
        }

        if (rejectGroupWritable && (st.st_mode & S_IWGRP)) {
            close(currentFd);
            KEYINJECTORD_LOG_ERROR("Launcher authorization failed: ancestor directory '%s' is group-writable",
                                   parts[i].c_str());
            return setResult(outResult, AuthResult::GroupWritable);
        }

        if (!allowDevMode && st.st_uid != 0) {
            close(currentFd);
            KEYINJECTORD_LOG_ERROR("Launcher authorization failed: ancestor directory '%s' is not root-owned (UID: %d)",
                                   parts[i].c_str(), st.st_uid);
            return setResult(outResult, AuthResult::NonRootOwner);
        }

        if (allowDevMode && st.st_uid != 0 && st.st_uid != getuid()) {
            close(currentFd);
            KEYINJECTORD_LOG_ERROR(
                "Dev launcher authorization failed: ancestor directory '%s' owned by UID %d (expected 0 or %d)",
                parts[i].c_str(), st.st_uid, getuid());
            return setResult(outResult, AuthResult::UntrustedLocation);
        }
    }

    const std::string& leafName = parts.back();
    int leafFd = openat(currentFd, leafName.c_str(), O_PATH | O_NOFOLLOW | O_CLOEXEC);
    if (leafFd < 0) {
        close(currentFd);
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: openat leaf '%s' error: %s", leafName.c_str(),
                               std::strerror(errno));
        return setResult(outResult, AuthResult::StatFailed);
    }
    close(currentFd);

    if (fstat(leafFd, &st) != 0) {
        close(leafFd);
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: fstat leaf '%s' error: %s", leafName.c_str(),
                               std::strerror(errno));
        return setResult(outResult, AuthResult::StatFailed);
    }
    close(leafFd);

    if (outLeafStat) {
        *outLeafStat = st;
    }

    if (st.st_nlink == 0) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: leaf '%s' has 0 hard links (deleted)", leafName.c_str());
        return setResult(outResult, AuthResult::DeletedExecutable);
    }

    if (st.st_mode & S_IWOTH) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: leaf '%s' is world-writable", leafName.c_str());
        return setResult(outResult, AuthResult::WorldWritable);
    }

    if (rejectGroupWritable && (st.st_mode & S_IWGRP)) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: leaf '%s' is group-writable", leafName.c_str());
        return setResult(outResult, AuthResult::GroupWritable);
    }

    if (!allowDevMode && st.st_uid != 0) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: leaf '%s' is not root-owned (UID: %d)", leafName.c_str(),
                               st.st_uid);
        return setResult(outResult, AuthResult::NonRootOwner);
    }

    if (allowDevMode && st.st_uid != 0 && st.st_uid != getuid()) {
        KEYINJECTORD_LOG_ERROR("Dev launcher authorization failed: leaf '%s' owned by UID %d (expected 0 or %d)",
                               leafName.c_str(), st.st_uid, getuid());
        return setResult(outResult, AuthResult::UntrustedLocation);
    }

    return setResult(outResult, AuthResult::Success);
}

} // namespace keyinjectord
