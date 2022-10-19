#include "V4L2Camera.hpp"

#include <string>

V4L2Camera::V4L2Camera(const std::vector<std::string> &camera_names)
{
    camera_card_strs = camera_names;
}

int V4L2Camera::init(const int video_width, const int video_height)
{
    this->video_width = video_width;

    _init_dev_files();

    if (_find_dev_file_by_strs(camera_card_strs, camera_dev_file)) {
        printf(">> Camera (%s)\n", camera_dev_file.c_str());
        // TODO
        /*
        if (!_configure_camera(camera_dev_file))
        {
            fprintf(stderr, "[ERROR] Couldn't configure Camera\n");
            return -1;
        }*/
    } else {
        fprintf(stderr, "[ERROR] Couldn't find Camera's device file\n");
        return -1;
    }

    initialized = 1;
    return 0;
}

int V4L2Camera::deinit(void)
{
    return 0;
}

int V4L2Camera::isValid(void)
{
    return initialized;
}

int V4L2Camera::getGStreamVideoSourceStr(std::string &str)
{
    str = camera_dev_file;
    if (str.empty())
        return -1;
    return 0;
}
