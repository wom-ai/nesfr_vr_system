#include "V4L2StereoCamera.hpp"

#include <string>

V4L2StereoCamera::V4L2StereoCamera(const std::vector<std::string> &camera_names_left, const std::vector<std::string> &camera_names_right)
{
    this->stereo_camera_left_strs = camera_names_left;
    this->stereo_camera_right_strs = camera_names_right;
}

int V4L2StereoCamera::init(const int stereo_video_width, const int stereo_video_height, const int mono_flag)
{
    this->stereo_video_width = stereo_video_width;
    this->stereo_video_height = stereo_video_height;

    _init_dev_files();

    // left
    if (_find_and_remove_dev_file_by_strs(stereo_camera_left_strs, stereo_camera_left_dev_file)) {
        printf(">> Stereo Camera (Left) (%s)\n", stereo_camera_left_dev_file.c_str());
        if (!_configure_camera(stereo_camera_left_dev_file))
        {
            fprintf(stderr, "[ERROR] Couldn't configure Stereo Camera (Left)\n");
            return -1;
        }
    } else {
        fprintf(stderr, "[ERROR] Couldn't find Stereo Camera's device file (Left)\n");
        return -1;
    }

    if (mono_flag)
        return 0;

    // right
    if (_find_and_remove_dev_file_by_strs(stereo_camera_right_strs, stereo_camera_right_dev_file)) {
        printf(">> Stereo Camera (Right) (%s)\n", stereo_camera_right_dev_file.c_str());
        if (!_configure_camera(stereo_camera_right_dev_file))
        {
            fprintf(stderr, "[ERROR] Couldn't configure Stereo Camera (Right)\n");
            return -1;
        }
    } else {
        fprintf(stderr, "[ERROR] Couldn't find Stereo Camera's device file (Right)\n");
        return -1;
    }

    initialized = 1;
    return 0;
}

int V4L2StereoCamera::deinit(void)
{
    return 0;
}

int V4L2StereoCamera::isValid(void)
{
    return initialized;
}

int V4L2StereoCamera::getGStreamVideoSourceLeftStr(std::string &str)
{
    str = stereo_camera_left_dev_file;
    if (str.empty())
        return -1;
    return 0;
}

int V4L2StereoCamera::getGStreamVideoSourceRightStr(std::string &str)
{
    str = stereo_camera_right_dev_file;
    if (str.empty())
        return -1;
    return 0;
}
