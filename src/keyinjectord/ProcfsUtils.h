#pragma once

#include "launcher_auth.h"

#include <filesystem>
#include <string>

#include <sys/stat.h>
#include <sys/types.h>

namespace keyinjectord {

class ProcfsUtils {
public:
    static int openProcExeFd(pid_t pid, struct stat* outStat = nullptr, AuthResult* outResult = nullptr);
    static int openSelfExeFd(struct stat* outStat = nullptr, AuthResult* outResult = nullptr);

    static bool readProcExePath(pid_t pid, std::filesystem::path& outPath, AuthResult* outResult = nullptr);
    static bool readSelfExePath(std::filesystem::path& outPath, AuthResult* outResult = nullptr);

    static bool validateProcFdSecurity(const struct stat& st, const std::string& description, AuthResult* outResult,
                                       bool allowDevMode, bool rejectGroupWritable);
};

} // namespace keyinjectord
