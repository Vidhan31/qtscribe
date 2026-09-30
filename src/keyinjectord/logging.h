#ifndef KEYINJECTORD_LOGGING_H
#define KEYINJECTORD_LOGGING_H

#include <cstdio>

#if defined(NDEBUG) || defined(KEYINJECTORD_NO_DEBUG_OUTPUT)
#define KEYINJECTORD_LOG_DEBUG(fmt, ...) \
    do {                                 \
    } while (0)
#else
#define KEYINJECTORD_LOG_DEBUG(fmt, ...) std::fprintf(stderr, "[keyinjectord][DEBUG] " fmt "\n", ##__VA_ARGS__)
#endif

#define KEYINJECTORD_LOG_ERROR(fmt, ...) std::fprintf(stderr, "[keyinjectord][ERROR] " fmt "\n", ##__VA_ARGS__)

#define KEYINJECTORD_LOG_WARN(fmt, ...) std::fprintf(stderr, "[keyinjectord][WARN] " fmt "\n", ##__VA_ARGS__)

#define KEYINJECTORD_LOG_INFO(fmt, ...) std::fprintf(stderr, "[keyinjectord][INFO] " fmt "\n", ##__VA_ARGS__)

#endif // KEYINJECTORD_LOGGING_H
