#include "ProcfsUtils.h"

#include "logging.h"

#include <cerrno>
#include <climits>
#include <cstring>
#include <string>

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

int ProcfsUtils::openProcExeFd(pid_t pid, struct stat* outStat, AuthResult* outResult) {
    std::string procExe = "/proc/" + std::to_string(pid) + "/exe";
    int fd = open(procExe.c_str(), O_PATH | O_CLOEXEC);
    if (fd < 0) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: open(%s) error: %s", procExe.c_str(),
                               std::strerror(errno));
        setResult(outResult, AuthResult::ParentExeReadFailed);
        return -1;
    }

    if (outStat) {
        if (fstat(fd, outStat) != 0) {
            close(fd);
            KEYINJECTORD_LOG_ERROR("Launcher authorization failed: fstat(%s) error: %s", procExe.c_str(),
                                   std::strerror(errno));
            setResult(outResult, AuthResult::StatFailed);
            return -1;
        }
    }

    setResult(outResult, AuthResult::Success);
    return fd;
}

int ProcfsUtils::openSelfExeFd(struct stat* outStat, AuthResult* outResult) {
    int fd = open("/proc/self/exe", O_PATH | O_CLOEXEC);
    if (fd < 0) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: open('/proc/self/exe') error: %s", std::strerror(errno));
        setResult(outResult, AuthResult::SelfExeReadFailed);
        return -1;
    }

    if (outStat) {
        if (fstat(fd, outStat) != 0) {
            close(fd);
            KEYINJECTORD_LOG_ERROR("Launcher authorization failed: fstat('/proc/self/exe') error: %s",
                                   std::strerror(errno));
            setResult(outResult, AuthResult::StatFailed);
            return -1;
        }
    }

    setResult(outResult, AuthResult::Success);
    return fd;
}

bool ProcfsUtils::readProcExePath(pid_t pid, std::filesystem::path& outPath, AuthResult* outResult) {
    std::string procExe = "/proc/" + std::to_string(pid) + "/exe";
    char buf[PATH_MAX];
    ssize_t len = readlink(procExe.c_str(), buf, sizeof(buf) - 1);
    if (len <= 0) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: readlink(%s) error: %s", procExe.c_str(),
                               std::strerror(errno));
        return setResult(outResult, AuthResult::ParentExeReadFailed);
    }
    buf[len] = '\0';
    outPath = std::filesystem::path(buf);
    return setResult(outResult, AuthResult::Success);
}

bool ProcfsUtils::readSelfExePath(std::filesystem::path& outPath, AuthResult* outResult) {
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len <= 0) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: readlink(/proc/self/exe) error: %s",
                               std::strerror(errno));
        return setResult(outResult, AuthResult::SelfExeReadFailed);
    }
    buf[len] = '\0';
    outPath = std::filesystem::path(buf);
    return setResult(outResult, AuthResult::Success);
}

bool ProcfsUtils::validateProcFdSecurity(const struct stat& st, const std::string& description, AuthResult* outResult,
                                         bool allowDevMode, bool rejectGroupWritable) {
    if (st.st_nlink == 0) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: %s is deleted (st_nlink == 0)", description.c_str());
        return setResult(outResult, AuthResult::DeletedExecutable);
    }

    if (!S_ISREG(st.st_mode)) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: %s is not a regular file", description.c_str());
        return setResult(outResult, AuthResult::NotRegularFile);
    }

    if (st.st_mode & S_IWOTH) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: %s is world-writable", description.c_str());
        return setResult(outResult, AuthResult::WorldWritable);
    }

    if (rejectGroupWritable && (st.st_mode & S_IWGRP)) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: %s is group-writable", description.c_str());
        return setResult(outResult, AuthResult::GroupWritable);
    }

    if (!allowDevMode && st.st_uid != 0) {
        KEYINJECTORD_LOG_ERROR("Launcher authorization failed: %s not root-owned (UID: %d)", description.c_str(),
                               st.st_uid);
        return setResult(outResult, AuthResult::NonRootOwner);
    }

    if (allowDevMode && st.st_uid != 0 && st.st_uid != getuid()) {
        KEYINJECTORD_LOG_ERROR("Dev launcher authorization failed: %s owned by UID %d (expected 0 or %d)",
                               description.c_str(), st.st_uid, getuid());
        return setResult(outResult, AuthResult::UntrustedLocation);
    }

    return setResult(outResult, AuthResult::Success);
}

} // namespace keyinjectord
