#include "config.h"
#include "logging.h"
#include <cstdio>
#include <fstream>
#include <iostream>


// filesystem
/*
 * references
 *  - https://stackoverflow.com/questions/48312460/c17-filesystem-is-not-a-namespace-name
 *  - https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
 */
#if __GNUC__ <  8
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#else
#include <filesystem>
namespace filesystem = std::filesystem;
#endif

#ifdef __SPDLOG__
auto console = spdlog::stdout_color_mt("Main");
#endif

void build_info(void)
{
    LOG_INFO("GCC __VERSION__={}", __VERSION__);
    LOG_INFO("CPP STANDARD __cplusplus={}", __cplusplus);
    LOG_INFO("Build Date={}", __DATE__);
    LOG_INFO("Git Branch=[{}]", __GIT_BRANCH__);
    LOG_INFO("    +-> commit {}", __GIT_COMMIT_HASH__);
}

int main(int argc, char *argv[])
{

    build_info();

    const std::string home_dir_path = getenv("HOME");
    const std::string cache_dir_path = CACHE_DIR_PATH;
    const std::string config_file_path = home_dir_path + cache_dir_path + "/" + CONFIG_FILE_NAME;

    LOG_INFO("Trying to load {}", config_file_path.c_str());
    if (filesystem::exists(config_file_path))
    {
        // https://cplusplus.com/reference/istream/istream/tellg/
        char *buf = nullptr;
        std::ifstream is(config_file_path);
        if (!is.is_open()) {
            LOG_ERR("{} is already opened.");
            return -1;
        }

        is.seekg(0, is.end);
        int len = is.tellg();
        is.seekg(0, is.beg);

        buf = new char[len];
        is.read(buf, len);
        is.close();

        std::cout.write(buf, len);


        delete[] buf;
    }
    else
    {
        LOG_ERR("No HW configuration");
        return -1;
    }

    return 0;
}
