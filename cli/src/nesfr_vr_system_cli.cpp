#include "config.h"
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

int main(int argc, char *argv[])
{

    const std::string cache_dir_path = CACHE_DIR_PATH;
    const std::string config_file_path = cache_dir_path + "/" + CONFIG_FILE_NAME;

    if (filesystem::exists(config_file_path))
    {
        char *buf = nullptr;
        std::ifstream is(config_file_path);
        if (!is.is_open())
            return -1;

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

        return -1;
    }

    return 0;
}
