#pragma once

#include "launcher_auth.h"

#include <filesystem>

#include <sys/stat.h>
#include <sys/types.h>

namespace keyinjectord {

class FsSecurityChecker {
public:
    static bool isUntrustedPath(const std::filesystem::path& path);

    static bool validateDirectoryAncestry(const std::filesystem::path& rawPath, AuthResult* outResult = nullptr,
                                          bool allowDevMode = isDevAuthDefault(), struct stat* outLeafStat = nullptr,
                                          bool rejectGroupWritable = !isDevAuthDefault());
};

} // namespace keyinjectord
