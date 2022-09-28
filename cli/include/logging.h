#ifdef __SPDLOG__

#include <spdlog/spdlog.h>

#define LOG_INFO(fmt, ...)  \
    console->info(fmt, ##__VA_ARGS__);

#define LOG_DEBUG(fmt, ...)  \
    console->debug(fmt, ##__VA_ARGS__);

#define LOG_WARN(fmt, ...)  \
    console->warn(fmt, ##__VA_ARGS__);

#define LOG_ERR(fmt, ...)  \
    console->error(fmt, ##__VA_ARGS__);

#else // printf

#define LOG_INFO(fmt, ...)  \
    std::printf("[info] " fmt, ##__VA_ARGS__);

#define LOG_DEBUG(fmt, ...)  \
    std::printf("[debug] " fmt, ##__VA_ARGS__);

#define LOG_WARN(fmt, ...)  \
    std::printf("[warn] " fmt, ##__VA_ARGS__);

#define LOG_ERR(fmt, ...)  \
    std::fprintf(stderr, "[error] " fmt, ##__VA_ARGS__);
#endif
