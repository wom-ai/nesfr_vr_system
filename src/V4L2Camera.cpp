#include "V4L2Camera.hpp"

#include <string>

V4L2Camera::V4L2Camera(void)
{
}

int V4L2Camera::init(const int video_width, const int video_height)
{
    this->video_width = video_width;

    _init_dev_files();

    // right
    if (_find_dev_file_by_strs(camera_card_strs, camera_dev_file)) {
        printf(">> Camera (Right) (%s)\n", camera_dev_file.c_str());
        if (!_configure_camera(camera_dev_file))
        {
            fprintf(stderr, "[ERROR] Couldn't configure Camera (Right)\n");
            return -1;
        }
    } else {
        fprintf(stderr, "[ERROR] Couldn't open Camera (Right)\n");
        return -1;
    }
    return 0;
}

int V4L2Camera::deinit(void)
{
    return 0;
}

int V4L2Camera::isValid(void)
{
    return 0;
}

int V4L2Camera::getGStreamVideoSourceStr(std::string &str)
{
    str = camera_dev_file;
    if (str.empty())
        return -1;
    return 0;
}
